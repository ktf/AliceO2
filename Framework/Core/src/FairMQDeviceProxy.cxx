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

#include "Framework/FairMQDeviceProxy.h"

#include <fairmq/FairMQDevice.h>
#include <fairmq/Channel.h>
#include <fairmq/FairMQMessage.h>

namespace o2::framework
{

fair::mq::Channel* FairMQDeviceProxy::getChannel(RouteIndex index) const
{
  return mRoutes[index.value].channel;
}

FairMQTransportFactory* FairMQDeviceProxy::getTransport(RouteIndex index) const
{
  return mRoutes[index.value].channel->Transport();
}

std::unique_ptr<FairMQMessage> FairMQDeviceProxy::createMessage(RouteIndex routeIndex) const
{
  return getTransport(routeIndex)->CreateMessage(fair::mq::Alignment{64});
}

std::unique_ptr<FairMQMessage> FairMQDeviceProxy::createMessage(RouteIndex routeIndex, const size_t size) const
{
  return getTransport(routeIndex)->CreateMessage(size, fair::mq::Alignment{64});
}

void FairMQDeviceProxy::bindRoutes(std::vector<OutputRoute> const& outputs, fair::mq::Device& device)
{
  mRoutes.reserve(outputs.size());
  size_t ri = 0;
  for (auto& route : outputs) {
    LOGP(debug, "Binding route {} to index {}", route.channel, ri);
    ri++;
    auto* channel = &device.fChannels.at(route.channel).at(0);
    mRoutes.emplace_back(RouteState{channel, false});
  }
}
} // namespace o2::framework
