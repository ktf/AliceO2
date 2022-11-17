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

#include "Framework/DataProcessingContext.h"

namespace o2::framework
{
/// Invoke callbacks to be executed before every dangling check
void DataProcessorContext::preDanglingCallbacks(DanglingContext& danglingContext)
{
  for (auto preDanglingHandle : preDanglingHandles) {
    preDanglingHandle.callback(danglingContext, preDanglingHandle.service);
  }
}

/// Invoke callbacks to be executed after every dangling check
void DataProcessorContext::postDanglingCallbacks(DanglingContext& danglingContext)
{
  for (auto postDanglingHandle : postDanglingHandles) {
    LOGP(debug, "Doing postDanglingCallback for service {}", postDanglingHandle.spec.name);
    postDanglingHandle.callback(danglingContext, postDanglingHandle.service);
  }
}

/// Invoke callbacks to be executed before every EOS user callback invokation
void DataProcessorContext::preEOSCallbacks(EndOfStreamContext& eosContext)
{
  for (auto& eosHandle : preEOSHandles) {
    eosHandle.callback(eosContext, eosHandle.service);
  }
}

/// Invoke callbacks to be executed after every EOS user callback invokation
void DataProcessorContext::postEOSCallbacks(EndOfStreamContext& eosContext)
{
  for (auto& eosHandle : postEOSHandles) {
    eosHandle.callback(eosContext, eosHandle.service);
  }
}

/// Invoke callbacks to be executed after every data Dispatching
void DataProcessorContext::postDispatchingCallbacks(ProcessingContext& processContext)
{
  for (auto& dispatchingHandle : postDispatchingHandles) {
    dispatchingHandle.callback(processContext, dispatchingHandle.service);
  }
}

/// Invoke callbacks to be executed after every data Dispatching
void DataProcessorContext::postForwardingCallbacks(ProcessingContext& processContext)
{
  for (auto& forwardingHandle : postForwardingHandles) {
    forwardingHandle.callback(processContext, forwardingHandle.service);
  }
}

/// Callbacks to be called in fair::mq::Device::PreRun()
void DataProcessorContext::preStartCallbacks(ServiceRegistryRef ref)
{
  // FIXME: we need to call the callback only once for the global services
  /// I guess...
  for (auto startHandle = preStartHandles.begin(); startHandle != preStartHandles.end(); ++startHandle) {
    startHandle->callback(ref, startHandle->service);
  }
}

void DataProcessorContext::postStopCallbacks(ServiceRegistryRef ref)
{
  // FIXME: we need to call the callback only once for the global services
  /// I guess...
  for (auto& stopHandle : postStopHandles) {
    stopHandle.callback(ref, stopHandle.service);
  }
}

/// Invoke callback to be executed on exit, in reverse order.
void DataProcessorContext::preExitCallbacks(std::vector<ServiceExitHandle> handles, ServiceRegistryRef ref)
{
  // FIXME: we need to call the callback only once for the global services
  /// I guess...
  for (auto exitHandle = handles.rbegin(); exitHandle != handles.rend(); ++exitHandle) {
    exitHandle->callback(ref, exitHandle->service);
  }
}

void DataProcessorContext::domainInfoUpdatedCallback(ServiceRegistryRef ref, size_t oldestPossibleTimeslice, ChannelIndex channelIndex)
{
  for (auto& handle : domainInfoHandles) {
    handle.callback(ref, oldestPossibleTimeslice, channelIndex);
  }
}

void DataProcessorContext::preSendingMessagesCallbacks(ServiceRegistryRef ref, fair::mq::Parts& parts, ChannelIndex channelIndex)
{
  for (auto& handle : preSendingMessagesHandles) {
    handle.callback(ref, parts, channelIndex);
  }
}

} // namespace o2::framework
