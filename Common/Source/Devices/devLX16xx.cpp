/*
   LK8000 Tactical Flight Computer -  WWW.LK8000.IT
   Released under GNU/GPL License v.2
   See CREDITS.TXT file for authors and copyrights

   $Id$
*/
//_____________________________________________________________________includes_

#include "externs.h"
#include "DevLX16xx.h"
#include "LKINTERFACE.h"
#include "Parser.h"


static int  MacCreadyUpdateTimeout = 0;
static int  BugsUpdateTimeout = 0;
static int  BallastUpdateTimeout = 0;
double fPolar_a=0.0f, fPolar_b=0.0f, fPolar_c=0.0f, fVolume=0.0f;
BOOL bValid = false;
int LX16xxNMEAddCheckSumStrg( TCHAR szStrg[] );

//____________________________________________________________class_definitions_

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/// Registers device into device subsystem.
///
/// @retval true  when device has been registered successfully
/// @retval false device cannot be registered
///
//static
bool DevLX16xx::Register()
{

  return(devRegister(GetName(),
    cap_gps | cap_baro_alt | cap_speed | cap_vario, Install));
} // Register()



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/// Returns device name (max length is @c DEVNAMESIZE).
///
//static
const TCHAR* DevLX16xx::GetName()
{
  return(_T("LX16xx"));
} // GetName()
//                                 Polar
//        MC     BAL    BUG%     a      b      c     Volume
//$PFLX2,1.80,  1.00,   30,    1.71,  -2.43,  1.46, 0*15


//GEXTERN double BUGS;
//GEXTERN double BALLAST;
//GEXTERN int POLARID;
//GEXTERN double POLAR[POLARSIZE];
//GEXTERN double WEIGHTS[POLARSIZE];
//GEXTERN double POLARV[POLARSIZE];
//GEXTERN double POLARLD[POLARSIZE];


bool DevLX16xx::RequestInfos(PDeviceDescriptor_t d)
{
TCHAR  szTmp[254];
  _stprintf(szTmp, TEXT("$PFLX0,GPGGA,1,GPRMC,1,LXWP0,1,LXWP1,30,LXWP2,30,LXWP3,30,LXWP5,30"));
  LX16xxNMEAddCheckSumStrg(szTmp);
  d->Com->WriteString(szTmp);

  return true;
}

bool DevLX16xx::SendInfos(PDeviceDescriptor_t d)
{
if(bValid == false)
  return false;
TCHAR  szTmp[254];
  _stprintf(szTmp, TEXT("$PFLX2,%4.1f,%4.2f,%d,%4.2f,%4.2f,%4.2f,%d"), MACCREADY , 1.0+BALLAST,(int)((1.0f-BUGS)*100.0f),fPolar_a, fPolar_b, fPolar_c,(int) fVolume);
  //_stprintf(szTmp, TEXT("$PFLX2,%4.2f,%4.2f,%4.2f"), MacCready , 1.0+BALLAST,((1.0f-BUGS)*100.0f));
  LX16xxNMEAddCheckSumStrg(szTmp);
  d->Com->WriteString(szTmp);
  d->Com->WriteString(szTmp);
  _stprintf(szTmp, TEXT("$PFLX3,288,0,3.0,4.0,0,25,5.0,1.0,0,0,0.0,,0"));
  //$PFLX3,288,0,3.0,4.0,0,25,5.0,1.0,0,0,0.0,,0
  LX16xxNMEAddCheckSumStrg(szTmp);
//  d->Com->WriteString(szTmp);

  return true;
}

int LX16xxNMEAddCheckSumStrg( TCHAR szStrg[] )
{
int i,iCheckSum=0;
TCHAR  szCheck[254];

 if(szStrg[0] != '$')
   return -1;

 iCheckSum = szStrg[1];
  for (i=2; i < (int)_tcslen(szStrg); i++)
  {
	    iCheckSum ^= szStrg[i];
  }
  _stprintf(szCheck,TEXT("*%X\r\n"),iCheckSum);
  _tcscat(szStrg,szCheck);
  return iCheckSum;
}

BOOL LX16xxPutMacCready(PDeviceDescriptor_t d, double MacCready){
if(bValid == false)
  return false;

  MacCreadyUpdateTimeout = 1;
  DevLX16xx::SendInfos(d);
  return true;

}


BOOL LX16xxPutBugs(PDeviceDescriptor_t d, double Bugs){
if(bValid == false)
  return false;


  MacCreadyUpdateTimeout = 1;
  DevLX16xx::SendInfos(d);
  return(TRUE);

}


BOOL LX16xxPutBallast(PDeviceDescriptor_t d, double Ballast){
if(bValid == false)
  return false;
  MacCreadyUpdateTimeout = 1;
  DevLX16xx::SendInfos(d);
  return(TRUE);

}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/// Installs device specific handlers.
///
/// @param d  device descriptor to be installed
///
/// @retval true  when device has been installed successfully
/// @retval false device cannot be installed
///
//static
BOOL DevLX16xx::Install(PDeviceDescriptor_t d)
{
  _tcscpy(d->Name, GetName());
  d->ParseNMEA    = ParseNMEA;
  d->PutMacCready = LX16xxPutMacCready;
  d->PutBugs      = LX16xxPutBugs;
  d->PutBallast   = LX16xxPutBallast;
  d->Open         = NULL;
  d->Close        = NULL;
  d->Init         = NULL;
  d->LinkTimeout  = GetTrue;
  d->Declare      = NULL;
  d->IsLogger     = NULL;
  d->IsGPSSource  = GetTrue;
  d->IsBaroSource = GetTrue;

  return(true);
} // Install()




//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/// Parses LXWPn sentences.
///
/// @param d         device descriptor
/// @param sentence  received NMEA sentence
/// @param info      GPS info to be updated
///
/// @retval true if the sentence has been parsed
///
//static
BOOL DevLX16xx::ParseNMEA(PDeviceDescriptor_t d, TCHAR* sentence, NMEA_INFO* info)
{
  TCHAR  szTmp[254];
  static int i=40;
  static double oldMC = MACCREADY;
  if (_tcsncmp(_T("$LXWP0"), sentence, 6) == 0)
  {


	  if(fabs(oldMC - MACCREADY)> 0.05f)
	  {
		oldMC = MACCREADY;
		SendInfos(d);
		MacCreadyUpdateTimeout = 3;
      }
  }

  if (_tcsncmp(_T("$GPGGA"), sentence, 6) == 0)
  {
	if((i++ >= 1) ||  (bValid == false))
	{
	  i=0;
	  RequestInfos(d);
	}
	else
	{
	  //    $PFLX4,,,,,,,,,0,
	  _stprintf(szTmp, TEXT("$PFLX4,,,,13512,-1896,,,,0,"));
	  LX16xxNMEAddCheckSumStrg(szTmp);
     // d->Com->WriteString(szTmp);
     // DevLX16xx::PutGPRMB( d);
	}
	return false;
  }



    if (_tcsncmp(_T("$LXWP0"), sentence, 6) == 0)
      return LXWP0(d, sentence + 7, info);
    else
      if (_tcsncmp(_T("$LXWP1"), sentence, 6) == 0)
        return LXWP1(d, sentence + 7, info);
      else
        if (_tcsncmp(_T("$LXWP2"), sentence, 6) == 0)
          return LXWP2(d, sentence + 7, info);
        else
          if (_tcsncmp(_T("$LXWP3"), sentence, 6) == 0)
            return LXWP3(d, sentence + 7, info);

  return(false);
} // ParseNMEA()


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/// Parses LXWP0 sentence.
///
/// @param d         device descriptor
/// @param sentence  received NMEA sentence
/// @param info      GPS info to be updated
///
/// @retval true if the sentence has been parsed
///
//static
bool DevLX16xx::LXWP0(PDeviceDescriptor_t d, const TCHAR* sentence, NMEA_INFO* info)
{
  // $LXWP0,logger_stored, airspeed, airaltitude,
  //   v1[0],v1[1],v1[2],v1[3],v1[4],v1[5], hdg, windspeed*CS<CR><LF>
  //
  // 0 loger_stored : [Y|N] (not used in LX1600)
  // 1 IAS [km/h] ----> Condor uses TAS!
  // 2 baroaltitude [m]
  // 3-8 vario values [m/s] (last 6 measurements in last second)
  // 9 heading of plane (not used in LX1600)
  // 10 windcourse [deg] (not used in LX1600)
  // 11 windspeed [km/h] (not used in LX1600)
  //
  // e.g.:
  // $LXWP0,Y,222.3,1665.5,1.71,,,,,,239,174,10.1

  double alt, airspeed;

  if (ParToDouble(sentence, 1, &airspeed))
  {
    airspeed /= TOKPH;
    info->TrueAirspeed = airspeed;
    info->AirspeedAvailable = TRUE;
  }

  if (ParToDouble(sentence, 2, &alt))
  {
    info->IndicatedAirspeed = airspeed / AirDensityRatio(alt);
    UpdateBaroSource( info, LX16xx, AltitudeToQNHAltitude(alt));
  }

  if (ParToDouble(sentence, 3, &info->Vario))
    info->VarioAvailable = TRUE;


 /*
  if (ParToDouble(sentence, 10, &info->ExternalWindDirection) &&
      ParToDouble(sentence, 11, &info->ExternalWindSpeed))
    info->ExternalWindAvailalbe = TRUE;
*/
  TriggerVarioUpdate();

  return(true);
} // LXWP0()



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/// Parses LXWP1 sentence.
///
/// @param d         device descriptor
/// @param sentence  received NMEA sentence
/// @param info      GPS info to be updated
///
/// @retval true if the sentence has been parsed
///
//static
bool DevLX16xx::LXWP1(PDeviceDescriptor_t, const TCHAR*, NMEA_INFO*)
{
  // $LXWP1,serial number,instrument ID, software version, hardware
  //   version,license string,NU*SC<CR><LF>
  //
  // instrument ID ID of LX1600
  // serial number unsigned serial number
  // software version float sw version
  // hardware version float hw version
  // license string (option to store a license of PDA SW into LX1600)

  // nothing to do
  return(true);
} // LXWP1()




//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/// Parses LXWP2 sentence.
///
/// @param d         device descriptor
/// @param sentence  received NMEA sentence
/// @param info      GPS info to be updated
///
/// @retval true if the sentence has been parsed
///
//static
bool DevLX16xx::LXWP2(PDeviceDescriptor_t, const TCHAR* sentence, NMEA_INFO*)
{
  // $LXWP2,mccready,ballast,bugs,polar_a,polar_b,polar_c, audio volume
  //   *CS<CR><LF>
  //
  // Mccready: float in m/s
  // Ballast: float 1.0 ... 1.5
  // Bugs: 0 - 100%
  // polar_a: float polar_a=a/10000 w=a*v2+b*v+c
  // polar_b: float polar_b=b/100 v=(km/h/100) w=(m/s)
  // polar_c: float polar_c=c
  // audio volume 0 - 100%
//float fBallast,fBugs, polar_a, polar_b, polar_c, fVolume;


double fTmp;
static double LastMc = MACCREADY;

if(MacCreadyUpdateTimeout > 0)
{
	MacCreadyUpdateTimeout--;
	return false;
}

  if (ParToDouble(sentence, 0, &fTmp))
  {
    bValid = true;
	if(fabs(LastMc - fTmp)> 0.05f)
	{
	  LastMc = fTmp;
	  MACCREADY = fTmp;
	}
  }

  if (ParToDouble(sentence, 1, &fTmp))
  {
    if (BallastUpdateTimeout <= 0)
      BALLAST = fTmp - 1.0f;
    else
      BallastUpdateTimeout--;
  }

  if(ParToDouble(sentence, 2, &fTmp))
  {
    if(BugsUpdateTimeout <= 0)
      BUGS = (100.0f-fTmp)/100.0f;
    else
      BugsUpdateTimeout--;
  }

  if (ParToDouble(sentence, 3, &fTmp))
    fPolar_a = fTmp;
  if (ParToDouble(sentence, 4, &fTmp))
    fPolar_b = fTmp;
  if (ParToDouble(sentence, 5, &fTmp))
    fPolar_c = fTmp;
  if (ParToDouble(sentence, 6, &fTmp))
  {
    fVolume = fTmp;
  }
  return(true);
} // LXWP2()


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/// Parses LXWP3 sentence.
///
/// @param d         device descriptor
/// @param sentence  received NMEA sentence
/// @param info      GPS info to be updated
///
/// @retval true if the sentence has been parsed
///
//static
bool DevLX16xx::LXWP3(PDeviceDescriptor_t, const TCHAR*, NMEA_INFO*)
{
  // $LXWP3,altioffset, scmode, variofil, tefilter, televel, varioavg,
  //   variorange, sctab, sclow, scspeed, SmartDiff,
  //   GliderName, time offset*CS<CR><LF>
  //
  // altioffset //offset necessary to set QNE in ft default=0
  // scmode // methods for automatic SC switch index 0=EXTERNAL, 1=ON CIRCLING
  //   2=auto IAS default=1
  // variofil // filtering of vario in seconds (float) default=1
  // tefilter // filtering of TE compensation in seconds (float) 0 = no
  //   filtering (default=0)
  // televel // level of TE compensation from 0 to 250 default=0 (%) default=0
  // varioavg // averaging time in seconds for integrator default=25
  // variorange // 2.5 5 or 10 (m/s or kts) (float) default=5.0
  // sctab // area of silence in SC mode (float) 0-5.0 1.0= silence between
  //   +1m/s and -1m/s default=1
  // sclow // external switch/taster function 0=NORMAL 1=INVERTED 2=TASTER
  //   default=1
  // scspeed // speed of automatic switch from vario to sc mode if SCMODE==2 in
  //   (km/h) default=110
  // SmartDiff float (m/s/s) (Smart VARIO filtering)
  // GliderName // Glider name string max. 14 characters
  // time offset int in hours

  // nothing to do
  return(true);
} // LXWP3()


bool DevLX16xx::PutGPRMB(PDeviceDescriptor_t d)
{

//RMB - The recommended minimum navigation sentence is sent whenever a route or a goto is active.
//      On some systems it is sent all of the time with null data.
//      The Arrival alarm flag is similar to the arrival alarm inside the unit and can be decoded to
//      drive an external alarm.
//      Note: the use of leading zeros in this message to preserve the character spacing.
//      This is done, I believe, because some autopilots may depend on exact character spacing.
//
//     $GPRMB,A,0.66,L,003,004,4917.24,N,12309.57,W,001.3,052.5,000.5,V*20
//where:
//           RMB          Recommended minimum navigation information
//           A            Data status A = OK, V = Void (warning)
//           0.66,L       Cross-track error (nautical miles, 9.99 max),
//                                steer Left to correct (or R = right)
//           003          Origin waypoint ID
//           004          Destination waypoint ID
//           4917.24,N    Destination waypoint latitude 49 deg. 17.24 min. N
//           12309.57,W   Destination waypoint longitude 123 deg. 09.57 min. W
//           001.3        Range to destination, nautical miles (999.9 max)
//           052.5        True bearing to destination
//           000.5        Velocity towards destination, knots
//           V            Arrival alarm  A = arrived, V = not arrived
//           *20          checksum

TCHAR  szTmp[256];
/*
extern START_POINT StartPoints[];
extern TASK_POINT Task[];
extern TASKSTATS_POINT TaskStats[];
extern WAYPOINT *WayPointList;
extern WPCALC   *WayPointCalc;
*/
//WayPointCalc->
  int overindex = GetOvertargetIndex();
  _stprintf(
      szTmp,
      TEXT("$GPRMB,A,0.66,L,EDLG,%6s,%010.5f,N,%010.5f,E,%05.1f,%05.1f,%05.1f,V"),

      WayPointList[overindex].Name,
      WayPointList[overindex].Latitude * 100,
      WayPointList[overindex].Longitude * 100,
      WayPointCalc->Distance * 1000 * TONAUTICALMILES, WayPointCalc->Bearing,
      WayPointCalc->VGR * TOKNOTS);

  //  _stprintf(szTmp, TEXT("$GPRMB,A,0.00,L,KLE,UWOE,4917.24,N,12309.57,E,011.3,052.5,000.5,V"));
  LX16xxNMEAddCheckSumStrg(szTmp);

  d->Com->WriteString(szTmp);


return(true);
}
