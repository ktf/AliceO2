#include "DataFlow/SubframeBuilderDevice.h"

AliceO2::DataFlow::SubframeBuilderDevice::SubframeBuilderDevice()
  : O2device()
{
}

AliceO2::DataFlow::SubframeBuilderDevice::~SubframeBuilderDevice()
{
}

void AliceO2::DataFlow::SubframeBuilderDevice::InitTask()
{
}

bool AliceO2::DataFlow::SubframeBuilderDevice::ConditionalRun()
{
  return false;
}

bool AliceO2::DataFlow::SubframeBuilderDevice::HandleData(const FairMQParts& msgParts)
{
  return true;
}
