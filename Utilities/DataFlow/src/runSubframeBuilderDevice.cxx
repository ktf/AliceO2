#include "runFairMQDevice.h"
#include "DataFlow/SubframeBuilderDevice.h"

namespace bpo = boost::program_options;

void addCustomOptions(bpo::options_description& options)
{
  options.add_options()
    (AliceO2::DataFlow::SubframeBuilderDevice::OptionKeyDuration,
     bpo::value<uint32_t>()->default_value(10000),
     "Name of the input channel")
    (AliceO2::DataFlow::SubframeBuilderDevice::OptionKeyInputChannelName,
     bpo::value<std::string>()->default_value("input"),
     "Name of the input channel")
    (AliceO2::DataFlow::SubframeBuilderDevice::OptionKeyOutputChannelName,
     bpo::value<std::string>()->default_value("output"),
     "Name of the output channel");
}

FairMQDevicePtr getDevice(const FairMQProgOptions& /*config*/)
{
  return new AliceO2::DataFlow::SubframeBuilderDevice();
}
