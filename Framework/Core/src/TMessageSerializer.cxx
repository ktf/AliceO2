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
#include <Framework/TMessageSerializer.h>
#include <algorithm>
#include <memory>

using namespace o2::framework;

TMessageSerializer::StreamerList TMessageSerializer::sStreamers{};
std::mutex TMessageSerializer::sStreamersLock{};

void TMessageSerializer::updateStreamers(const FairTMessage& message, StreamerList& streamers)
{
  std::lock_guard<std::mutex> lock{TMessageSerializer::sStreamersLock};

  TIter nextStreamer(message.GetStreamerInfos()); // unfortunately ROOT uses TList* here
  // this looks like we could use std::map here.
  while (TVirtualStreamerInfo* in = static_cast<TVirtualStreamerInfo*>(nextStreamer())) {
    auto found = std::find_if(streamers.begin(), streamers.end(), [&](const auto& old) {
      return (old->GetName() == in->GetName() && old->GetClassVersion() == in->GetClassVersion());
    });
    if (found == streamers.end()) {
      streamers.push_back(in);
    }
  }
}

void TMessageSerializer::updateStreamers(const TObject* object)
{
  FairTMessage msg(kMESS_OBJECT);
  serialize(msg, object, CacheStreamers::yes, CompressionLevel{0});
}
