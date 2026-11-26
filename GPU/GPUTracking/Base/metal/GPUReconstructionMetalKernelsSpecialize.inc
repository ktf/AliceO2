// Copyright 2019-2025 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file GPUReconstructionMetalKernelsSpecialize.inc

#include <chrono>

template <>
inline void GPUReconstructionMetal::runKernelBackend<GPUMemClean16, 0>(const krnlSetupTime& _xyz, void* const& ptr, uint64_t const& size)
{
  const uint64_t offset = static_cast<const uint8_t*>(ptr) - static_cast<const uint8_t*>(mDeviceMemoryBase);
  const uint64_t length = (size + 15ull) & ~15ull;
  if (_xyz.z.evList) {
    StreamWaitForEvents(_xyz.x.stream, _xyz.z.evList, _xyz.z.nEvents);
  }

  id<MTLCommandBuffer> cb = mInternals->commandBuffers[_xyz.x.stream];
  id<MTLBlitCommandEncoder> blit = [cb blitCommandEncoder];
  metalBeginEncoder(mInternals, blit, _xyz.x.stream);

  [blit fillBuffer:mInternals->mem_gpu range:NSMakeRange(offset, length) value:0];
  metalEndEncoder(mInternals, blit, _xyz.x.stream);
  if (_xyz.z.ev) {
    RecordMarker(_xyz.z.ev, _xyz.x.stream);
  }
}

// Kernels that are a long serial chain per GPU thread, with little parallelism,
// run on the host instead, with the CPU code, on the memory shared with the
// host, as the sorts do with UNIFIED_MEM_SORT_ON_HOST.
template <class T, int32_t I, typename... Args>
inline void GPUReconstructionMetal::RunOnHost(const krnlSetupTime& _xyz, uint32_t nBlocks, const Args&... args)
{
  const auto start = std::chrono::steady_clock::now();
  krnlSetupArgs<T, I, Args...> argPack{{}, {{{nBlocks, 1, _xyz.x.stream, krnlDeviceType::HostOnDeviceMemory, _xyz.x.step}, _xyz.y, _xyz.z}, _xyz.t}, {args...}};
  runKernelOnHost(GetKernelNum<T, I>(), &argPack);
  _xyz.t = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
}

template <>
inline void GPUReconstructionMetal::runKernelBackend<GPUTPCGMMergerMergeBorders, 2>(const krnlSetupTime& _xyz, int32_t const& iSector, uint8_t const& mergeMode)
{
  RunOnHost<GPUTPCGMMergerMergeBorders, 2>(_xyz, 1, iSector, mergeMode);
}
