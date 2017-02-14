#ifndef SUBFRAMEMETADATA_H
#define SUBFRAMEMETADATA_H

namespace AliceO2 {
namespace DataFlow {

struct SubframeMetadata
{
  // TODO: replace with timestamp struct
  // IDEA: not timeframeID because can be calculcated with helper function
  uint64_t startTime = ~(uint64_t)0;
  uint64_t duration = ~(uint64_t)0;

  //further meta data to be added

  // putting data specific to FLP origin
  int      flpIndex;
};

} // end namespace DataFlow
} // end namespace AliceO2


#endif
