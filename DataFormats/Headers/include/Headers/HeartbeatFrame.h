//-*- Mode: C++ -*-

#ifndef ALICEO2_HEADER_HEARTBEATFRAME_H
#define ALICEO2_HEADER_HEARTBEATFRAME_H

// @file   Heartbeatframe.h
// @author Matthias Richter
// @since  2017-02-02
// @brief  Definition of the heartbeat frame layout

#include "Headers/DataHeader.h"

namespace AliceO2 {
namespace Header {

// The Heartbeat frame layout is specified in
// http://svnweb.cern.ch/world/wsvn/alicetdrrun3/Notes/Run34SystemNote/detector-read-alice/ALICErun34_readout.pdf
// TODO: replace citation with correct ALICE note reference when published

// general remark:
// at the moment its not clear how the heartbeat frame is transmitted in  AliceO2.
// Current understanding is that the heartbeat header and trailer embed some detector
// data and form the heartbeat frame. The detector data is the payload which is going
// to be sent as the payload of the O2 data packet. The HBH and HBT must be stripped
// from the payload and probably be added to the header stack and later on be stripped
// from there as well during the accumulation of subtimeframes and timeframes as the
// HB information should be identical in the (sub)timeframes

// define the data type id for the heartbeat frame
// check if this is the correct term, we probably won't send what is referred to be
// the heartbeat frame (composition of HBH - detector payload - HBT); instead, the
// HBH and HBT can be added to the header stack
extern const AliceO2::Header::DataDescription gDataDescriptionHeartbeatFrame;

struct HeartbeatHeader
{
  union {
    // the complete 64 bit header word
    uint64_t headerWord;
    struct {
	// bit 0 to 31: orbit number
	uint32_t orbit;
	// bit 32 to 43: bunch crossing id
	uint16_t bcid:12;
	// bit 44 to 47: reserved
	uint16_t reserved:4;
	// bit 48 to 51: trigger type
	uint8_t triggerType:4;
	// bit 52 to 55: reserved
	uint8_t reservedTriggerType:4;
	// bit 56 to 59: header length
	uint8_t headerLength:4;
	// bit 60 to 63: block type (=1 HBF/trigger Header)
	uint8_t blockType:4;
    };
  };

  HeartbeatHeader() : headerWord(0) {}
};

struct HeartbeatTrailer
{
  union {
    // the complete 64 bit trailer word
    uint64_t trailerWord;
    struct {
	// bit 0 to 31: data length in words
	uint32_t dataLength;
	// bit 32 to 52: detector specific status words
	uint32_t bcid:21;
	// bit 53: =1 in case a new physics trigger arrived within read-out period
	uint16_t hbfTruncated:1;
	// bit 54: =0 HBF correctly transmitted
	uint16_t hbfStatus:1;
	// bit 55: =1 HBa/0 HBr received
	uint16_t hbAccept:1;
	// bit 56 to 59: trailer length
	uint16_t trailerLength:4;
	// bit 60 to 63: block type (=5 HBF Trailer)
	uint8_t blockType:4;
    };
  };

  HeartbeatTrailer() : trailerWord(0) {}
};

// composite struct for the HBH and HBT which are the envelope for the payload
// in the heartbeat frame
// TODO: check if the copying of header and trailer can be avoided if references
// are used in a temporary object inserted to the header stack
struct HeartbeatFrameEnvelope : public BaseHeader
{
  //static data for this header type/version
  static const uint32_t sVersion;
  static const AliceO2::Header::HeaderType sHeaderType;
  static const AliceO2::Header::SerializationMethod sSerializationMethod;

  HeartbeatHeader header;
  HeartbeatTrailer trailer;

  HeartbeatFrameEnvelope()
    : BaseHeader(sizeof(HeartbeatFrameEnvelope), sHeaderType, sSerializationMethod, sVersion)
    , header(), trailer() {}

  HeartbeatFrameEnvelope(const HeartbeatHeader& h, const HeartbeatTrailer& t)
    : BaseHeader(sizeof(HeartbeatFrameEnvelope), sHeaderType, sSerializationMethod, sVersion)
    , header(h), trailer(t) {}
};
};
};
#endif
