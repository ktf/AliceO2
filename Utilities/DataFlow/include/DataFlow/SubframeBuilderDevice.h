//-*- Mode: C++ -*-

#ifndef SUBFRAMEBUILDERDEVICE_H
#define SUBFRAMEBUILDERDEVICE_H

#include "Headers/DataHeader.h"
#include "Headers/HeartbeatFrame.h"
#include "O2device/O2device.h"

class FairMQParts;

namespace AliceO2 {
namespace DataFlow {

/// @class SubframeBuilderDevice
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
class SubframeBuilderDevice : public Base::O2device
{
public:
  /// Default constructor
  SubframeBuilderDevice();

  /// Default destructor
  virtual ~SubframeBuilderDevice() final;

protected:
  /// overloading the InitTask() method of FairMQDevice
  void InitTask() final;

   /// overloading ConditionalRun method of FairMQDevice
  virtual bool ConditionalRun() final;

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
