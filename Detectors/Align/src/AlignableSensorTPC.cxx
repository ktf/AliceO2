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

/// @file   AlignableSensorTPC.h
/// @author ruben.shahoyan@cern.ch, michael.lettrich@cern.ch
/// @since  2021-02-01
/// @brief  TPC sensor (chamber)

#include "Align/AlignableSensorTPC.h"
#include "Align/utils.h"
#include "Framework/Logger.h"
//#include "AliTrackPointArray.h"
//#include "AliESDtrack.h"
#include "Align/AlignmentPoint.h"
#include "Align/AlignableDetector.h"

ClassImp(o2::align::AlignableSensorTPC)

  using namespace o2::align::utils;
using namespace TMath;

namespace o2
{
namespace align
{

//_________________________________________________________
AlignableSensorTPC::AlignableSensorTPC(const char* name, int vid, int iid, int isec)
  : AlignableSensor(name, vid, iid), fSector(isec)
{
  // def c-tor
}

//_________________________________________________________
AlignableSensorTPC::~AlignableSensorTPC()
{
  // d-tor
}

//____________________________________________
void AlignableSensorTPC::prepareMatrixT2L()
{
  // extract from geometry T2L matrix
  double alp = math_utils::detail::sector2Angle<float>(fSector);
  double loc[3] = {0, 0, 0}, glo[3];
  getMatrixL2GIdeal().LocalToMaster(loc, glo);
  double x = Sqrt(glo[0] * glo[0] + glo[1] * glo[1]);
  TGeoHMatrix t2l;
  t2l.SetDx(x);
  t2l.RotateZ(alp * RadToDeg());
  const TGeoHMatrix& l2gi = getMatrixL2GIdeal().Inverse();
  t2l.MultiplyLeft(&l2gi);
  /*
  const TGeoHMatrix* t2l = AliGeomManager::GetTracking2LocalMatrix(getVolID());
  if (!t2l) {
    Print("long");
    AliFatalF("Failed to find T2L matrix for VID:%d %s",getVolID(),getSymName());
  }
  */
  setMatrixT2L(t2l);
  //
}

} // namespace align
} // namespace o2
