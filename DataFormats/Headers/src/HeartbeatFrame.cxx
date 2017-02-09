/// @file   Heartbeatframe.cxx
/// @author Matthias Richter
/// @since  2017-02-02
/// @brief  Some additions to definition of the heartbeat frame layout

#include "Headers/HeartbeatFrame.h"

// define the description with a terminating '0' (meaning 15 characters)
const AliceO2::Header::DataDescription AliceO2::Header::gDataDescriptionHeartbeatFrame("HEARTBEATFRAME");
