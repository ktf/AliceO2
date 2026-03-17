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
#ifndef FRAMEWORK_MESSAGESET_H
#define FRAMEWORK_MESSAGESET_H

#include "Framework/PartRef.h"
#include <fairmq/Message.h>
#include "Framework/DataModelViews.h"
#include <memory>
#include <vector>
#include <cassert>

namespace o2::framework
{

/// A set of inflight messages.
/// The messages are stored in a linear vector. Originally, an O2 message was
/// comprised of a header-payload pair which makes indexing of pairs in the
/// storage simple. To support O2 messages with multiple payloads in a future
/// update of the data model, a message index is needed to store position in the
/// linear storage and number of messages.
/// DPL InputRecord API is providing refs of header-payload pairs, the original
/// O2 message model. For this purpose, also the pair index is filled and can
/// be used to access header and payload associated with a pair
struct MessageSet {
  // linear storage of messages
  std::vector<fair::mq::MessagePtr> messages;

  MessageSet()
    : messages()
  {
  }

  template <typename F>
  MessageSet(F getter, size_t size)
    : messages()
  {
    add(std::forward<F>(getter), size);
  }

  MessageSet(MessageSet&& other)
    : messages(std::move(other.messages))
  {
    other.clear();
  }

  MessageSet& operator=(MessageSet&& other)
  {
    if (&other == this) {
      return *this;
    }
    messages = std::move(other.messages);
    other.clear();
    return *this;
  }

  /// get number of in-flight O2 messages
  [[nodiscard]] size_t size() const
  {
    return messages | count_parts{};
  }

  /// get number of header-payload pairs
  [[nodiscard]] size_t getNumberOfPairs() const
  {
    return messages | count_payloads{};
  }

  /// get number of payloads for an in-flight message
  [[nodiscard]] size_t getNumberOfPayloads(size_t mi) const
  {
    return messages | get_num_payloads{mi};
  }

  /// clear the set
  void clear()
  {
    messages.clear();
  }

  // this is more or less legacy
  // PartRef has been earlier used to store fixed header-payload pairs
  // reset the set and store content of the part ref
  void reset(PartRef&& ref)
  {
    clear();
    add(std::move(ref));
  }

  // this is more or less legacy
  // PartRef has been earlier used to store fixed header-payload pairs
  // add  content of the part ref
  void add(PartRef&& ref)
  {
    messages.emplace_back(std::move(ref.header));
    messages.emplace_back(std::move(ref.payload));
  }

  /// add an O2 message
  template <typename F>
  void add(F getter, size_t size)
  {
    for (size_t i = 0; i < size; ++i) {
      messages.emplace_back(std::move(getter(i)));
    }
  }

  fair::mq::MessagePtr& header(size_t partIndex)
  {
    return messages[(messages | get_dataref_indices{partIndex, 0}).headerIdx];
  }

  fair::mq::MessagePtr& payload(size_t partIndex, size_t payloadIndex = 0)
  {
    return messages[(messages | get_dataref_indices{partIndex, payloadIndex}).payloadIdx];
  }

  [[nodiscard]] fair::mq::MessagePtr const& header(size_t partIndex) const
  {
    return messages[(messages | get_dataref_indices{partIndex, 0}).headerIdx];
  }

  [[nodiscard]] fair::mq::MessagePtr const& payload(size_t partIndex, size_t payloadIndex = 0) const
  {
    return messages[(messages | get_dataref_indices{partIndex, payloadIndex}).payloadIdx];
  }
};

} // namespace o2::framework

#endif // FRAMEWORK_MESSAGESET_H
