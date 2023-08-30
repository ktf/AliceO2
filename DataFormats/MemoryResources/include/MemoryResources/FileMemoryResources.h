// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

#ifndef O2_DATAHEADERS_MEMORYRESOURCES_FILEMEMORYRESOURCES_H_
#define O2_DATAHEADERS_MEMORYRESOURCES_FILEMEMORYRESOURCES_H_

#include <boost/container/pmr/memory_resource.hpp>
#include <boost/container/pmr/monotonic_buffer_resource.hpp>
#include <boost/container/pmr/polymorphic_allocator.hpp>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

namespace o2::pmr
{
class FileMemoryResource : public boost::container::pmr::memory_resource
{
 public:
  FileMemoryResource(char const* filename, size_t size)
    : mFilename(filename)
  {
    // Open the file
  }
  virtual void resize(size_t size)
  {
    // We have not created the file yet, lets do now.
    if (mMapAddress == nullptr) {
      mFd = open(mFilename.data(), O_RDWR | O_CREAT, 0666);
      mMapAddress = mmap(nullptr, size, PROT_WRITE | PROT_READ, MAP_SHARED, mFd, 0);
    }
  }

  virtual void close()
  {
    munmap(mMapAddress, 0);
    ::fsync(mFd);
    ::close(mFd);
  }

  virtual void* getMapAddress() { return mMapAddress; }

  virtual void setWriteCallback(void* data, size_t size)
  {
    memcpy(mMapAddress, data, size);
  }

 private:
  std::string mFilename;
  void* mMapAddress;
  size_t offset = 0;
  int mFd = -1;
};
} // namespace o2::pmr
#endif // O2_DATAHEADERS_MEMORYRESOURCES_FILEMEMORYRESOURCES_H_
