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
#include "Framework/CommonServices.h"
#include "Framework/AsyncQueue.h"
#include "Framework/ParallelContext.h"
#include "Framework/ControlService.h"
#include "Framework/DriverClient.h"
#include "Framework/CallbackService.h"
#include "Framework/ServiceSpec.h"
#include "Framework/TimesliceIndex.h"
#include "Framework/DataTakingContext.h"
#include "Framework/DataSender.h"
#include "Framework/ServiceRegistryRef.h"
#include "Framework/DeviceSpec.h"
#include "Framework/LocalRootFileService.h"
#include "Framework/DataRelayer.h"
#include "Framework/Signpost.h"
#include "Framework/DataProcessingStats.h"
#include "Framework/CommonMessageBackends.h"
#include "Framework/DanglingContext.h"
#include "Framework/DataProcessingHelpers.h"
#include "InputRouteHelpers.h"
#include "Framework/EndOfStreamContext.h"
#include "Framework/RawDeviceService.h"
#include "Framework/RunningWorkflowInfo.h"
#include "Framework/Tracing.h"
#include "Framework/Monitoring.h"
#include "Framework/AsyncQueue.h"
#include "Framework/Plugins.h"
#include "Framework/DeviceContext.h"
#include "Framework/DataProcessingContext.h"
#include "Framework/StreamContext.h"
#include "Framework/DeviceState.h"
#include "Framework/DeviceConfig.h"

#include "TextDriverClient.h"
#include "WSDriverClient.h"
#include "HTTPParser.h"
#include "../src/DataProcessingStatus.h"
#include "DecongestionService.h"
#include "ArrowSupport.h"
#include "DPLMonitoringBackend.h"
#include "Headers/STFHeader.h"
#include "Headers/DataHeader.h"

#include <Configuration/ConfigurationInterface.h>
#include <Configuration/ConfigurationFactory.h>
#include <Monitoring/MonitoringFactory.h>

#include <fairmq/Device.h>
#include <fairmq/shmem/Monitor.h>
#include <fairmq/shmem/Common.h>
#include <fairmq/ProgOptions.h>
#include <uv.h>

#include <cstdlib>
#include <cstring>

using o2::configuration::ConfigurationFactory;
using o2::configuration::ConfigurationInterface;
using o2::monitoring::Monitoring;
using o2::monitoring::MonitoringFactory;
using Metric = o2::monitoring::Metric;
using Key = o2::monitoring::tags::Key;
using Value = o2::monitoring::tags::Value;

// This is to allow C++20 aggregate initialisation
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"

namespace o2::framework
{

#define MONITORING_QUEUE_SIZE 100
o2::framework::ServiceSpec CommonServices::monitoringSpec()
{
  return ServiceSpec{
    .name = "monitoring",
    .init = [](ServiceRegistryRef registry, DeviceState&, fair::mq::ProgOptions& options) -> ServiceHandle {
      void* service = nullptr;
      bool isWebsocket = strncmp(options.GetPropertyAsString("driver-client-backend").c_str(), "ws://", 4) == 0;
      bool isDefault = options.GetPropertyAsString("monitoring-backend") == "default";
      bool useDPL = (isWebsocket && isDefault) || options.GetPropertyAsString("monitoring-backend") == "dpl://";
      o2::monitoring::Monitoring* monitoring;
      if (useDPL) {
        monitoring = new Monitoring();
        auto dplBackend = std::make_unique<DPLMonitoringBackend>(registry);
        (dynamic_cast<o2::monitoring::Backend*>(dplBackend.get()))->setVerbosity(o2::monitoring::Verbosity::Debug);
        monitoring->addBackend(std::move(dplBackend));
      } else {
        auto backend = isDefault ? "infologger://" : options.GetPropertyAsString("monitoring-backend");
        monitoring = MonitoringFactory::Get(backend).release();
      }
      service = monitoring;
      monitoring->enableBuffering(MONITORING_QUEUE_SIZE);
      assert(registry.get<DeviceSpec const>().name.empty() == false);
      monitoring->addGlobalTag("pipeline_id", std::to_string(registry.get<DeviceSpec const>().inputTimesliceId));
      monitoring->addGlobalTag("dataprocessor_name", registry.get<DeviceSpec const>().name);
      monitoring->addGlobalTag("dpl_instance", options.GetPropertyAsString("shm-segment-id"));
      return ServiceHandle{TypeIdHelpers::uniqueId<Monitoring>(), service};
    },
    .configure = noConfiguration(),
    .start = [](ServiceRegistryRef services, void* service) {
      auto* monitoring = (o2::monitoring::Monitoring*)service;

      auto extRunNumber = services.get<RawDeviceService>().device()->fConfig->GetProperty<std::string>("runNumber", "unspecified");
      if (extRunNumber == "unspecified") {
        return;
      }
      try {
        monitoring->setRunNumber(std::stoul(extRunNumber));
      } catch (...) {
      } },
    .exit = [](ServiceRegistryRef registry, void* service) {
                       auto* monitoring = reinterpret_cast<Monitoring*>(service);
                       delete monitoring; },
    .kind = ServiceKind::Serial};
}

// An asyncronous service that executes actions in at the end of the data processing
o2::framework::ServiceSpec CommonServices::asyncQueue()
{
  return ServiceSpec{
    .name = "async-queue",
    .init = simpleServiceInit<AsyncQueue, AsyncQueue>(),
    .configure = noConfiguration(),
    .kind = ServiceKind::Serial};
}

// Make it a service so that it can be used easily from the analysis
// FIXME: Moreover, it makes sense that this will be duplicated on a per thread
// basis when we get to it.
o2::framework::ServiceSpec CommonServices::timingInfoSpec()
{
  return ServiceSpec{
    .name = "timing-info",
    .uniqueId = simpleServiceId<TimingInfo>(),
    .init = simpleServiceInit<TimingInfo, TimingInfo, ServiceKind::Stream>(),
    .configure = noConfiguration(),
    .kind = ServiceKind::Stream};
}

o2::framework::ServiceSpec CommonServices::streamContextSpec()
{
  return ServiceSpec{
    .name = "stream-context",
    .uniqueId = simpleServiceId<StreamContext>(),
    .init = simpleServiceInit<StreamContext, StreamContext, ServiceKind::Stream>(),
    .configure = noConfiguration(),
    .kind = ServiceKind::Stream};
}

o2::framework::ServiceSpec CommonServices::datatakingContextSpec()
{
  return ServiceSpec{
    .name = "datataking-contex",
    .uniqueId = simpleServiceId<DataTakingContext>(),
    .init = simpleServiceInit<DataTakingContext, DataTakingContext, ServiceKind::Stream>(),
    .configure = noConfiguration(),
    .preProcessing = [](ProcessingContext& processingContext, void* service) {
      auto& context = processingContext.services().get<DataTakingContext>();
      for (auto const& ref : processingContext.inputs()) {
        const o2::framework::DataProcessingHeader *dph = o2::header::get<DataProcessingHeader*>(ref.header);
        const auto* dh = o2::header::get<o2::header::DataHeader*>(ref.header);
        if (!dph || !dh) {
          continue;
        }
        context.runNumber = fmt::format("{}", dh->runNumber);
        break;
      } },
    // Notice this will be executed only once, because the service is declared upfront.
    .start = [](ServiceRegistryRef services, void* service) {
      auto& context = services.get<DataTakingContext>();
      auto extRunNumber = services.get<RawDeviceService>().device()->fConfig->GetProperty<std::string>("runNumber", "unspecified");
      if (extRunNumber != "unspecified" || context.runNumber == "0") {
        context.runNumber = extRunNumber;
      }
      auto extLHCPeriod = services.get<RawDeviceService>().device()->fConfig->GetProperty<std::string>("lhc_period", "unspecified");
      if (extLHCPeriod != "unspecified") {
        context.lhcPeriod = extLHCPeriod;
      } else {
        static const char* months[12] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
        time_t now = time(nullptr);
        auto ltm = gmtime(&now);
        context.lhcPeriod = months[ltm->tm_mon];
        LOG(info) << "LHCPeriod is not available, using current month " << context.lhcPeriod;
      }

      auto extRunType = services.get<RawDeviceService>().device()->fConfig->GetProperty<std::string>("run_type", "unspecified");
      if (extRunType != "unspecified") {
        context.runType = extRunType;
      }
      auto extEnvId = services.get<RawDeviceService>().device()->fConfig->GetProperty<std::string>("environment_id", "unspecified");
      if (extEnvId != "unspecified") {
        context.envId = extEnvId;
      }
      auto extDetectors = services.get<RawDeviceService>().device()->fConfig->GetProperty<std::string>("detectors", "unspecified");
      if (extDetectors != "unspecified") {
        context.detectors = extDetectors;
      }
      auto forcedRaw = services.get<RawDeviceService>().device()->fConfig->GetProperty<std::string>("force_run_as_raw", "false");
      context.forcedRaw = forcedRaw == "true";

      context.nOrbitsPerTF = services.get<RawDeviceService>().device()->fConfig->GetProperty<uint64_t>("Norbits_per_TF", 128); },
    .kind = ServiceKind::Stream};
}

struct MissingService {
};

o2::framework::ServiceSpec CommonServices::configurationSpec()
{
  return ServiceSpec{
    .name = "configuration",
    .init = [](ServiceRegistryRef services, DeviceState&, fair::mq::ProgOptions& options) -> ServiceHandle {
      auto backend = options.GetPropertyAsString("configuration");
      if (backend == "command-line") {
        return ServiceHandle{0, nullptr};
      }
      return ServiceHandle{TypeIdHelpers::uniqueId<ConfigurationInterface>(),
                           ConfigurationFactory::getConfiguration(backend).release()};
    },
    .configure = noConfiguration(),
    .driverStartup = [](ServiceRegistryRef registry, DeviceConfig const& dc) {
      if (dc.options.count("configuration") == 0) {
        registry.registerService(ServiceHandle{0, nullptr});
        return;
      }
      auto backend = dc.options["configuration"].as<std::string>();
      registry.registerService(ServiceHandle{TypeIdHelpers::uniqueId<ConfigurationInterface>(),
                                             ConfigurationFactory::getConfiguration(backend).release()}); },
    .kind = ServiceKind::Global};
}

o2::framework::ServiceSpec CommonServices::driverClientSpec()
{
  return ServiceSpec{
    .name = "driverClient",
    .init = [](ServiceRegistryRef services, DeviceState& state, fair::mq::ProgOptions& options) -> ServiceHandle {
      auto backend = options.GetPropertyAsString("driver-client-backend");
      if (backend == "stdout://") {
        return ServiceHandle{TypeIdHelpers::uniqueId<DriverClient>(),
                             new TextDriverClient(services, state)};
      }
      auto [ip, port] = o2::framework::parse_websocket_url(backend.c_str());
      return ServiceHandle{TypeIdHelpers::uniqueId<DriverClient>(),
                           new WSDriverClient(services, state, ip.c_str(), port)};
    },
    .configure = noConfiguration(),
    .kind = ServiceKind::Global};
}

o2::framework::ServiceSpec CommonServices::controlSpec()
{
  return ServiceSpec{
    .name = "control",
    .init = [](ServiceRegistryRef services, DeviceState& state, fair::mq::ProgOptions& options) -> ServiceHandle {
      return ServiceHandle{TypeIdHelpers::uniqueId<ControlService>(),
                           new ControlService(services, state)};
    },
    .configure = noConfiguration(),
    .kind = ServiceKind::Serial};
}

o2::framework::ServiceSpec CommonServices::rootFileSpec()
{
  return ServiceSpec{
    .name = "localrootfile",
    .init = simpleServiceInit<LocalRootFileService, LocalRootFileService>(),
    .configure = noConfiguration(),
    .kind = ServiceKind::Serial};
}

o2::framework::ServiceSpec CommonServices::parallelSpec()
{
  return ServiceSpec{
    .name = "parallel",
    .init = [](ServiceRegistryRef services, DeviceState&, fair::mq::ProgOptions& options) -> ServiceHandle {
      auto& spec = services.get<DeviceSpec const>();
      return ServiceHandle{TypeIdHelpers::uniqueId<ParallelContext>(),
                           new ParallelContext(spec.rank, spec.nSlots)};
    },
    .configure = noConfiguration(),
    .kind = ServiceKind::Serial};
}

o2::framework::ServiceSpec CommonServices::timesliceIndex()
{
  return ServiceSpec{
    .name = "timesliceindex",
    .init = [](ServiceRegistryRef services, DeviceState& state, fair::mq::ProgOptions& options) -> ServiceHandle {
      auto& spec = services.get<DeviceSpec const>();
      return ServiceHandle{TypeIdHelpers::uniqueId<TimesliceIndex>(),
                           new TimesliceIndex(InputRouteHelpers::maxLanes(spec.inputs), state.inputChannelInfos)};
    },
    .configure = noConfiguration(),
    .kind = ServiceKind::Serial};
}

o2::framework::ServiceSpec CommonServices::callbacksSpec()
{
  return ServiceSpec{
    .name = "callbacks",
    .init = simpleServiceInit<CallbackService, CallbackService>(),
    .configure = noConfiguration(),
    .kind = ServiceKind::Serial};
}

o2::framework::ServiceSpec CommonServices::dataRelayer()
{
  return ServiceSpec{
    .name = "datarelayer",
    .init = [](ServiceRegistryRef services, DeviceState&, fair::mq::ProgOptions& options) -> ServiceHandle {
      auto& spec = services.get<DeviceSpec const>();
      return ServiceHandle{TypeIdHelpers::uniqueId<DataRelayer>(),
                           new DataRelayer(spec.completionPolicy,
                                           spec.inputs,
                                           services.get<Monitoring>(),
                                           services.get<TimesliceIndex>())};
    },
    .configure = noConfiguration(),
    .kind = ServiceKind::Serial};
}

o2::framework::ServiceSpec CommonServices::dataSender()
{
  return ServiceSpec{
    .name = "datasender",
    .init = [](ServiceRegistryRef services, DeviceState&, fair::mq::ProgOptions& options) -> ServiceHandle {
      auto& spec = services.get<DeviceSpec const>();
      return ServiceHandle{TypeIdHelpers::uniqueId<DataSender>(),
                           new DataSender(services, spec.sendingPolicy)};
    },
    .configure = noConfiguration(),
    .preProcessing = [](ProcessingContext&, void* service) {
      auto& dataSender = *reinterpret_cast<DataSender*>(service);
      dataSender.reset(); },
    .postDispatching = [](ProcessingContext& ctx, void* service) {
      auto& dataSender = *reinterpret_cast<DataSender*>(service);
      // If the quit was requested, the post dispatching can still happen
      // but with an empty set of data.
      if (ctx.services().get<DeviceState>().quitRequested == false) {
        dataSender.verifyMissingSporadic();
      } },
    .kind = ServiceKind::Serial};
}

struct TracingInfrastructure {
  int processingCount;
};

o2::framework::ServiceSpec CommonServices::tracingSpec()
{
  return ServiceSpec{
    .name = "tracing",
    .init = [](ServiceRegistryRef, DeviceState&, fair::mq::ProgOptions&) -> ServiceHandle {
      return ServiceHandle{.hash = TypeIdHelpers::uniqueId<TracingInfrastructure>(),
                           .instance = new TracingInfrastructure(),
                           .kind = ServiceKind::Serial};
    },
    .configure = noConfiguration(),
    .preProcessing = [](ProcessingContext&, void* service) {
      auto* t = reinterpret_cast<TracingInfrastructure*>(service);
      t->processingCount += 1; },
    .postProcessing = [](ProcessingContext&, void* service) {
      auto* t = reinterpret_cast<TracingInfrastructure*>(service);
      t->processingCount += 1; },
    .kind = ServiceKind::Serial};
}

struct CCDBSupport {
};

// CCDB Support service
o2::framework::ServiceSpec CommonServices::ccdbSupportSpec()
{
  return ServiceSpec{
    .name = "ccdb-support",
    .init = [](ServiceRegistryRef services, DeviceState&, fair::mq::ProgOptions&) -> ServiceHandle {
      // iterate on all the outputs matchers
      auto& spec = services.get<DeviceSpec const>();
      for (auto& output : spec.outputs) {
        if (DataSpecUtils::match(output.matcher, ConcreteDataTypeMatcher{"FLP", "DISTSUBTIMEFRAME"})) {
          LOGP(debug, "Optional inputs support enabled");
          return ServiceHandle{.hash = TypeIdHelpers::uniqueId<CCDBSupport>(), .instance = new CCDBSupport, .kind = ServiceKind::Serial};
        }
      }
      return ServiceHandle{.hash = TypeIdHelpers::uniqueId<CCDBSupport>(), .instance = nullptr, .kind = ServiceKind::Serial};
    },
    .configure = noConfiguration(),
    .postProcessing = [](ProcessingContext& pc, void* service) {
      if (!service) {
        return;
      }
      if (pc.services().get<DeviceState>().streaming == StreamingState::EndOfStreaming) {
        if (pc.outputs().countDeviceOutputs(true) == 0) {
          LOGP(debug, "We are in EoS w/o outputs, do not automatically add DISTSUBTIMEFRAME to outgoing messages");
          return;
        }
      }
      const auto ref = pc.inputs().getFirstValid(true);
      const auto* dh = DataRefUtils::getHeader<o2::header::DataHeader*>(ref);
      const auto* dph = DataRefUtils::getHeader<DataProcessingHeader*>(ref);

      // For any output that is a FLP/DISTSUBTIMEFRAME with subspec != 0,
      // we create a new message.
      InputSpec matcher{"matcher", ConcreteDataTypeMatcher{"FLP", "DISTSUBTIMEFRAME"}};
      for (auto& output : pc.services().get<DeviceSpec const>().outputs) {
        if ((output.timeslice % output.maxTimeslices) != 0) {
          continue;
        }
        if (DataSpecUtils::match(output.matcher, ConcreteDataTypeMatcher{"FLP", "DISTSUBTIMEFRAME"})) {
          auto concrete = DataSpecUtils::asConcreteDataMatcher(output.matcher);
          if (concrete.subSpec == 0) {
            continue;
          }
          auto& stfDist = pc.outputs().make<o2::header::STFHeader>(Output{concrete.origin, concrete.description, concrete.subSpec, output.matcher.lifetime});
          stfDist.id = dph->startTime;
          stfDist.firstOrbit = dh->firstTForbit;
          stfDist.runNumber = dh->runNumber;
        }
      } },
    .kind = ServiceKind::Global};
}

// Decongestion service
// If we do not have any Timeframe input, it means we must be creating timeslices
// in order and that we should propagate the oldest possible timeslice at the end
// of each processing step.
o2::framework::ServiceSpec CommonServices::decongestionSpec()
{
  return ServiceSpec{
    .name = "decongestion",
    .init = [](ServiceRegistryRef services, DeviceState&, fair::mq::ProgOptions& options) -> ServiceHandle {
      DecongestionService* decongestion = new DecongestionService();
      for (auto& input : services.get<DeviceSpec const>().inputs) {
        if (input.matcher.lifetime == Lifetime::Timeframe) {
          LOGP(detail, "Found a Timeframe input, we cannot update the oldest possible timeslice");
          decongestion->isFirstInTopology = false;
          break;
        }
      }
      auto& queue = services.get<AsyncQueue>();
      decongestion->oldestPossibleTimesliceTask = AsyncQueueHelpers::create(queue, {"oldest-possible-timeslice", 100});
      return ServiceHandle{TypeIdHelpers::uniqueId<DecongestionService>(), decongestion, ServiceKind::Serial};
    },
    .postForwarding = [](ProcessingContext& ctx, void* service) {
      DecongestionService* decongestion = reinterpret_cast<DecongestionService*>(service);
      if (decongestion->isFirstInTopology == false) {
        LOGP(debug, "We are not the first in the topology, do not update the oldest possible timeslice");
        return;
      }
      auto& timesliceIndex = ctx.services().get<TimesliceIndex>();
      auto& relayer = ctx.services().get<DataRelayer>();
      timesliceIndex.updateOldestPossibleOutput();
      auto& proxy = ctx.services().get<FairMQDeviceProxy>();
      auto oldestPossibleOutput = relayer.getOldestPossibleOutput();
      if (decongestion->lastTimeslice && oldestPossibleOutput.timeslice.value == decongestion->lastTimeslice) {
        LOGP(debug, "Not sending already sent value");
        return;
      }
      if (oldestPossibleOutput.timeslice.value < decongestion->lastTimeslice) {
        LOGP(error, "We are trying to send an oldest possible timeslice {} that is older than the last one we already sent {}",
             oldestPossibleOutput.timeslice.value, decongestion->lastTimeslice);
        return;
      }

      LOGP(debug, "Broadcasting possible output {} due to {} ({})", oldestPossibleOutput.timeslice.value,
           oldestPossibleOutput.slot.index == -1 ? "channel" : "slot",
           oldestPossibleOutput.slot.index == -1 ? oldestPossibleOutput.channel.value: oldestPossibleOutput.slot.index);
      DataProcessingHelpers::broadcastOldestPossibleTimeslice(proxy, oldestPossibleOutput.timeslice.value);

      for (int fi = 0; fi < proxy.getNumForwardChannels(); fi++) {
        auto& info = proxy.getForwardChannelInfo(ChannelIndex{fi});
        auto& state = proxy.getForwardChannelState(ChannelIndex{fi});
        // TODO: this we could cache in the proxy at the bind moment.
        if (info.channelType != ChannelAccountingType::DPL) {
          LOG(debug) << "Skipping channel";
          continue;
        }
        if (DataProcessingHelpers::sendOldestPossibleTimeframe(info, state, oldestPossibleOutput.timeslice.value)) {
          LOGP(debug, "Forwarding to channel {} oldest possible timeslice {}, prio 20", info.name, oldestPossibleOutput.timeslice.value);
        }
      }
      decongestion->lastTimeslice = oldestPossibleOutput.timeslice.value; },
    .domainInfoUpdated = [](ServiceRegistryRef services, size_t oldestPossibleTimeslice, ChannelIndex channel) {
      DecongestionService& decongestion = services.get<DecongestionService>();
      auto& relayer = services.get<DataRelayer>();
      auto& timesliceIndex = services.get<TimesliceIndex>();
      auto& proxy = services.get<FairMQDeviceProxy>();
      LOGP(debug, "Received oldest possible timeframe {} from channel {}", oldestPossibleTimeslice, channel.value);
      relayer.setOldestPossibleInput({oldestPossibleTimeslice}, channel);
      timesliceIndex.updateOldestPossibleOutput();
      auto oldestPossibleOutput = relayer.getOldestPossibleOutput();

      if (oldestPossibleOutput.timeslice.value == decongestion.lastTimeslice) {
        LOGP(debug, "Not sending already sent value");
        return;
      }
      if (oldestPossibleOutput.timeslice.value < decongestion.lastTimeslice) {
        LOGP(error, "We are trying to send an oldest possible timeslice {} that is older than the last one we sent {}",
             oldestPossibleOutput.timeslice.value, decongestion.lastTimeslice);
        return;
      }
      LOGP(debug, "Broadcasting possible output {}", oldestPossibleOutput.timeslice.value);
      auto &queue = services.get<AsyncQueue>();
      auto& spec = services.get<DeviceSpec const>();
      auto *device = services.get<RawDeviceService>().device();
      /// We use the oldest possible timeslice to debuounce, so that only the latest one
      /// at the end of one iteration is sent.
      LOGP(debug, "Queueing oldest possible timeslice {} propagation for execution.", oldestPossibleOutput.timeslice.value);
      AsyncQueueHelpers::post(
        queue, decongestion.oldestPossibleTimesliceTask, [oldestPossibleOutput, &decongestion, &proxy, &spec, device, &timesliceIndex]() {
          if (decongestion.lastTimeslice >= oldestPossibleOutput.timeslice.value) {
            LOGP(debug, "Not sending already sent value {} >= {}", decongestion.lastTimeslice, oldestPossibleOutput.timeslice.value);
            return;
          }
          LOGP(debug, "Running oldest possible timeslice {} propagation.", oldestPossibleOutput.timeslice.value);
          DataProcessingHelpers::broadcastOldestPossibleTimeslice(proxy, oldestPossibleOutput.timeslice.value);

          for (int fi = 0; fi < proxy.getNumForwardChannels(); fi++) {
            auto& info = proxy.getForwardChannelInfo(ChannelIndex{fi});
            auto& state = proxy.getForwardChannelState(ChannelIndex{fi});
            // TODO: this we could cache in the proxy at the bind moment.
            if (info.channelType != ChannelAccountingType::DPL) {
              LOG(debug) << "Skipping channel";
              continue;
            }
            if (DataProcessingHelpers::sendOldestPossibleTimeframe(info, state, oldestPossibleOutput.timeslice.value)) {
              LOGP(debug, "Forwarding to channel {} oldest possible timeslice {}, prio 20", info.name, oldestPossibleOutput.timeslice.value);
            }
          }
          decongestion.lastTimeslice = oldestPossibleOutput.timeslice.value;
        },
        TimesliceId{oldestPossibleTimeslice}, -1); },
    .kind = ServiceKind::Serial};
}

// FIXME: allow configuring the default number of threads per device
//        This should probably be done by overriding the preFork
//        callback and using the boost program options there to
//        get the default number of threads.
o2::framework::ServiceSpec CommonServices::threadPool(int numWorkers)
{
  return ServiceSpec{
    .name = "threadpool",
    .init = [](ServiceRegistryRef services, DeviceState&, fair::mq::ProgOptions& options) -> ServiceHandle {
      ThreadPool* pool = new ThreadPool();
      // FIXME: this will require some extra argument for the configuration context of a service
      pool->poolSize = 1;
      return ServiceHandle{TypeIdHelpers::uniqueId<ThreadPool>(), pool};
    },
    .configure = [](InitContext&, void* service) -> void* {
      ThreadPool* t = reinterpret_cast<ThreadPool*>(service);
      // FIXME: this will require some extra argument for the configuration context of a service
      t->poolSize = 1;
      return service;
    },
    .postForkParent = [](ServiceRegistryRef services) -> void {
      // FIXME: this will require some extra argument for the configuration context of a service
      auto numWorkersS = std::to_string(1);
      setenv("UV_THREADPOOL_SIZE", numWorkersS.c_str(), 0);
    },
    .kind = ServiceKind::Serial};
}

namespace
{
auto sendRelayerMetrics(ServiceRegistryRef registry, DataProcessingStats& stats) -> void
{
  // Derive the amount of shared memory used
  auto& runningWorkflow = registry.get<RunningWorkflowInfo const>();
  using namespace fair::mq::shmem;
  auto& spec = registry.get<DeviceSpec const>();

  // FIXME: Ugly, but we do it only every 5 seconds...
  if (spec.name == "readout-proxy") {
    auto device = registry.get<RawDeviceService>().device();
    long freeMemory = -1;
    try {
      freeMemory = fair::mq::shmem::Monitor::GetFreeMemory(ShmId{makeShmIdStr(device->fConfig->GetProperty<uint64_t>("shmid"))}, runningWorkflow.shmSegmentId);
    } catch (...) {
    }
    if (freeMemory == -1) {
      try {
        freeMemory = fair::mq::shmem::Monitor::GetFreeMemory(SessionId{device->fConfig->GetProperty<std::string>("session")}, runningWorkflow.shmSegmentId);
      } catch (...) {
      }
    }
    stats.updateStats({static_cast<short>(static_cast<short>(ProcessingStatsId::AVAILABLE_MANAGED_SHM_BASE) + runningWorkflow.shmSegmentId), DataProcessingStats::Op::SetIfPositive, freeMemory});
  }

  ZoneScopedN("send metrics");
  auto& relayerStats = registry.get<DataRelayer>().getStats();
  auto& monitoring = registry.get<Monitoring>();

  O2_SIGNPOST_START(MonitoringStatus::ID, MonitoringStatus::SEND, 0, 0, O2_SIGNPOST_BLUE);

  monitoring.send(Metric{(int)relayerStats.malformedInputs, "malformed_inputs"}.addTag(Key::Subsystem, Value::DPL));
  monitoring.send(Metric{(int)relayerStats.droppedComputations, "dropped_computations"}.addTag(Key::Subsystem, Value::DPL));
  monitoring.send(Metric{(int)relayerStats.droppedIncomingMessages, "dropped_incoming_messages"}.addTag(Key::Subsystem, Value::DPL));
  monitoring.send(Metric{(int)relayerStats.relayedMessages, "relayed_messages"}.addTag(Key::Subsystem, Value::DPL));

  O2_SIGNPOST_END(MonitoringStatus::ID, MonitoringStatus::SEND, 0, 0, O2_SIGNPOST_BLUE);

  auto device = registry.get<RawDeviceService>().device();

  int64_t totalBytesIn = 0;
  int64_t totalBytesOut = 0;

  for (auto& channel : device->fChannels) {
    totalBytesIn += channel.second[0].GetBytesRx();
    totalBytesOut += channel.second[0].GetBytesTx();
  }

  stats.updateStats({static_cast<short>(ProcessingStatsId::TOTAL_BYTES_IN), DataProcessingStats::Op::CumulativeRate, totalBytesIn});
  stats.updateStats({static_cast<short>(ProcessingStatsId::TOTAL_BYTES_OUT), DataProcessingStats::Op::CumulativeRate, totalBytesOut});
};

/// This will flush metrics only once every second.
auto flushMetrics(ServiceRegistryRef registry, DataProcessingStats& stats) -> void
{
  ZoneScopedN("flush metrics");
  auto& monitoring = registry.get<Monitoring>();
  auto& relayer = registry.get<DataRelayer>();

  O2_SIGNPOST_START(MonitoringStatus::ID, MonitoringStatus::FLUSH, 0, 0, O2_SIGNPOST_RED);
  // Send all the relevant metrics for the relayer to update the GUI
  stats.flushChangedMetrics([&monitoring](std::string const& name, int64_t timestamp, uint64_t value) mutable -> void {
    // convert timestamp to a time_point
    auto tp = std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds>(std::chrono::milliseconds(timestamp));
    auto metric = o2::monitoring::Metric{name, Metric::DefaultVerbosity, tp};
    metric.addValue((int)value, name);
    monitoring.send(std::move(metric));
  });
  relayer.sendContextState();
  monitoring.flushBuffer();
  O2_SIGNPOST_END(MonitoringStatus::ID, MonitoringStatus::FLUSH, 0, 0, O2_SIGNPOST_RED);
};
} // namespace

o2::framework::ServiceSpec CommonServices::dataProcessingStats()
{
  return ServiceSpec{
    .name = "data-processing-stats",
    .init = [](ServiceRegistryRef services, DeviceState& state, fair::mq::ProgOptions& options) -> ServiceHandle {
      auto* stats = new DataProcessingStats(DataProcessingStatsHelpers::defaultRealtimeBaseConfigurator(state.loop),
                                            DataProcessingStatsHelpers::defaultCPUTimeConfigurator());
      auto& runningWorkflow = services.get<RunningWorkflowInfo const>();

      // It makes no sense to update the stats more often than every 5s
      int quickUpdateInterval = 5000;
      uint64_t quickRefreshInterval = 7000;
      stats->registerMetric({.name = "errors", .metricId = (int)ProcessingStatsId::ERROR_COUNT, .defaultValue = 0, .minPublishInterval = quickUpdateInterval, .maxRefreshLatency = quickRefreshInterval});
      stats->registerMetric({"exceptions", (int)ProcessingStatsId::EXCEPTION_COUNT, 0, quickUpdateInterval});
      stats->registerMetric({"inputs/relayed/pending", (int)ProcessingStatsId::PENDING_INPUTS, 0, quickUpdateInterval});
      stats->registerMetric({"inputs/relayed/incomplete", (int)ProcessingStatsId::INCOMPLETE_INPUTS, 0, quickUpdateInterval});
      stats->registerMetric({"inputs/relayed/total", (int)ProcessingStatsId::TOTAL_INPUTS, 0, quickUpdateInterval});
      stats->registerMetric({"elapsed_time_ms", (int)ProcessingStatsId::LAST_ELAPSED_TIME_MS, 0, quickUpdateInterval});
      stats->registerMetric({"last_processed_input_size_byte", (int)ProcessingStatsId::LAST_PROCESSED_SIZE, 0, quickUpdateInterval});
      stats->registerMetric({"total_processed_input_size_byte", (int)ProcessingStatsId::TOTAL_PROCESSED_SIZE, 0, quickUpdateInterval});
      stats->registerMetric({"total_sigusr1", (int)ProcessingStatsId::TOTAL_SIGUSR1, 0, quickUpdateInterval});
      stats->registerMetric({"processing_rate_mb_s", (int)ProcessingStatsId::PROCESSING_RATE_MB_S, 0, quickUpdateInterval});
      stats->registerMetric({"min_input_latency_ms", (int)ProcessingStatsId::LAST_MIN_LATENCY, 0, quickUpdateInterval});
      stats->registerMetric({"max_input_latency_ms", (int)ProcessingStatsId::LAST_MAX_LATENCY, 0, quickUpdateInterval});
      stats->registerMetric({"input_rate_mb_s", (int)ProcessingStatsId::INPUT_RATE_MB_S, 0, quickUpdateInterval});
      stats->registerMetric({"processing_rate_hz", (int)ProcessingStatsId::PROCESSING_RATE_HZ, 0, quickUpdateInterval});
      stats->registerMetric({"performed_computations", (int)ProcessingStatsId::PERFORMED_COMPUTATIONS, 0, quickUpdateInterval});
      stats->registerMetric({"total_bytes_in", (int)ProcessingStatsId::TOTAL_BYTES_IN, 0, quickUpdateInterval});
      stats->registerMetric({"total_bytes_out", (int)ProcessingStatsId::TOTAL_BYTES_OUT, 0, quickUpdateInterval});
      stats->registerMetric({fmt::format("available_managed_shm_{}", runningWorkflow.shmSegmentId), (int)ProcessingStatsId::AVAILABLE_MANAGED_SHM_BASE + runningWorkflow.shmSegmentId, 500});
      auto &relayer = services.get<DataRelayer>();

      for (size_t i = 0; i < relayer.getCacheSize(); ++i) {
        stats->registerMetric({fmt::format("data_relayer/{}", i), static_cast<int>((int)ProcessingStatsId::RELAYER_METRIC_BASE + i), 0, 100});
      }
      return ServiceHandle{TypeIdHelpers::uniqueId<DataProcessingStats>(), stats};
    },
    .configure = noConfiguration(),
    .postProcessing = [](ProcessingContext& context, void* service) {
      auto* stats = (DataProcessingStats*)service;
      stats->updateStats({(short)ProcessingStatsId::PERFORMED_COMPUTATIONS, DataProcessingStats::Op::Add, 1}); },
    .preDangling = [](DanglingContext& context, void* service) {
       auto* stats = (DataProcessingStats*)service;
       sendRelayerMetrics(context.services(), *stats);
       flushMetrics(context.services(), *stats); },
    .postDangling = [](DanglingContext& context, void* service) {
       auto* stats = (DataProcessingStats*)service;
       sendRelayerMetrics(context.services(), *stats);
       flushMetrics(context.services(), *stats); },
    .preEOS = [](EndOfStreamContext& context, void* service) {
      auto* stats = (DataProcessingStats*)service;
      sendRelayerMetrics(context.services(), *stats);
      flushMetrics(context.services(), *stats); },
    .kind = ServiceKind::Serial};
}

struct GUIMetrics {
};

o2::framework::ServiceSpec CommonServices::guiMetricsSpec()
{
  return ServiceSpec{
    .name = "gui-metrics",
    .init = [](ServiceRegistryRef services, DeviceState&, fair::mq::ProgOptions& options) -> ServiceHandle {
      auto* stats = new GUIMetrics();
      auto& monitoring = services.get<Monitoring>();
      auto& spec = services.get<DeviceSpec const>();
      monitoring.send({(int)spec.inputChannels.size(), fmt::format("oldest_possible_timeslice/h"), o2::monitoring::Verbosity::Debug});
      monitoring.send({(int)1, fmt::format("oldest_possible_timeslice/w"), o2::monitoring::Verbosity::Debug});
      monitoring.send({(int)spec.outputChannels.size(), fmt::format("oldest_possible_output/h"), o2::monitoring::Verbosity::Debug});
      monitoring.send({(int)1, fmt::format("oldest_possible_output/w"), o2::monitoring::Verbosity::Debug});
      return ServiceHandle{TypeIdHelpers::uniqueId<GUIMetrics>(), stats};
    },
    .configure = noConfiguration(),
    .postProcessing = [](ProcessingContext& context, void* service) {
      auto& relayer = context.services().get<DataRelayer>();
      auto& monitoring = context.services().get<Monitoring>();
      auto& spec = context.services().get<DeviceSpec const>();
      auto oldestPossibleOutput = relayer.getOldestPossibleOutput();
      for (size_t ci; ci < spec.outputChannels.size(); ++ci) {
        monitoring.send({(uint64_t)oldestPossibleOutput.timeslice.value, fmt::format("oldest_possible_output/{}", ci), o2::monitoring::Verbosity::Debug});
      } },
    .domainInfoUpdated = [](ServiceRegistryRef registry, size_t timeslice, ChannelIndex channel) {
      auto& monitoring = registry.get<Monitoring>();
      monitoring.send({(uint64_t)timeslice, fmt::format("oldest_possible_timeslice/{}", channel.value), o2::monitoring::Verbosity::Debug}); },
    .active = false,
    .kind = ServiceKind::Serial};
}

o2::framework::ServiceSpec CommonServices::objectCache()
{
  return ServiceSpec{
    .name = "object-cache",
    .init = [](ServiceRegistryRef, DeviceState&, fair::mq::ProgOptions&) -> ServiceHandle {
      auto* cache = new ObjectCache();
      return ServiceHandle{TypeIdHelpers::uniqueId<ObjectCache>(), cache};
    },
    .configure = noConfiguration(),
    .kind = ServiceKind::Serial};
}

o2::framework::ServiceSpec CommonServices::dataProcessorContextSpec()
{
  return ServiceSpec{
    .name = "data-processing-context",
    .init = [](ServiceRegistryRef, DeviceState&, fair::mq::ProgOptions&) -> ServiceHandle {
      return ServiceHandle{TypeIdHelpers::uniqueId<DataProcessorContext>(), new DataProcessorContext()};
    },
    .configure = noConfiguration(),
    .exit = [](ServiceRegistryRef, void* service) { auto* context = (DataProcessorContext*)service; delete context; },
    .kind = ServiceKind::Serial};
}

o2::framework::ServiceSpec CommonServices::deviceContextSpec()
{
  return ServiceSpec{
    .name = "device-context",
    .init = [](ServiceRegistryRef, DeviceState&, fair::mq::ProgOptions&) -> ServiceHandle {
      return ServiceHandle{TypeIdHelpers::uniqueId<DeviceContext>(), new DeviceContext()};
    },
    .configure = noConfiguration(),
    .kind = ServiceKind::Serial};
}

o2::framework::ServiceSpec CommonServices::dataAllocatorSpec()
{
  return ServiceSpec{
    .name = "data-allocator",
    .uniqueId = simpleServiceId<DataAllocator>(),
    .init = [](ServiceRegistryRef ref, DeviceState&, fair::mq::ProgOptions&) -> ServiceHandle {
      return ServiceHandle{
        .hash = TypeIdHelpers::uniqueId<DataAllocator>(),
        .instance = new DataAllocator(ref),
        .kind = ServiceKind::Stream,
        .name = "data-allocator",
      };
    },
    .configure = noConfiguration(),
    .kind = ServiceKind::Stream};
}

/// Split a string into a vector of strings using : as a separator.
std::vector<ServiceSpec> CommonServices::defaultServices(int numThreads)
{
  std::vector<ServiceSpec> specs{
    dataProcessorContextSpec(),
    streamContextSpec(),
    dataAllocatorSpec(),
    asyncQueue(),
    timingInfoSpec(),
    timesliceIndex(),
    driverClientSpec(),
    datatakingContextSpec(),
    monitoringSpec(),
    configurationSpec(),
    controlSpec(),
    rootFileSpec(),
    parallelSpec(),
    callbacksSpec(),
    dataRelayer(),
    CommonMessageBackends::fairMQDeviceProxy(),
    dataSender(),
    dataProcessingStats(),
    objectCache(),
    ccdbSupportSpec(),
    CommonMessageBackends::fairMQBackendSpec(),
    ArrowSupport::arrowBackendSpec(),
    CommonMessageBackends::stringBackendSpec(),
    decongestionSpec()};

  std::string loadableServicesStr;
  // Do not load InfoLogger by default if we are not at P2.
  if (getenv("DDS_SESSION_ID") != nullptr || getenv("OCC_CONTROL_PORT") != nullptr) {
    loadableServicesStr += "O2FrameworkDataTakingSupport:InfoLoggerContext,O2FrameworkDataTakingSupport:InfoLogger";
  }
  // Load plugins depending on the environment
  std::vector<LoadableService> loadableServices = {};
  char* loadableServicesEnv = getenv("DPL_LOAD_SERVICES");
  // String to define the services to load is:
  //
  // library1:name1,library2:name2,...
  if (loadableServicesEnv) {
    if (loadableServicesStr.empty() == false) {
      loadableServicesStr += ",";
    }
    loadableServicesStr += loadableServicesEnv;
  }
  loadableServices = ServiceHelpers::parseServiceSpecString(loadableServicesStr.c_str());
  ServiceHelpers::loadFromPlugin(loadableServices, specs);
  // I should make it optional depending wether the GUI is there or not...
  specs.push_back(CommonServices::guiMetricsSpec());
  if (numThreads) {
    specs.push_back(threadPool(numThreads));
  }
  return specs;
}

} // namespace o2::framework
#pragma GCC diagnostic pop
