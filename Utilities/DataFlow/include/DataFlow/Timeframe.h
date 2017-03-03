#ifndef Timeframe_H
#define Timeframe_H

#include "FairMQParts.h"
#include "TObject.h" // for ClassDef

namespace AliceO2
{
namespace DataFlow
{
// helper struct so that we can
// stream messages using ROOT
struct MessageSizePair {
  MessageSizePair() : size(0), buffer(nullptr) {}
  MessageSizePair(size_t s, char* b) : size(s), buffer(b) {}
  Int_t size;   // size of buffer in bytes (must be Int_t due to ROOT requirement)
  char* buffer; //[size]
};

// a class describing and encapsulating a complete timeframe
class Timeframe
{
 public:
  Timeframe() : mParts() {} // in principle just for ROOT io
  // constructor taking FairMQParts
  // might offer another constructor not depending on FairMQ ... directly taking buffers?
  // take care of ownership later
  Timeframe(FairMQParts& parts) : mParts()
  {
    for (int i = 0; i < parts.Size(); ++i) {
      mParts.push_back(MessageSizePair(parts[i].GetSize(), (char*)parts[i].GetData()));
    }
  };

  // return timestamp (starttime of this timeframe)
  size_t GetTimeStamp() const { return 0; }
  // return duration of this Timeframe
  double GetDuration() const { return 0.; }
  // from how many flps we received data
  int GetNumFlps() const { return 0; }
  // get missing flps

  // is this timeframe complete
  bool IsComplete() const { return false; }
  // return the number of message parts in this timeframe
  size_t GetNumParts() const { return mParts.size(); }
  // access to the data
  MessageSizePair& GetPart(size_t i) { return mParts[i]; }
  // Get total payload size in bytes
  size_t GetPayloadSize() const { return 0; }
  // Get payload size of part i
  size_t GetPayloadSize(size_t i) const { return 0; }
  // possibly detector specific functions?
  // return detector specific raw data types
  void* GetTPCData() const { return nullptr; }
 private:
  size_t mEpnId; // EPN origin of timeframe

  std::vector<MessageSizePair> mParts; // the message parts as accumulated by the EPN

  // Index mIndex; // index structure into parts

  ClassDefNV(Timeframe, 1);
};
}
}

#endif
