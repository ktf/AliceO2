//-*- Mode: C++ -*-

#include "Headers/DataHeader.h"
#include <chrono>

// https://lhc-machine-outreach.web.cern.ch/lhc-machine-outreach/collisions.htm
// https://www.lhc-closer.es/taking_a_closer_look_at_lhc/0.buckets_and_bunches

namespace LHCClockParameter {
  // number of bunches and the 40 MHz clock with 25 ns bunch spacing
  // gives revolution time of 89.1 us and 11.223345 kHz
  // this depends on the assumption that the particles are moving effectively
  // at speed of light. There are also documents specifying the orbit time
  // to 89.4 us
  // Note: avoid to define the revolution frequency and use the integral numbers
  // for bunch places and bunch spacing in nano seconds
  // TODO: this eventually needs to be configurable
  static const int gNumberOfBunches = 3564;
  static const int gBunchSpacingNanoSec = 25;
  static const int gOrbitTimeNanoSec = std::ratio<gNumberOfBunches*gBunchSpacingNanoSec>::num;

  // the type of the clock tick depends on whether to use also the bunches
  // as substructure of the orbit.
  // a trait class to extrat the properties of the clock, namely the type
  // of the tick and the period
  template <bool BunchPrecision>
  struct Property {
    typedef uint32_t rep;
    // avoid rounding errors by using the integral numbers in the std::ratio
    // template to define the period
    typedef std::ratio_multiply<std::ratio<gOrbitTimeNanoSec>, std::nano> period;
  };
  template <>
  struct Property<true> {
    typedef uint64_t rep;
    // this is effectively the LHC clock and the ratio is the
    // bunch spacing
    typedef std::ratio_multiply<std::ratio<gBunchSpacingNanoSec>, std::nano> period;
  };
};

// a chrono clock implementation
// - always relative to run start
// - need run start to calculate the epoch
// - based on revolution frequency and number of bunches
template <typename RefTimePoint, bool BunchPrecision = false>
class LHCClock {
public:
  LHCClock(const RefTimePoint& start) : mReference(start) {}
  ~LHCClock() {}
  LHCClock(const LHCClock&) = default;
  LHCClock& operator=(const LHCClock&) = default;

  typedef typename LHCClockParameter::Property<BunchPrecision>::rep    rep;
  typedef typename LHCClockParameter::Property<BunchPrecision>::period period;
  typedef std::chrono::duration<rep, period> duration;
  typedef std::chrono::time_point<LHCClock>  time_point;
  static const bool is_steady =              true;

  /// the now() function is the main characteristics of the clock
  /// calculate now from the system clock and the reference start time
  time_point now() noexcept {
    // tp1 - tp2 results in a duration, we use to create a time_point with characteristics
    // of the clock.
    return time_point(std::chrono::duration_cast<duration>(std::chrono::system_clock::now()) - mReference);
  }

private:
  /// forbidden, always need a reference
  LHCClock() {};

  /// external reference: start time of the run
  RefTimePoint mReference;
};

class TimeStamp
{
 public:
  TimeStamp();
  ~TimeStamp();

  typedef AliceO2::Header::Descriptor<2> TimeUnitID;

  static TimeUnitID const gClockLHC;
  static TimeUnitID const gMicroSeconds;

  // TODO: find out whether the in-class initialization is possible
  //static TimeUnitID const gMicroSeconds = TimeUnitID("USEC");

  template <typename TimeUnit>
  std::chrono::time_point<TimeUnit> get() const;

  template <typename TimeUnit>
  operator std::chrono::time_point<TimeUnit>() const {return get<TimeUnit>();}

 private:
  TimeUnitID mUnit;
  // the unions are probably not a good idea as the members have too different
  // meaning depending on the unit, but take it as a fist working assumption
  union {
    uint16_t mBCNumber;
    uint16_t mSubTicks;
  };
  union {
    uint32_t mPeriod;
    uint32_t mTicks;
  };
};
