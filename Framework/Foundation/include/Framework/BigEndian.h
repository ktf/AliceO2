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

#ifndef O2_FRAMEWORK_BIGENDIAN_H_
#define O2_FRAMEWORK_BIGENDIAN_H_

#include "Framework/RuntimeError.h"
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace o2::framework
{

/// Copy @a count elements of @a typeSize bytes each from big-endian @a src
/// into native byte order at @a dest. For typeSize == 1 or on big-endian
/// platforms this reduces to a plain memcpy. @a dest must be 64-byte aligned
/// and must not overlap with @a src.
inline void bigEndianCopy(void* dest, const void* src, int count, size_t typeSize)
{
  auto const totalBytes = static_cast<size_t>(count) * typeSize;
  if (reinterpret_cast<uintptr_t>(dest) % 64 != 0) {
    throw runtime_error_f("bigEndianCopy: destination is not 64-byte aligned");
  }
  if (reinterpret_cast<uintptr_t>(src) < reinterpret_cast<uintptr_t>(dest) + totalBytes &&
      reinterpret_cast<uintptr_t>(dest) < reinterpret_cast<uintptr_t>(src) + totalBytes) {
    throw runtime_error_f("bigEndianCopy: source and destination ranges overlap");
  }
  if constexpr (std::endian::native == std::endian::big) {
    std::memcpy(dest, src, totalBytes);
    return;
  }
  switch (typeSize) {
    case 2: {
      auto* p = static_cast<uint16_t*>(dest);
      auto* q = static_cast<const uint16_t*>(src);
      for (int i = 0; i < count; ++i) {
        p[i] = __builtin_bswap16(q[i]);
      }
      return;
    }
    case 4: {
      auto* p = static_cast<uint32_t*>(dest);
      auto* q = static_cast<const uint32_t*>(src);
      for (int i = 0; i < count; ++i) {
        p[i] = __builtin_bswap32(q[i]);
      }
      return;
    }
    case 8: {
      auto* p = static_cast<uint64_t*>(dest);
      auto* q = static_cast<const uint64_t*>(src);
      for (int i = 0; i < count; ++i) {
        p[i] = __builtin_bswap64(q[i]);
      }
      return;
    }
  }
  std::memcpy(dest, src, totalBytes);
}

} // namespace o2::framework

#endif // O2_FRAMEWORK_BIGENDIAN_H_
