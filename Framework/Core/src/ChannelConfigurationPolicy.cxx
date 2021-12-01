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

#include "Framework/ChannelConfigurationPolicy.h"
#include "Framework/ConfigContext.h"

namespace o2::framework
{

ChannelConfigurationPolicy defaultDispatcherPolicy(ConfigContext const& configContext)
{
  ChannelConfigurationPolicy policy;
  FairMQChannelConfigSpec spec;
  auto &options = configContext.options();
  spec.rateLogging = options.get<int>("fairmq-rate-logging");
  spec.recvBufferSize = options.isDefault("fairmq-recv-buffer-size") ? 256 : options.get<int>("fairmq-recv-buffer-size");
  spec.sendBufferSize = options.isDefault("fairmq-send-buffer-size") ? 256 : options.get<int>("fairmq-send-buffer-size");
  spec.ipcPrefix = options.get<std::string>("fairmq-ipc-prefix");
  policy.match = ChannelConfigurationPolicyHelpers::matchByProducerName("Dispatcher");
  // Notice we swap the Bind / Connect method and let the pusher to connect so that
  // the queue is correctly filled even if the peers are not yet fully setup.
  policy.modifyInput = [spec](InputChannelSpec& channel) {
    channel.method = ChannelMethod::Bind;
    channel.type = ChannelType::Pull;
    channel.rateLogging = spec.rateLogging;
    channel.recvBufferSize = spec.recvBufferSize;
    channel.sendBufferSize = spec.sendBufferSize;
    channel.ipcPrefix = spec.ipcPrefix;
  };

  policy.modifyOutput = [spec](OutputChannelSpec& channel) {
    channel.method = ChannelMethod::Connect;
    channel.type = ChannelType::Push;
    channel.rateLogging = spec.rateLogging;
    channel.recvBufferSize = spec.recvBufferSize;
    channel.sendBufferSize = spec.sendBufferSize;
    channel.ipcPrefix = spec.ipcPrefix;
  };
  return policy;
}

std::vector<ChannelConfigurationPolicy> ChannelConfigurationPolicy::createDefaultPolicies(ConfigContext const& configContext)
{
  ChannelConfigurationPolicy defaultPolicy;
  FairMQChannelConfigSpec spec;
  spec.rateLogging = configContext.options().get<int>("fairmq-rate-logging");
  spec.recvBufferSize = configContext.options().get<int>("fairmq-recv-buffer-size");
  spec.sendBufferSize = configContext.options().get<int>("fairmq-send-buffer-size");
  spec.ipcPrefix = configContext.options().get<std::string>("fairmq-ipc-prefix");

  defaultPolicy.match = ChannelConfigurationPolicyHelpers::matchAny;
  defaultPolicy.modifyInput = ChannelConfigurationPolicyHelpers::pullInput(spec);
  defaultPolicy.modifyOutput = ChannelConfigurationPolicyHelpers::pushOutput(spec);

  return {defaultDispatcherPolicy(configContext), defaultPolicy};
}

} // namespace o2::framework
