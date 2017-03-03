#define BOOST_TEST_MODULE Test DataFlow Timeframetest
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include "DataFlow/Timeframe.h"
#include "FairMQDevice.h"
#include "FairMQMessage.h"
#include "FairMQParts.h"
#include "Headers/DataHeader.h"
#include "O2Device/O2Device.h"
#include "TFile.h"

namespace AliceO2
{
namespace DataFlow
{
BOOST_AUTO_TEST_CASE(MessageSizePair_test)
{
  size_t S = 100;
  char* buffer = new char[S];

  // fill the buffer with something
  buffer[0] = 'a';
  buffer[S - 1] = 'z';

  MessageSizePair mes(S, buffer);

  // assert some properties on MessageSizePair
  BOOST_CHECK(mes.size == S);
  BOOST_CHECK(mes.buffer = buffer);

  // check ROOT IO of MessageSizePair
  TFile* file = TFile::Open("mspair.root", "RECREATE");
  if (file) {
    file->WriteObject(&mes, "MessageSizePair");
    file->Close();
  }

  MessageSizePair* ptr = nullptr;
  TFile* infile = TFile::Open("mspair.root", "READ");
  if (infile) {
    infile->GetObject("MessageSizePair", ptr);
    infile->Close();
    BOOST_CHECK(ptr != nullptr);
  }
  BOOST_CHECK(ptr->size == S);
  // test first and last characters in buffer to see if correctly streamed
  BOOST_CHECK(ptr->buffer[0] == buffer[0]);
  BOOST_CHECK(ptr->buffer[S - 1] == buffer[S - 1]);
}

BOOST_AUTO_TEST_CASE(Timeframe_test)
{
  FairMQParts messages;
  FairMQDevice fakedevice; // a fake device in order to be able to
                           // generate a message
  fakedevice.SetTransport("zeromq");
  FairMQMessagePtr msg(fakedevice.NewMessage(1000));
  messages.AddPart(std::move(msg));

  AliceO2::Header::DataHeader dh;
  messages.AddPart(fakedevice.NewSimpleMessage(dh));

  Timeframe frame(messages);
  BOOST_CHECK(frame.GetNumParts() == 2);

  // test ROOT IO
  TFile* file = TFile::Open("timeframe.root", "RECREATE");
  if (file) {
    file->WriteObject(&frame, "Timeframe");
    file->Close();
  }

  Timeframe* ptr = nullptr;
  TFile* infile = TFile::Open("timeframe.root", "READ");
  if (infile) {
    infile->GetObject("Timeframe", ptr);
    infile->Close();
    BOOST_CHECK(ptr != nullptr);
  }

  // test access to data
  BOOST_CHECK(ptr->GetNumParts() == 2);
  for (int i = 0; i < ptr->GetNumParts(); ++i) {
    BOOST_CHECK(ptr->GetPart(i).size == messages[i].GetSize());
  }
}
}
}
