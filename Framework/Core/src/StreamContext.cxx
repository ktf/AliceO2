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

#include "Framework/StreamContext.h"

namespace o2::framework
{

/// Invoke callbacks to be executed before every process method invokation
void StreamContext::preProcessingCallbacks(ProcessingContext& pcx)
{
  auto& streamContext = pcx.services().get<StreamContext>();
  for (auto& handle : streamContext.preProcessingHandles) {
    handle.callback(pcx, handle.service);
  }
}

/// Invoke callbacks to be executed after every process method invokation
void StreamContext::postProcessingCallbacks(ProcessingContext& pcx)
{
  auto& streamContext = pcx.services().get<StreamContext>();
  for (auto& handle : streamContext.postProcessingHandles) {
    assert(handle.service);
    handle.callback(pcx, handle.service);
  }
}

/// Invoke callbacks to be executed before every EOS user callback invokation
void StreamContext::preEOSCallbacks(EndOfStreamContext& eosContext)
{
  for (auto& eosHandle : preEOSHandles) {
    eosHandle.callback(eosContext, eosHandle.service);
  }
}

/// Invoke callbacks to be executed after every EOS user callback invokation
void StreamContext::postEOSCallbacks(EndOfStreamContext& eosContext)
{
  for (auto& eosHandle : postEOSHandles) {
    eosHandle.callback(eosContext, eosHandle.service);
  }
}

} // namespace o2::framework
