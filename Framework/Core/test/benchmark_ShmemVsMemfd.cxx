// Copyright 2019-2026 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file benchmark_ShmemVsMemfd.cxx
/// \brief Head-to-head benchmark: FairMQ shmem transport vs memfd+UDS fd passing
///
/// Self-contained single-file benchmark using fork() for sender/receiver.
/// Approach A: FairMQ shmem push/pull channel with per-message allocation
/// Approach B: memfd (Linux) or shm_open (macOS) + bump allocator + UDS SCM_RIGHTS

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <numeric>
#include <string>
#include <vector>

#include <fcntl.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

#include <fairmq/Channel.h>
#include <fairmq/Message.h>
#include <fairmq/Parts.h>
#include <fairmq/ProgOptions.h>
#include <fairmq/TransportFactory.h>

// ---------------------------------------------------------------------------
// Parameters
// ---------------------------------------------------------------------------
static constexpr int N_MESSAGES = 100;
static constexpr int N_ITERATIONS = 1000;
static constexpr size_t ALIGNMENT = 64;

// Realistic mix: 50x4KB + 30x64KB + 15x256KB + 5x1MB
static std::vector<size_t> generateMessageSizes()
{
  std::vector<size_t> sizes;
  sizes.reserve(N_MESSAGES);
  for (int i = 0; i < 50; ++i) {
    sizes.push_back(4 * 1024);
  }
  for (int i = 0; i < 30; ++i) {
    sizes.push_back(64 * 1024);
  }
  for (int i = 0; i < 15; ++i) {
    sizes.push_back(256 * 1024);
  }
  for (int i = 0; i < 5; ++i) {
    sizes.push_back(1024 * 1024);
  }
  return sizes;
}

static size_t totalPayloadSize(const std::vector<size_t>& sizes)
{
  return std::accumulate(sizes.begin(), sizes.end(), size_t{0});
}

static size_t alignUp(size_t v, size_t align)
{
  return (v + align - 1) & ~(align - 1);
}

// Fill buffer with a pattern that depends on both iteration and message index,
// so swapped or misrouted messages are detected.
static void fillPattern(void* buf, size_t size, uint8_t iterSeed, int msgIndex)
{
  auto* p = static_cast<uint8_t*>(buf);
  uint8_t base = static_cast<uint8_t>(iterSeed ^ (msgIndex * 37));
  for (size_t i = 0; i < size; ++i) {
    p[i] = static_cast<uint8_t>(base + (i & 0xFF));
  }
}

static bool verifyPattern(const void* buf, size_t size, uint8_t iterSeed, int msgIndex)
{
  auto* p = static_cast<const uint8_t*>(buf);
  uint8_t base = static_cast<uint8_t>(iterSeed ^ (msgIndex * 37));
  for (size_t i = 0; i < size; ++i) {
    if (p[i] != static_cast<uint8_t>(base + (i & 0xFF))) {
      return false;
    }
  }
  return true;
}

// ---------------------------------------------------------------------------
// Timing results communicated from child to parent via pipe
// ---------------------------------------------------------------------------
struct TimingResult {
  double totalMs;
};

using Clock = std::chrono::high_resolution_clock;

static double msElapsed(Clock::time_point start, Clock::time_point end)
{
  return std::chrono::duration<double, std::milli>(end - start).count();
}

// ---------------------------------------------------------------------------
// Helper: create anonymous shared memory fd (portable)
// ---------------------------------------------------------------------------
static int createAnonymousShmFd(size_t size)
{
#ifdef __linux__
  int fd = memfd_create("benchmark_region", MFD_CLOEXEC);
  if (fd < 0) {
    perror("memfd_create");
    return -1;
  }
#else
  // macOS fallback: shm_open + shm_unlink for an anonymous-like fd
  // shm_open names must be short (max 31 chars on macOS including the leading /)
  static int shmCounter = 0;
  char name[32];
  snprintf(name, sizeof(name), "/bm_%d_%d", getpid(), shmCounter++);
  int fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0600);
  if (fd < 0) {
    perror("shm_open");
    return -1;
  }
  shm_unlink(name); // unlink immediately so it's anonymous
#endif
  if (ftruncate(fd, static_cast<off_t>(size)) != 0) {
    perror("ftruncate");
    close(fd);
    return -1;
  }
  return fd;
}

// ---------------------------------------------------------------------------
// Approach A: FairMQ shmem push/pull
// ---------------------------------------------------------------------------
struct ApproachAResult {
  double allocFillMs;
  double sendMs;
  double receiveMs;
};

static ApproachAResult benchmarkFairMQShmem(const std::vector<size_t>& sizes)
{
  // Use a unique IPC path and session to avoid collisions
  std::string ipcPath = "ipc:///tmp/benchmark_fairmq_" + std::to_string(getpid());

  // Pipe for child to send timing back to parent
  int timePipe[2];
  if (pipe(timePipe) != 0) {
    perror("pipe");
    exit(1);
  }

  // Sync pipe: parent writes a byte after binding, child reads before connecting
  int syncPipe[2];
  if (pipe(syncPipe) != 0) {
    perror("pipe");
    exit(1);
  }

  pid_t pid = fork();
  if (pid < 0) {
    perror("fork");
    exit(1);
  }

  if (pid == 0) {
    // --- Child: receiver (pull) ---
    close(timePipe[0]); // close read end
    close(syncPipe[1]); // close write end

    // Wait for parent to bind
    char syncByte;
    if (read(syncPipe[0], &syncByte, 1) != 1) {
      _exit(1);
    }
    close(syncPipe[0]);

    size_t session = static_cast<size_t>(getppid()) * 1000 + 1;
    fair::mq::ProgOptions config;
    config.SetProperty<std::string>("session", std::to_string(session));

    auto factory = fair::mq::TransportFactory::CreateTransportFactory("zeromq");
    fair::mq::Channel channel("benchmark", "pull", factory);
    channel.Connect(ipcPath);
    channel.Validate();

    double totalReceiveMs = 0.0;

    for (int iter = 0; iter < N_ITERATIONS; ++iter) {
      fair::mq::Parts parts;
      auto t0 = Clock::now();
      auto rc = channel.Receive(parts, 30000); // 30s timeout
      auto t1 = Clock::now();

      if (rc < 0) {
        fprintf(stderr, "FairMQ Receive failed: %ld\n", (long)rc);
        _exit(1);
      }

      // Verify data integrity
      for (int i = 0; i < static_cast<int>(parts.Size()); ++i) {
        if (!verifyPattern(parts[i].GetData(), parts[i].GetSize(),
                           static_cast<uint8_t>(iter & 0xFF), i)) {
          fprintf(stderr, "FairMQ: data verification failed at iter=%d msg=%d\n", iter, i);
          _exit(1);
        }
      }
      totalReceiveMs += msElapsed(t0, t1);
    }

    TimingResult result{totalReceiveMs};
    if (write(timePipe[1], &result, sizeof(result)) != sizeof(result)) {
      perror("write timing");
    }
    close(timePipe[1]);
    _exit(0);
  }

  // --- Parent: sender (push) ---
  close(timePipe[1]); // close write end
  close(syncPipe[0]); // close read end

  size_t session = static_cast<size_t>(getpid()) * 1000 + 1;
  fair::mq::ProgOptions config;
  config.SetProperty<std::string>("session", std::to_string(session));

  auto factory = fair::mq::TransportFactory::CreateTransportFactory("zeromq");
  fair::mq::Channel channel("benchmark", "push", factory);
  channel.Bind(ipcPath);
  channel.Validate();

  // Signal child that we've bound
  char syncByte = 'G';
  if (write(syncPipe[1], &syncByte, 1) != 1) {
    perror("write sync");
  }
  close(syncPipe[1]);

  // Give child a moment to connect
  usleep(50000);

  double totalAllocFillMs = 0.0;
  double totalSendMs = 0.0;

  for (int iter = 0; iter < N_ITERATIONS; ++iter) {
    fair::mq::Parts parts;

    auto t0 = Clock::now();
    for (int m = 0; m < N_MESSAGES; ++m) {
      auto msg = factory->CreateMessage(sizes[m]);
      fillPattern(msg->GetData(), sizes[m], static_cast<uint8_t>(iter & 0xFF), m);
      parts.AddPart(std::move(msg));
    }
    auto t1 = Clock::now();

    auto rc = channel.Send(parts, 30000);
    auto t2 = Clock::now();

    if (rc < 0) {
      fprintf(stderr, "FairMQ Send failed: %ld\n", (long)rc);
      exit(1);
    }

    totalAllocFillMs += msElapsed(t0, t1);
    totalSendMs += msElapsed(t1, t2);
  }

  // Read child timing
  TimingResult childResult{};
  if (read(timePipe[0], &childResult, sizeof(childResult)) != sizeof(childResult)) {
    perror("read timing");
  }
  close(timePipe[0]);

  int status = 0;
  waitpid(pid, &status, 0);
  if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
    fprintf(stderr, "FairMQ child exited abnormally\n");
  }

  // Clean up IPC file
  std::string ipcFile = "/tmp/benchmark_fairmq_" + std::to_string(getpid());
  unlink(ipcFile.c_str());

  return ApproachAResult{
    totalAllocFillMs / N_ITERATIONS,
    totalSendMs / N_ITERATIONS,
    childResult.totalMs / N_ITERATIONS};
}

// ---------------------------------------------------------------------------
// Approach B: memfd + bump allocator + UDS fd passing
// ---------------------------------------------------------------------------

// Manifest entry describing one message within the shared region
struct ManifestEntry {
  uint32_t offset;
  uint32_t size;
};

struct Manifest {
  uint32_t count;
  uint32_t totalSize;
  ManifestEntry entries[N_MESSAGES];
};

// Send fd + manifest over UDS using SCM_RIGHTS
static bool sendFdAndManifest(int sockFd, int shmFd, const Manifest& manifest)
{
  struct msghdr msg = {};
  struct iovec iov = {};
  iov.iov_base = const_cast<Manifest*>(&manifest);
  iov.iov_len = sizeof(manifest);
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;

  // Ancillary data for SCM_RIGHTS
  union {
    char buf[CMSG_SPACE(sizeof(int))];
    struct cmsghdr align;
  } cmsgBuf = {};

  msg.msg_control = cmsgBuf.buf;
  msg.msg_controllen = sizeof(cmsgBuf.buf);

  struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
  cmsg->cmsg_level = SOL_SOCKET;
  cmsg->cmsg_type = SCM_RIGHTS;
  cmsg->cmsg_len = CMSG_LEN(sizeof(int));
  memcpy(CMSG_DATA(cmsg), &shmFd, sizeof(int));

  ssize_t sent = sendmsg(sockFd, &msg, 0);
  return sent >= 0;
}

// Receive fd + manifest from UDS
static bool recvFdAndManifest(int sockFd, int& shmFd, Manifest& manifest)
{
  struct msghdr msg = {};
  struct iovec iov = {};
  iov.iov_base = &manifest;
  iov.iov_len = sizeof(manifest);
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;

  union {
    char buf[CMSG_SPACE(sizeof(int))];
    struct cmsghdr align;
  } cmsgBuf = {};

  msg.msg_control = cmsgBuf.buf;
  msg.msg_controllen = sizeof(cmsgBuf.buf);

  ssize_t received = recvmsg(sockFd, &msg, 0);
  if (received < static_cast<ssize_t>(sizeof(manifest))) {
    return false;
  }

  struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
  if (cmsg && cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_RIGHTS) {
    memcpy(&shmFd, CMSG_DATA(cmsg), sizeof(int));
    return true;
  }
  return false;
}

struct ApproachBResult {
  double allocFillMs;
  double sendMs;
  double receiveMs;
};

static ApproachBResult benchmarkMemfdUDS(const std::vector<size_t>& sizes)
{
  std::string sockPath = "/tmp/benchmark_memfd_" + std::to_string(getpid()) + ".sock";
  unlink(sockPath.c_str());

  // Pipe for child to send timing back
  int timePipe[2];
  if (pipe(timePipe) != 0) {
    perror("pipe");
    exit(1);
  }

  // Sync pipe: parent writes after listen(), child reads before connect()
  int syncPipe[2];
  if (pipe(syncPipe) != 0) {
    perror("pipe");
    exit(1);
  }

  // Compute total bump region size (with alignment)
  size_t regionSize = 0;
  for (int m = 0; m < N_MESSAGES; ++m) {
    regionSize += alignUp(sizes[m], ALIGNMENT);
  }

  pid_t pid = fork();
  if (pid < 0) {
    perror("fork");
    exit(1);
  }

  if (pid == 0) {
    // --- Child: receiver ---
    close(timePipe[0]);
    close(syncPipe[1]);

    // Wait for parent to listen
    char syncByte;
    if (read(syncPipe[0], &syncByte, 1) != 1) {
      _exit(1);
    }
    close(syncPipe[0]);

    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
      perror("socket");
      _exit(1);
    }

    struct sockaddr_un addr = {};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, sockPath.c_str(), sizeof(addr.sun_path) - 1);

    if (connect(sock, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) != 0) {
      perror("connect");
      _exit(1);
    }

    double totalReceiveMs = 0.0;

    for (int iter = 0; iter < N_ITERATIONS; ++iter) {
      Manifest manifest{};
      int shmFd = -1;

      auto t0 = Clock::now();
      if (!recvFdAndManifest(sock, shmFd, manifest)) {
        fprintf(stderr, "memfd: recvFdAndManifest failed at iter=%d\n", iter);
        _exit(1);
      }

      void* region = mmap(nullptr, manifest.totalSize, PROT_READ, MAP_SHARED, shmFd, 0);
      if (region == MAP_FAILED) {
        perror("mmap receiver");
        _exit(1);
      }
      auto t1 = Clock::now();

      // Verify
      for (uint32_t m = 0; m < manifest.count; ++m) {
        const auto& entry = manifest.entries[m];
        if (!verifyPattern(static_cast<const uint8_t*>(region) + entry.offset,
                           entry.size, static_cast<uint8_t>(iter & 0xFF), static_cast<int>(m))) {
          fprintf(stderr, "memfd: data verification failed at iter=%d msg=%u\n", iter, m);
          _exit(1);
        }
      }

      munmap(region, manifest.totalSize);
      close(shmFd);
      totalReceiveMs += msElapsed(t0, t1);
    }

    close(sock);

    TimingResult result{totalReceiveMs};
    if (write(timePipe[1], &result, sizeof(result)) != sizeof(result)) {
      perror("write timing");
    }
    close(timePipe[1]);
    _exit(0);
  }

  // --- Parent: sender ---
  close(timePipe[1]);
  close(syncPipe[0]);

  int listenSock = socket(AF_UNIX, SOCK_STREAM, 0);
  if (listenSock < 0) {
    perror("socket");
    exit(1);
  }

  struct sockaddr_un addr = {};
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, sockPath.c_str(), sizeof(addr.sun_path) - 1);

  if (bind(listenSock, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) != 0) {
    perror("bind");
    exit(1);
  }
  if (listen(listenSock, 1) != 0) {
    perror("listen");
    exit(1);
  }

  // Signal child that we're listening
  char syncByte = 'G';
  if (write(syncPipe[1], &syncByte, 1) != 1) {
    perror("write sync");
  }
  close(syncPipe[1]);

  int connSock = accept(listenSock, nullptr, nullptr);
  if (connSock < 0) {
    perror("accept");
    exit(1);
  }

  double totalAllocFillMs = 0.0;
  double totalSendMs = 0.0;

  for (int iter = 0; iter < N_ITERATIONS; ++iter) {
    auto t0 = Clock::now();

    // Create anonymous shared memory region
    int shmFd = createAnonymousShmFd(regionSize);
    if (shmFd < 0) {
      exit(1);
    }

    void* region = mmap(nullptr, regionSize, PROT_READ | PROT_WRITE, MAP_SHARED, shmFd, 0);
    if (region == MAP_FAILED) {
      perror("mmap sender");
      exit(1);
    }

    // Bump-allocate and fill
    Manifest manifest{};
    manifest.count = N_MESSAGES;
    manifest.totalSize = static_cast<uint32_t>(regionSize);
    size_t offset = 0;
    for (int m = 0; m < N_MESSAGES; ++m) {
      manifest.entries[m].offset = static_cast<uint32_t>(offset);
      manifest.entries[m].size = static_cast<uint32_t>(sizes[m]);
      fillPattern(static_cast<uint8_t*>(region) + offset, sizes[m],
                  static_cast<uint8_t>(iter & 0xFF), m);
      offset += alignUp(sizes[m], ALIGNMENT);
    }
    auto t1 = Clock::now();

    // Send fd + manifest
    if (!sendFdAndManifest(connSock, shmFd, manifest)) {
      fprintf(stderr, "memfd: sendFdAndManifest failed at iter=%d\n", iter);
      exit(1);
    }
    auto t2 = Clock::now();

    munmap(region, regionSize);
    close(shmFd);

    totalAllocFillMs += msElapsed(t0, t1);
    totalSendMs += msElapsed(t1, t2);
  }

  close(connSock);
  close(listenSock);
  unlink(sockPath.c_str());

  // Read child timing
  TimingResult childResult{};
  if (read(timePipe[0], &childResult, sizeof(childResult)) != sizeof(childResult)) {
    perror("read timing");
  }
  close(timePipe[0]);

  int status = 0;
  waitpid(pid, &status, 0);
  if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
    fprintf(stderr, "memfd child exited abnormally\n");
  }

  return ApproachBResult{
    totalAllocFillMs / N_ITERATIONS,
    totalSendMs / N_ITERATIONS,
    childResult.totalMs / N_ITERATIONS};
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------
int main()
{
  auto sizes = generateMessageSizes();
  size_t totalBytes = totalPayloadSize(sizes);
  double totalMB = static_cast<double>(totalBytes) / (1024.0 * 1024.0);

  printf("Benchmark: FairMQ shmem vs memfd+UDS\n");
  printf("  Messages per TF: %d\n", N_MESSAGES);
  printf("  Total payload:   %.2f MB per TF\n", totalMB);
  printf("  Iterations:      %d\n\n", N_ITERATIONS);

  // --- Approach A ---
  printf("Running FairMQ zeromq/IPC benchmark...\n");
  auto resultA = benchmarkFairMQShmem(sizes);

  // --- Approach B ---
  printf("Running memfd+UDS benchmark...\n");
  auto resultB = benchmarkMemfdUDS(sizes);

  // --- Print comparison ---
  double totalA = resultA.allocFillMs + resultA.sendMs + resultA.receiveMs;
  double throughputA = totalMB / (totalA / 1000.0);

  double totalB = resultB.allocFillMs + resultB.sendMs + resultB.receiveMs;
  double throughputB = totalMB / (totalB / 1000.0);

  printf("\n=== FairMQ zeromq/IPC (%d iterations, %d messages/TF) ===\n",
         N_ITERATIONS, N_MESSAGES);
  printf("  Alloc+Fill:  %.2f ms/TF\n", resultA.allocFillMs);
  printf("  Send:        %.2f ms/TF\n", resultA.sendMs);
  printf("  Receive:     %.2f ms/TF\n", resultA.receiveMs);
  printf("  Total:       %.2f ms/TF\n", totalA);
  printf("  Throughput:  %.2f GB/s\n", throughputA / 1024.0);

  printf("\n=== memfd + bump + UDS (%d iterations, %d messages/TF) ===\n",
         N_ITERATIONS, N_MESSAGES);
  printf("  Alloc+Fill:  %.2f ms/TF\n", resultB.allocFillMs);
  printf("  Send:        %.2f ms/TF\n", resultB.sendMs);
  printf("  Receive:     %.2f ms/TF\n", resultB.receiveMs);
  printf("  Total:       %.2f ms/TF\n", totalB);
  printf("  Throughput:  %.2f GB/s\n", throughputB / 1024.0);

  printf("\nSpeedup (memfd vs FairMQ): %.1fx\n", totalA / totalB);

  return 0;
}
