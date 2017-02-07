//-*- Mode: C++ -*-

#ifndef SUBFRAMEBUILDER_H
#define SUBFRAMEBUILDER_H

#include "Headers/DataHeader.h"
#include "Headers/HeartbeatFrame.h"
#include "O2device/O2device.h"

namespace AliceO2 {
namespace DataFlow {

/// @class SubframeBuilder
///
/// The device inplements the high-level API of the FairMQDevice
/// run loop. The registered method is called once per message
/// which is in itself a multipart message. It;s parts are passed
/// with a FairMQParts object. Not yet clear if this aproach is
/// suited for the framebuilder as it needs the data from multiple
/// channels with the same channel name. Depends on the validity of
/// the message data. Very likely the message parts are no longer
/// valid after leaving the handler method. But one could apply a
/// scheme with unique pointers and move semantics
class SubframeBuilder : public Base::O2device
{
public:
  /// Default constructor
  SubframeBuilder();

  /// Default destructor
  virtual ~SubframeBuilder();

protected:
  /// overloading the InitTask() method of FairMQDevice
  virtual void InitTask();

  /// data handling method to be registered as handler in the
  /// FairMQDevice API method OnData
  /// The device base class handles the state loop in the RUNNING
  /// state and calls the handler when receiving a message on one channel
  /// The multiple parts included in one message are provided in the
  /// FairMQParts object.
  bool HandleData(const FairMQParts& msgParts);

private:
};

}; // namespace DataFlow
}; // namespace AliceO2
#endif
