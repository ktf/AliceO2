//-*- Mode: C++ -*-

#include "Headers/DataHeader.h"
#include <chrono>

// a chrono clock implementation
// - always relative to run start
// - need run start to calculate the epoch
// - based on revolution frequency and number of bunches
template <typename RefTimePoint, bool BunchPrecision = false>
class LHCClock {
public:
  LHCClock(const RefTimePoint& start);
  ~LHCClock();
  LHCClock(const LHCClock&) = default;
  LHCClock& operator=(const LHCClock&) = default;

  // number of bunches and the 40 MHz clock with 25 ns bunch spacing
  // gives revolution time of 89.1 us and 11.223345 kHz
  static const int gRevolutionFrequency = 11223;
  static const int gNumberOfBunches = 3564;

  // the type of the clock tick depends on whether to use also the bunches
  // as substructure of the orbit.
  template <bool PrecisionSwitch>
  struct RepTypeTrait {
    typedef uint32_t rep;
    typedef std::ratio<1, gRevolutionFrequency> period;
  };
  //template <>
  //struct RepTypeTrait<true> {
  //  typedef uint64_t rep;
  //  typedef std::ratio<1, gRevolutionFrequency*gNumberOfBunches> period;
  //};

  typedef typename RepTypeTrait<BunchPrecision>::rep    rep;
  typedef typename RepTypeTrait<BunchPrecision>::period period;
  typedef std::chrono::duration<rep, period>            duration;
  typedef std::chrono::time_point<LHCClock>             time_point;
  static const bool is_steady =                         true;

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
