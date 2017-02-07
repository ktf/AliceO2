#include "DataFlow/SubframeBuilder.h"

AliceO2::DataFlow::SubframeBuilder::SubframeBuilder()
  : O2device()
{
}

AliceO2::DataFlow::SubframeBuilder::~SubframeBuilder()
{
}

void AliceO2::DataFlow::SubframeBuilder::InitTask()
{
}

bool AliceO2::DataFlow::SubframeBuilder::HandleData(const FairMQParts& msgParts)
{
  return true;
}
