/*
   LK8000 Tactical Flight Computer -  WWW.LK8000.IT
   Released under GNU/GPL License v.2
   See CREDITS.TXT file for authors and copyrights

   $Id: dlgStatistics.cpp,v 1.1 2011/12/21 10:29:29 root Exp root $
*/

#include "externs.h"
#include "InfoBoxLayout.h"
#include "McReady.h"
#include "dlgTools.h"
#include "Atmosphere.h"
#include "RasterTerrain.h"
#include "LKInterface.h"
#include "LKAirspace.h"
#include "RGB.h"

using std::min;
using std::max;

#define GROUND_COLOUR RGB(157,101,60)

#define MAXPAGE 8

double Statistics::yscale;
double Statistics::xscale;
double Statistics::y_min;
double Statistics::x_min;
double Statistics::x_max;
double Statistics::y_max;
bool   Statistics::unscaled_x;
bool   Statistics::unscaled_y;

static HPEN penThinSignal = NULL;
static bool asp_heading_task = false;

#define BORDER_X 24
#define BORDER_Y 19

#define GC_MAX_NO 50 //(AIRSPACE_SCANSIZE_H*AIRSPACE_SCANSIZE_X)
#define ID_NO_LABLE    0
#define ID_SHORT_LABLE 1
#define ID_FULL_LABLE  2
int X=-1,Y=-1;



int iNohandeldSpaces=0;
AirSpaceSideViewSTRUCT pHandeled[GC_MAX_NO];
AirSpaceSideViewSTRUCT asDrawn[GC_MAX_NO];
void Statistics::ResetScale() {
  unscaled_y = true;  
  unscaled_x = true;  
}



void Statistics::Reset() {
  ThermalAverage.Reset();
  Wind_x.Reset();
  Wind_y.Reset();
  Altitude.Reset();
  Altitude_Base.Reset();
  Altitude_Ceiling.Reset();
  Task_Speed.Reset();
  Altitude_Terrain.Reset();
  for(int j=0;j<MAXTASKPOINTS;j++) {
    LegStartTime[j] = -1;
  }
}


void Statistics::ScaleYFromData(const RECT rc, LeastSquares* lsdata) 
{
  if (!lsdata->sum_n) {
    return;
  }

  if (unscaled_y) {
    y_min = lsdata->y_min;
    y_max = lsdata->y_max;
    unscaled_y = false;
  } else {
    y_min = min(y_min,lsdata->y_min);
    y_max = max(y_max,lsdata->y_max);
  }

  if (lsdata->sum_n>1) {
    double y0, y1;
    y0 = lsdata->x_min*lsdata->m+lsdata->b;
    y1 = lsdata->x_max*lsdata->m+lsdata->b;
    y_min = min(y_min,min(y0,y1));
    y_max = max(y_max,max(y0,y1));
  }


  if (fabs(y_max - y_min) > 50){
    yscale = (y_max - y_min);
    if (yscale>0) {
      yscale = (rc.bottom-rc.top-BORDER_Y)/yscale;
    }
  } else {
    yscale = 2000;
  }
}


void Statistics::ScaleXFromData(const RECT rc, LeastSquares* lsdata) 
{
  if (!lsdata->sum_n) {
    return;
  }
  if (unscaled_x) {
    x_min = lsdata->x_min;
    x_max = lsdata->x_max;
    unscaled_x = false;
  } else {
    x_min = min(x_min,lsdata->x_min);
    x_max = max(x_max,lsdata->x_max);
  }

  xscale = (x_max-x_min);
  if (xscale>0) {
    xscale = (rc.right-rc.left-BORDER_X)/xscale;
  }
}


void Statistics::ScaleYFromValue(const RECT rc, const double value) 
{
  if (unscaled_y) {
    y_min = value;
    y_max = value;
    unscaled_y = false;
  } else {
    y_min = min(value, y_min);
    y_max = max(value, y_max);
  }

  yscale = (y_max - y_min);
  if (yscale>0) {
    yscale = (rc.bottom-rc.top-BORDER_Y)/yscale;
  }
}


void Statistics::ScaleXFromValue(const RECT rc, const double value) 
{
  if (unscaled_x) {
    x_min = value;
    x_max = value;
    unscaled_x = false;
  } else {
    x_min = min(value, x_min);
    x_max = max(value, x_max);
  }

  xscale = (x_max-x_min);
  if (xscale>0) {
    xscale = (rc.right-rc.left-BORDER_X)/xscale;
  }

}


void Statistics::StyleLine(HDC hdc, const POINT l1, const POINT l2,
                           const int Style, const RECT rc) {
  int minwidth = 1;
  minwidth = 2;
  POINT line[2];
  line[0] = l1;
  line[1] = l2;
  HPEN mpen ;
  HPEN oldpen;

  switch (Style) {
  case STYLE_BLUETHIN:
    MapWindow::DrawDashLine(hdc, 
			    minwidth, 
			    l1, 
			    l2, 
			    RGB(0,50,255), rc);
    break;
  case STYLE_REDTHICK:
    MapWindow::DrawDashLine(hdc, 2,
			    l1,
			    l2,
			    RGB(250,50,50), rc);
    break;

  case STYLE_GREENMEDIUM:
	  line[0].x +=2;
	  line[1].x +=2;
      mpen = (HPEN)CreatePen(PS_SOLID, IBLSCALE(2),  RGB(0,255,0));
      oldpen = (HPEN)SelectObject(hdc, (HPEN)mpen);
      MapWindow::_Polyline(hdc, line, 2, rc);
      SelectObject(hdc, oldpen);
      DeleteObject(mpen);
    break;

  case STYLE_GREENTHICK:
	  line[0].x +=2;
	  line[1].x +=2;
      mpen = (HPEN)CreatePen(PS_SOLID, IBLSCALE(4),  RGB(0,255,0));
      oldpen = (HPEN)SelectObject(hdc, (HPEN)mpen);
      MapWindow::_Polyline(hdc, line, 2, rc);
      SelectObject(hdc, oldpen);
      DeleteObject(mpen);
    break;

  case STYLE_ORANGETHICK:
	line[0].x +=2;
	line[1].x +=2;
    mpen = (HPEN)CreatePen(PS_SOLID, IBLSCALE(4),  RGB(255,165,0));
    oldpen = (HPEN)SelectObject(hdc, (HPEN)mpen);
    MapWindow::_Polyline(hdc, line, 2, rc);
    SelectObject(hdc, oldpen);
    DeleteObject(mpen);
  break;

  case STYLE_DASHGREEN:
    MapWindow::DrawDashLine(hdc, 2, 
			    line[0], 
			    line[1], 
			    RGB(0,255,0), rc);
    break;
  case STYLE_MEDIUMBLACK:
    SelectObject(hdc, penThinSignal /*GetStockObject(BLACK_PEN)*/);
    MapWindow::_Polyline(hdc, line, 2, rc);
    break;
  case STYLE_THINDASHPAPER:
    MapWindow::DrawDashLine(hdc, 1, 
			    l1, 
			    l2, 
			    RGB(0x60,0x60,0x60), rc);    
    break;
  case STYLE_WHITETHICK:
    MapWindow::DrawDashLine(hdc, 3, 
          l1, 
          l2, 
          RGB(255,255,255), rc);
    break;

  default:
    break;
  }

}


void Statistics::DrawLabel(HDC hdc, const RECT rc, const TCHAR *text, 
			   const double xv, const double yv) {

  SIZE tsize;
  GetTextExtentPoint(hdc, text, _tcslen(text), &tsize);
  int x = (int)((xv-x_min)*xscale)+rc.left-tsize.cx/2+BORDER_X;
  int y = (int)((y_max-yv)*yscale)+rc.top-tsize.cy/2;
  SetBkMode(hdc, OPAQUE);
  ExtTextOut(hdc, x, y, ETO_OPAQUE, NULL, text, _tcslen(text), NULL);
  SetBkMode(hdc, TRANSPARENT);
}


void Statistics::DrawNoData(HDC hdc, RECT rc) {

  SIZE tsize;
  TCHAR text[80];
	// LKTOKEN  _@M470_ = "No data" 
  _stprintf(text,TEXT("%s"), gettext(TEXT("_@M470_")));
  GetTextExtentPoint(hdc, text, _tcslen(text), &tsize);
  int x = (int)(rc.left+rc.right-tsize.cx)/2;
  int y = (int)(rc.top+rc.bottom-tsize.cy)/2;
  SetBkMode(hdc, OPAQUE);
  ExtTextOut(hdc, x, y, ETO_OPAQUE, NULL, text, _tcslen(text), NULL);
  SetBkMode(hdc, TRANSPARENT);
}



void Statistics::DrawXLabel(HDC hdc, const RECT rc, const TCHAR *text) {
  SIZE tsize;
  HFONT hfOld = (HFONT)SelectObject(hdc, MapLabelFont);
  GetTextExtentPoint(hdc, text, _tcslen(text), &tsize);
  int x = rc.right-tsize.cx-IBLSCALE(3);
  int y = rc.bottom-tsize.cy;
  ExtTextOut(hdc, x, y, ETO_OPAQUE, NULL, text, _tcslen(text), NULL);
  SelectObject(hdc, hfOld);
}


void Statistics::DrawYLabel(HDC hdc, const RECT rc, const TCHAR *text) {
  SIZE tsize;
  HFONT hfOld = (HFONT)SelectObject(hdc, MapLabelFont);
  GetTextExtentPoint(hdc, text, _tcslen(text), &tsize);
  int x = max(2,(int)rc.left-(int)tsize.cx);
  int y = rc.top;
  ExtTextOut(hdc, x, y, ETO_OPAQUE, NULL, text, _tcslen(text), NULL);
  SelectObject(hdc, hfOld);
}


void Statistics::DrawTrend(HDC hdc, const RECT rc, LeastSquares* lsdata, 
			   const int Style) 
{
  if (lsdata->sum_n<2) {
    return;
  }

  if (unscaled_x || unscaled_y) {
    return;
  }

  double xmin, xmax, ymin, ymax;
  xmin = lsdata->x_min;
  xmax = lsdata->x_max;
  ymin = lsdata->x_min*lsdata->m+lsdata->b;
  ymax = lsdata->x_max*lsdata->m+lsdata->b;
  
  xmin = (int)((xmin-x_min)*xscale)+rc.left+BORDER_X;
  xmax = (int)((xmax-x_min)*xscale)+rc.left+BORDER_X;
  ymin = (int)((y_max-ymin)*yscale)+rc.top;
  ymax = (int)((y_max-ymax)*yscale)+rc.top;
  POINT line[2];
  line[0].x = (int)xmin;
  line[0].y = (int)ymin;
  line[1].x = (int)xmax;
  line[1].y = (int)ymax;

  StyleLine(hdc, line[0], line[1], Style, rc);

}


void Statistics::DrawTrendN(HDC hdc, const RECT rc, 
			    LeastSquares* lsdata, 
                            const int Style) 
{
  if (lsdata->sum_n<2) {
    return;
  }

  if (unscaled_x || unscaled_y) {
    return;
  }

  double xmin, xmax, ymin, ymax;
  xmin = 0.5;
  xmax = lsdata->sum_n+0.5;
  ymin = lsdata->x_min*lsdata->m+lsdata->b;
  ymax = lsdata->x_max*lsdata->m+lsdata->b;
  
  xmin = (int)((xmin)*xscale)+rc.left+BORDER_X;
  xmax = (int)((xmax)*xscale)+rc.left+BORDER_X;
  ymin = (int)((y_max-ymin)*yscale)+rc.top;
  ymax = (int)((y_max-ymax)*yscale)+rc.top;
  POINT line[2];
  line[0].x = (int)xmin;
  line[0].y = (int)ymin;
  line[1].x = (int)xmax;
  line[1].y = (int)ymax;

  StyleLine(hdc, line[0], line[1], Style, rc);

}


void Statistics::DrawLine(HDC hdc, const RECT rc, 
			  const double xmin, const double ymin,
                          const double xmax, const double ymax,
                          const int Style) {

  if (unscaled_x || unscaled_y) {
    return;
  }
  POINT line[2];
  line[0].x = (int)((xmin-x_min)*xscale)+rc.left+BORDER_X;
  line[0].y = (int)((y_max-ymin)*yscale)+rc.top;
  line[1].x = (int)((xmax-x_min)*xscale)+rc.left+BORDER_X;
  line[1].y = (int)((y_max-ymax)*yscale)+rc.top;

  StyleLine(hdc, line[0], line[1], Style, rc);

}


void Statistics::DrawBarChart(HDC hdc, const RECT rc, LeastSquares* lsdata) {
  int i;

  if (unscaled_x || unscaled_y) {
    return;
  }

  SelectObject(hdc, GetStockObject(WHITE_PEN));
  SelectObject(hdc, GetStockObject(WHITE_BRUSH));

  int xmin, ymin, xmax, ymax;

  for (i= 0; i<lsdata->sum_n; i++) {
    xmin = (int)((i+1+0.2)*xscale)+rc.left+BORDER_X;
    ymin = (int)((y_max-y_min)*yscale)+rc.top;
    xmax = (int)((i+1+0.8)*xscale)+rc.left+BORDER_X;
    ymax = (int)((y_max-lsdata->ystore[i])*yscale)+rc.top;
    Rectangle(hdc, 
              xmin, 
              ymin,
              xmax,
              ymax);
  }

}


void Statistics::DrawFilledLineGraph(HDC hdc, const RECT rc, 
				     LeastSquares* lsdata,
				     const COLORREF color) {

  POINT line[4];

  for (int i=0; i<lsdata->sum_n-1; i++) {
    line[0].x = (int)((lsdata->xstore[i]-x_min)*xscale)+rc.left+BORDER_X;
    line[0].y = (int)((y_max-lsdata->ystore[i])*yscale)+rc.top;
    line[1].x = (int)((lsdata->xstore[i+1]-x_min)*xscale)+rc.left+BORDER_X;
    line[1].y = (int)((y_max-lsdata->ystore[i+1])*yscale)+rc.top;
    line[2].x = line[1].x;
    line[2].y = rc.bottom-BORDER_Y;
    line[3].x = line[0].x;
    line[3].y = rc.bottom-BORDER_Y;
    Polygon(hdc, line, 4);
  }
}



void Statistics::DrawLineGraph(HDC hdc, RECT rc, LeastSquares* lsdata,
                               int Style) {

  POINT line[2];

  for (int i=0; i<lsdata->sum_n-1; i++) {
    line[0].x = (int)((lsdata->xstore[i]-x_min)*xscale)+rc.left+BORDER_X;
    line[0].y = (int)((y_max-lsdata->ystore[i])*yscale)+rc.top;
    line[1].x = (int)((lsdata->xstore[i+1]-x_min)*xscale)+rc.left+BORDER_X;
    line[1].y = (int)((y_max-lsdata->ystore[i+1])*yscale)+rc.top;

    // STYLE_DASHGREEN
    // STYLE_MEDIUMBLACK
    StyleLine(hdc, line[0], line[1], Style, rc);
  }
}


void Statistics::FormatTicText(TCHAR *text, const double val, const double step) {
  if (step<1.0) {
    _stprintf(text, TEXT("%.1f"), val);
  } else {
    _stprintf(text, TEXT("%.0f"), val);
  }
}


void Statistics::DrawXGrid(HDC hdc, const RECT rc, 
			   const double tic_step, 
			   const double zero,
                           const int Style, 
			   const double unit_step, bool draw_units) {

  POINT line[2];

  double xval;
  SIZE tsize;

  int xmin, ymin, xmax, ymax;
  if (!tic_step) return;

  //  bool do_units = ((x_max-zero)/tic_step)<10;

  for (xval=zero; xval<= x_max; xval+= tic_step) {

    xmin = (int)((xval-x_min)*xscale)+rc.left+BORDER_X;
    ymin = rc.top;
    xmax = xmin;
    ymax = rc.bottom;
    line[0].x = xmin;
    line[0].y = ymin;
    line[1].x = xmax;
    line[1].y = ymax-BORDER_Y;

    // STYLE_THINDASHPAPER
    if ((xval< x_max) 
        && (xmin>=rc.left+BORDER_X) && (xmin<=rc.right)) {
      StyleLine(hdc, line[0], line[1], Style, rc);

      if (draw_units) {
	TCHAR unit_text[MAX_PATH];
	FormatTicText(unit_text, xval*unit_step/tic_step, unit_step);

//	SetBkMode(hdc, OPAQUE);
	GetTextExtentPoint(hdc, unit_text, _tcslen(unit_text), &tsize);
	ExtTextOut(hdc, xmin-tsize.cx/2, ymax-tsize.cy ,
		   ETO_OPAQUE, NULL, unit_text, _tcslen(unit_text), NULL);
	SetBkMode(hdc, TRANSPARENT);
      }
    }

  }

  for (xval=zero-tic_step; xval>= x_min; xval-= tic_step) {

    xmin = (int)((xval-x_min)*xscale)+rc.left+BORDER_X;
    ymin = rc.top;
    xmax = xmin;
    ymax = rc.bottom;
    line[0].x = xmin;
    line[0].y = ymin;
    line[1].x = xmax;
    line[1].y = ymax-BORDER_Y;

    // STYLE_THINDASHPAPER

    if ((xval> x_min) 
        && (xmin>=rc.left+BORDER_X) && (xmin<=rc.right)) {

      StyleLine(hdc, line[0], line[1], Style, rc);

      if (draw_units) {
	TCHAR unit_text[MAX_PATH];
	FormatTicText(unit_text, xval*unit_step/tic_step, unit_step);
//	SetBkMode(hdc, OPAQUE);
	GetTextExtentPoint(hdc, unit_text, _tcslen(unit_text), &tsize);
	ExtTextOut(hdc, xmin-tsize.cx/2, ymax-tsize.cy,
		   ETO_OPAQUE, NULL, unit_text, _tcslen(unit_text), NULL);
	SetBkMode(hdc, TRANSPARENT);
      }
    }

  }

}

void Statistics::DrawYGrid(HDC hdc, const RECT rc, 
			   const double tic_step, 
			   const double zero,
                           const int Style, 
			   const double unit_step, bool draw_units) {

  POINT line[2];
  SIZE tsize;
  double yval;

  int xmin, ymin, xmax, ymax;

  if (!tic_step) return;

  for (yval=zero; yval<= y_max; yval+= tic_step) {

    xmin = rc.left;
    ymin = (int)((y_max-yval)*yscale)+rc.top;
    xmax = rc.right;
    ymax = ymin;
    line[0].x = xmin+BORDER_X;
    line[0].y = ymin;
    line[1].x = xmax;
    line[1].y = ymax;

    // STYLE_THINDASHPAPER
    if ((yval< y_max) && 
        (ymin>=rc.top) && (ymin<=rc.bottom-BORDER_Y)) {

      StyleLine(hdc, line[0], line[1], Style, rc);

      if (draw_units) {
	TCHAR unit_text[MAX_PATH];
	FormatTicText(unit_text, yval*unit_step/tic_step, unit_step);
//	SetBkMode(hdc, OPAQUE);
	GetTextExtentPoint(hdc, unit_text, _tcslen(unit_text), &tsize);
	ExtTextOut(hdc, xmin, ymin-tsize.cy/2,
		   ETO_OPAQUE, NULL, unit_text, _tcslen(unit_text), NULL);
//	SetBkMode(hdc, TRANSPARENT);
      }
    }
  }

  for (yval=zero-tic_step; yval>= y_min; yval-= tic_step) {

    xmin = rc.left;
    ymin = (int)((y_max-yval)*yscale)+rc.top;
    xmax = rc.right;
    ymax = ymin;
    line[0].x = xmin+BORDER_X;
    line[0].y = ymin;
    line[1].x = xmax;
    line[1].y = ymax;

    // STYLE_THINDASHPAPER
    if ((yval> y_min) &&
        (ymin>=rc.top) && (ymin<=rc.bottom-BORDER_Y)) {

      StyleLine(hdc, line[0], line[1], Style, rc);

      if (draw_units) {
	TCHAR unit_text[MAX_PATH];
	FormatTicText(unit_text, yval*unit_step/tic_step, unit_step);
//	SetBkMode(hdc, OPAQUE);
	GetTextExtentPoint(hdc, unit_text, _tcslen(unit_text), &tsize);
	ExtTextOut(hdc, xmin, ymin-tsize.cy/2,
		   ETO_OPAQUE, NULL, unit_text, _tcslen(unit_text), NULL);
//	SetBkMode(hdc, TRANSPARENT);
      }
    }
  }
}




#include "ContestMgr.h"
using std::min;
using std::max;

static CContestMgr::TType contestType = CContestMgr::TYPE_OLC_CLASSIC;


void Statistics::RenderBarograph(HDC hdc, const RECT rc)
{

  if (flightstats.Altitude.sum_n<2) {
    DrawNoData(hdc, rc);
    return;
  }

  ResetScale();

  ScaleXFromData(rc, &flightstats.Altitude);
  ScaleYFromData(rc, &flightstats.Altitude);
  ScaleYFromValue(rc, 0);
  ScaleXFromValue(rc, flightstats.Altitude.x_min+1.0); // in case no data
  ScaleXFromValue(rc, flightstats.Altitude.x_min); 

  for(int j=1;j<MAXTASKPOINTS;j++) {
    if (ValidTaskPoint(j) && (flightstats.LegStartTime[j]>=0)) {
      double xx = 
        (flightstats.LegStartTime[j]-CALCULATED_INFO.TakeOffTime)/3600.0;
      if (xx>=0) {
        DrawLine(hdc, rc,
                 xx, y_min,
                 xx, y_max,
                 STYLE_REDTHICK);
      }
    }
  }

  HPEN   hpHorizonGround;
  HBRUSH hbHorizonGround;
  hpHorizonGround = (HPEN)CreatePen(PS_SOLID, IBLSCALE(1), 
                                    GROUND_COLOUR);
  hbHorizonGround = (HBRUSH)CreateSolidBrush(GROUND_COLOUR);
  SelectObject(hdc, hpHorizonGround);
  SelectObject(hdc, hbHorizonGround);

  DrawFilledLineGraph(hdc, rc, &flightstats.Altitude_Terrain,
                GROUND_COLOUR);

  SelectObject(hdc, GetStockObject(WHITE_PEN));
  SelectObject(hdc, GetStockObject(WHITE_BRUSH));
  DeleteObject(hpHorizonGround);
  DeleteObject(hbHorizonGround);

  DrawXGrid(hdc, rc, 
            0.5, flightstats.Altitude.x_min,
            STYLE_THINDASHPAPER, 0.5, true);

  DrawYGrid(hdc, rc, 1000/ALTITUDEMODIFY, 0, STYLE_THINDASHPAPER,
            1000, true);

  DrawLineGraph(hdc, rc, &flightstats.Altitude,
                STYLE_MEDIUMBLACK);

  DrawTrend(hdc, rc, &flightstats.Altitude_Base, STYLE_BLUETHIN);

  DrawTrend(hdc, rc, &flightstats.Altitude_Ceiling, STYLE_BLUETHIN);

  DrawXLabel(hdc, rc, TEXT("t"));
  DrawYLabel(hdc, rc, TEXT("h"));

}


void Statistics::RenderSpeed(HDC hdc, const RECT rc)
{

  if ((flightstats.Task_Speed.sum_n<2)
      || !ValidTaskPoint(ActiveWayPoint)) {
    DrawNoData(hdc, rc);
    return;
  }

  ResetScale();

  ScaleXFromData(rc, &flightstats.Task_Speed);
  ScaleYFromData(rc, &flightstats.Task_Speed);
  ScaleYFromValue(rc, 0);
  ScaleXFromValue(rc, flightstats.Task_Speed.x_min+1.0); // in case no data
  ScaleXFromValue(rc, flightstats.Task_Speed.x_min); 

  for(int j=1;j<MAXTASKPOINTS;j++) {
    if (ValidTaskPoint(j) && (flightstats.LegStartTime[j]>=0)) {
      double xx = 
        (flightstats.LegStartTime[j]-CALCULATED_INFO.TaskStartTime)/3600.0;
      if (xx>=0) {
        DrawLine(hdc, rc,
                 xx, y_min,
                 xx, y_max,
                 STYLE_REDTHICK);
      }
    }
  }

  DrawXGrid(hdc, rc, 
            0.5, flightstats.Task_Speed.x_min,
            STYLE_THINDASHPAPER, 0.5, true);

  DrawYGrid(hdc, rc, 10/TASKSPEEDMODIFY, 0, STYLE_THINDASHPAPER,
            10, true);

  DrawLineGraph(hdc, rc, &flightstats.Task_Speed,
                STYLE_MEDIUMBLACK);

  DrawTrend(hdc, rc, &flightstats.Task_Speed, STYLE_BLUETHIN);

  DrawXLabel(hdc, rc, TEXT("t"));
  DrawYLabel(hdc, rc, TEXT("V"));

}



void Statistics::RenderClimb(HDC hdc, const RECT rc) 
{

  if (flightstats.ThermalAverage.sum_n<1) {
    DrawNoData(hdc, rc);
    return;
  }

  ResetScale();
  ScaleYFromData(rc, &flightstats.ThermalAverage);
  ScaleYFromValue(rc, (MACCREADY+0.5));
  ScaleYFromValue(rc, 0);

  ScaleXFromValue(rc, -1);
  ScaleXFromValue(rc, flightstats.ThermalAverage.sum_n);

  DrawYGrid(hdc, rc, 
            1.0/LIFTMODIFY, 0,
            STYLE_THINDASHPAPER, 1.0, true);

  DrawBarChart(hdc, rc,
               &flightstats.ThermalAverage);

  DrawLine(hdc, rc,
           0, MACCREADY, 
           flightstats.ThermalAverage.sum_n,
           MACCREADY,
           STYLE_REDTHICK);

  DrawLabel(hdc, rc, TEXT("MC"), 
	    max(0.5, (double)flightstats.ThermalAverage.sum_n-1), MACCREADY);
  
  DrawTrendN(hdc, rc,
             &flightstats.ThermalAverage,
             STYLE_BLUETHIN);

  DrawXLabel(hdc, rc, TEXT("n"));
  DrawYLabel(hdc, rc, TEXT("w"));

}


void Statistics::RenderGlidePolar(HDC hdc, const RECT rc)
{
  int i;

  ResetScale();
  ScaleYFromValue(rc, 0);
  ScaleYFromValue(rc, GlidePolar::SinkRateFast(0,(int)(SAFTEYSPEED-1))*1.1);
  ScaleXFromValue(rc, GlidePolar::Vminsink*0.8);
  ScaleXFromValue(rc, SAFTEYSPEED+2);

  DrawXGrid(hdc, rc, 
            10.0/SPEEDMODIFY, 0,
            STYLE_THINDASHPAPER, 10.0, true);
  DrawYGrid(hdc, rc, 
            1.0/LIFTMODIFY, 0,
            STYLE_THINDASHPAPER, 1.0, true);
  
  double sinkrate0, sinkrate1;
  double v0=0, v1;
  bool v0valid = false;
  int i0=0;

  for (i= GlidePolar::Vminsink; i< SAFTEYSPEED-1;
       i++) {
    
    sinkrate0 = GlidePolar::SinkRateFast(0,i);
    sinkrate1 = GlidePolar::SinkRateFast(0,i+1);
    DrawLine(hdc, rc,
             i, sinkrate0 , 
             i+1, sinkrate1, 
             STYLE_MEDIUMBLACK);

    if (CALCULATED_INFO.AverageClimbRateN[i]>0) {
      v1= CALCULATED_INFO.AverageClimbRate[i]
        /CALCULATED_INFO.AverageClimbRateN[i];
      
      if (v0valid) {

        DrawLine(hdc, rc,
                 i0, v0 , 
                 i, v1, 
                 STYLE_DASHGREEN);


      }
     
      v0 = v1; i0 = i;
      v0valid = true;
    } 

  }

  double ff = SAFTEYSPEED/max(1.0, CALCULATED_INFO.VMacCready);
  double sb = GlidePolar::SinkRate(CALCULATED_INFO.VMacCready);
  ff= (sb-MACCREADY)/max(1.0, CALCULATED_INFO.VMacCready);
  DrawLine(hdc, rc,
           0, MACCREADY, 
           SAFTEYSPEED,
           MACCREADY+ff*SAFTEYSPEED,
           STYLE_REDTHICK);

  DrawXLabel(hdc, rc, TEXT("V"));
  DrawYLabel(hdc, rc, TEXT("w"));

  TCHAR text[80];
  SetBkMode(hdc, OPAQUE);

  _stprintf(text,TEXT("%s %.0f kg"),  
	gettext(TEXT("_@M814_")), // Weight
	    GlidePolar::GetAUW());
  ExtTextOut(hdc, rc.left+IBLSCALE(30), 
	     rc.bottom-IBLSCALE(55), 
	     ETO_OPAQUE, NULL, text, _tcslen(text), NULL);

  _stprintf(text,TEXT("%s %.1f kg/m2"),  
	gettext(TEXT("_@M821_")), // Wing load
	    GlidePolar::WingLoading);
  ExtTextOut(hdc, rc.left+IBLSCALE(30), 
	     rc.bottom-IBLSCALE(40), 
	     ETO_OPAQUE, NULL, text, _tcslen(text), NULL);

  SetBkMode(hdc, TRANSPARENT);
}


void Statistics::ScaleMakeSquare(const RECT rc) {
  if (y_max-y_min<=0) return;
  if (rc.bottom-rc.top-BORDER_Y<=0) return;
  double ar = ((double)(rc.right-rc.left-BORDER_X))
    /(rc.bottom-rc.top-BORDER_Y);
  double ard = (x_max-x_min)/(y_max-y_min);
  double armod = ard/ar;
  double delta;

  if (armod<1.0) {
    // need to expand width
    delta = (x_max-x_min)*(1.0/armod-1.0);
    x_max += delta/2.0;
    x_min -= delta/2.0;
  } else {
    // need to expand height
    delta = (y_max-y_min)*(armod-1.0);
    y_max += delta/2.0;
    y_min -= delta/2.0;
  }
  // shrink both by 10%
  delta = (x_max-x_min)*(1.1-1.0);
  x_max += delta/2.0;
  x_min -= delta/2.0;
  delta = (y_max-y_min)*(1.1-1.0);
  y_max += delta/2.0;
  y_min -= delta/2.0;

  yscale = (rc.bottom-rc.top-BORDER_Y)/(y_max-y_min);
  xscale = (rc.right-rc.left-BORDER_X)/(x_max-x_min);
}


void Statistics::RenderTask(HDC hdc, const RECT rc, const bool olcmode)
{
  int i;

  double lat1 = 0;
  double lon1 = 0;
  double lat2 = 0;
  double lon2 = 0;
  double x1, y1, x2=0, y2=0;
  double lat_c, lon_c;
  double aatradius[MAXTASKPOINTS];

  // find center
  ResetScale();

  if ( (!ValidTaskPoint(0) || !ValidTaskPoint(1)) && !olcmode) {
	DrawNoData(hdc,rc);
	return;
  }

  for (i=0; i<MAXTASKPOINTS; i++) {
    aatradius[i]=0;
  }
  bool nowaypoints = true;
  for (i=0; i<MAXTASKPOINTS; i++) {
    if (ValidTaskPoint(i)) {
      lat1 = WayPointList[Task[i].Index].Latitude;
      lon1 = WayPointList[Task[i].Index].Longitude;
      ScaleYFromValue(rc, lat1);
      ScaleXFromValue(rc, lon1);
      nowaypoints = false;
    }
  }
  if (nowaypoints && !olcmode) {
    DrawNoData(hdc, rc);
    return;
  }

  CPointGPSArray trace;
  CContestMgr::Instance().Trace(trace);
  for(unsigned i=0; i<trace.size(); i++) {
    lat1 = trace[i].Latitude();
    lon1 = trace[i].Longitude();
    ScaleYFromValue(rc, lat1);
    ScaleXFromValue(rc, lon1);
  }

  lat_c = (y_max+y_min)/2;
  lon_c = (x_max+x_min)/2;

  int nwps = 0;

  // find scale
  ResetScale();

  lat1 = GPS_INFO.Latitude;
  lon1 = GPS_INFO.Longitude;
  x1 = (lon1-lon_c)*fastcosine(lat1);
  y1 = (lat1-lat_c);
  ScaleXFromValue(rc, x1);
  ScaleYFromValue(rc, y1);

  for (i=0; i<MAXTASKPOINTS; i++) {
    if (ValidTaskPoint(i)) {
      nwps++;
      lat1 = WayPointList[Task[i].Index].Latitude;
      lon1 = WayPointList[Task[i].Index].Longitude;
      x1 = (lon1-lon_c)*fastcosine(lat1);
      y1 = (lat1-lat_c);
      ScaleXFromValue(rc, x1);
      ScaleYFromValue(rc, y1);

      if (AATEnabled) {
	double aatlat;
	double aatlon;
	double bearing;
	double radius;

        if (ValidTaskPoint(i+1)) {
          if (Task[i].AATType == SECTOR) {
            radius = Task[i].AATSectorRadius;
          } else {
            radius = Task[i].AATCircleRadius;
          }
          for (int j=0; j<4; j++) {
            bearing = j*360.0/4;
            
            FindLatitudeLongitude(WayPointList[Task[i].Index].Latitude,
                                  WayPointList[Task[i].Index].Longitude, 
                                  bearing, radius,
                                  &aatlat, &aatlon);
            x1 = (aatlon-lon_c)*fastcosine(aatlat);
            y1 = (aatlat-lat_c);
            ScaleXFromValue(rc, x1);
            ScaleYFromValue(rc, y1);
            if (j==0) {
              aatradius[i] = fabs(aatlat-WayPointList[Task[i].Index].Latitude);
            }
          }
        } else {
          aatradius[i] = 0;
        }
      }
    }
  }
  for(unsigned i=0; i<trace.size(); i++) {
    lat1 = trace[i].Latitude();
    lon1 = trace[i].Longitude();
    x1 = (lon1-lon_c)*fastcosine(lat1);
    y1 = (lat1-lat_c);
    ScaleXFromValue(rc, x1);
    ScaleYFromValue(rc, y1);
  }

  ScaleMakeSquare(rc);

  DrawXGrid(hdc, rc, 
            1.0, 0,
            STYLE_THINDASHPAPER, 1.0, false);
  DrawYGrid(hdc, rc, 
            1.0, 0,
            STYLE_THINDASHPAPER, 1.0, false);

  // draw aat areas
  if (!olcmode) {
    if (AATEnabled) {
      for (i=MAXTASKPOINTS-1; i>0; i--) {
	if (ValidTaskPoint(i)) {
	  lat1 = WayPointList[Task[i-1].Index].Latitude;
	  lon1 = WayPointList[Task[i-1].Index].Longitude;
	  lat2 = WayPointList[Task[i].Index].Latitude;
	  lon2 = WayPointList[Task[i].Index].Longitude;
	  x1 = (lon1-lon_c)*fastcosine(lat1);
	  y1 = (lat1-lat_c);
	  x2 = (lon2-lon_c)*fastcosine(lat2);
	  y2 = (lat2-lat_c);
	  
	  SelectObject(hdc, 
		       MapWindow::GetAirspaceBrushByClass(AATASK));
	  SelectObject(hdc, GetStockObject(WHITE_PEN));
	  if (Task[i].AATType == SECTOR) {
	    Segment(hdc,
		    (long)((x2-x_min)*xscale+rc.left+BORDER_X),
		    (long)((y_max-y2)*yscale+rc.top),
		    (long)(aatradius[i]*yscale), 
		    rc, 
		    Task[i].AATStartRadial, 
		    Task[i].AATFinishRadial); 
	  } else {
	    Circle(hdc,
		   (long)((x2-x_min)*xscale+rc.left+BORDER_X),
		   (long)((y_max-y2)*yscale+rc.top),
		   (long)(aatradius[i]*yscale), 
		   rc);
	  }
	}
      }
    }
  }

  // draw track
  for(unsigned i=0; trace.size() && i<trace.size()-1; i++) {
    lat1 = trace[i].Latitude();
    lon1 = trace[i].Longitude();
    lat2 = trace[i+1].Latitude();
    lon2 = trace[i+1].Longitude();
    x1 = (lon1-lon_c)*fastcosine(lat1);
    y1 = (lat1-lat_c);
    x2 = (lon2-lon_c)*fastcosine(lat2);
    y2 = (lat2-lat_c);
    DrawLine(hdc, rc,
	     x1, y1, x2, y2,
	     STYLE_MEDIUMBLACK);
  }

  // draw task lines and labels

  if (!olcmode) {
    for (i=MAXTASKPOINTS-1; i>0; i--) {
      if (ValidTaskPoint(i) && ValidTaskPoint(i-1)) {
        lat1 = WayPointList[Task[i-1].Index].Latitude;
	lon1 = WayPointList[Task[i-1].Index].Longitude;
		if (!ValidTaskPoint(1) ) {
		  lat2 = GPS_INFO.Latitude;
		  lon2 = GPS_INFO.Longitude;
		} else {
		  lat2 = WayPointList[Task[i].Index].Latitude;
		  lon2 = WayPointList[Task[i].Index].Longitude;
		}
	x1 = (lon1-lon_c)*fastcosine(lat1);
	y1 = (lat1-lat_c);
	x2 = (lon2-lon_c)*fastcosine(lat2);
	y2 = (lat2-lat_c);
	
	DrawLine(hdc, rc,
		 x1, y1, x2, y2,
		 STYLE_DASHGREEN);
	
	TCHAR text[100];
	if ((i==nwps-1) && (Task[i].Index == Task[0].Index)) {
	  _stprintf(text,TEXT("%0d"),1);
	  DrawLabel(hdc, rc, text, x2, y2);
	} else {
	  _stprintf(text,TEXT("%0d"),i+1);
	  DrawLabel(hdc, rc, text, x2, y2);
	}
	
	if ((i==ActiveWayPoint)&&(!AATEnabled)) {
	  lat1 = GPS_INFO.Latitude;
	  lon1 = GPS_INFO.Longitude;
	  x1 = (lon1-lon_c)*fastcosine(lat1);
	  y1 = (lat1-lat_c);
	  DrawLine(hdc, rc,
		   x1, y1, x2, y2,
		   STYLE_REDTHICK);
	}
	
      }
    }

    // draw aat task line 
    
    if (AATEnabled) {
      for (i=MAXTASKPOINTS-1; i>0; i--) {
	if (ValidTaskPoint(i) && ValidTaskPoint(i-1)) {
          if (i==1) {
            lat1 = WayPointList[Task[i-1].Index].Latitude;
            lon1 = WayPointList[Task[i-1].Index].Longitude;
          } else {
            lat1 = Task[i-1].AATTargetLat;
            lon1 = Task[i-1].AATTargetLon;
          }
          lat2 = Task[i].AATTargetLat;
          lon2 = Task[i].AATTargetLon;

          /*	  
	  if (i==ActiveWayPoint) {
	    lat1 = GPS_INFO.Latitude;
	    lon1 = GPS_INFO.Longitude;
	  }
          */

	  x1 = (lon1-lon_c)*fastcosine(lat1);
	  y1 = (lat1-lat_c);
	  x2 = (lon2-lon_c)*fastcosine(lat2);
	  y2 = (lat2-lat_c);
	  
	  DrawLine(hdc, rc,
		   x1, y1, x2, y2,
		   STYLE_REDTHICK);
	}
      }
    }
  }
  
  if(olcmode) {
    CContestMgr::CResult result = CContestMgr::Instance().Result(contestType, true);
    if(result.Type() == contestType) {
      const CPointGPSArray &points = result.PointArray();
      for(unsigned i=0; i<points.size()-1; i++) {
        lat1 = points[i].Latitude();
        lon1 = points[i].Longitude();
        lat2 = points[i+1].Latitude();
        lon2 = points[i+1].Longitude();
        x1 = (lon1-lon_c)*fastcosine(lat1);
        y1 = (lat1-lat_c);
        x2 = (lon2-lon_c)*fastcosine(lat2);
        y2 = (lat2-lat_c);
        int style = STYLE_REDTHICK;
        if((result.Type() == CContestMgr::TYPE_OLC_FAI ||
            result.Type() == CContestMgr::TYPE_OLC_FAI_PREDICTED) &&
           (i==0 || i==3)) {
          // triangle start and finish
          style = STYLE_DASHGREEN;
        }
        else if(result.Predicted() &&
                (result.Type() == CContestMgr::TYPE_OLC_FAI_PREDICTED ||
                 i == points.size() - 2)) {
          // predicted edge
          style = STYLE_BLUETHIN;
        }
        DrawLine(hdc, rc, x1, y1, x2, y2, style);
      }
      if(result.Type() == CContestMgr::TYPE_OLC_FAI ||
         result.Type() == CContestMgr::TYPE_OLC_FAI_PREDICTED) {
        // draw the last edge of a triangle
        lat1 = points[1].Latitude();
        lon1 = points[1].Longitude();
        lat2 = points[3].Latitude();
        lon2 = points[3].Longitude();
        x1 = (lon1-lon_c)*fastcosine(lat1);
        y1 = (lat1-lat_c);
        x2 = (lon2-lon_c)*fastcosine(lat2);
        y2 = (lat2-lat_c);
        DrawLine(hdc, rc, x1, y1, x2, y2, result.Predicted() ? STYLE_BLUETHIN : STYLE_REDTHICK);
      }
    }
  }

  // Draw aircraft on top
  lat1 = GPS_INFO.Latitude;
  lon1 = GPS_INFO.Longitude;
  x1 = (lon1-lon_c)*fastcosine(lat1);
  y1 = (lat1-lat_c);
  DrawLabel(hdc, rc, TEXT("+"), x1, y1);
}


void Statistics::RenderTemperature(HDC hdc, const RECT rc)
{
  ResetScale();

  int i;
  float hmin= 10000;
  float hmax= -10000;
  float tmin= (float)CuSonde::maxGroundTemperature;
  float tmax= (float)CuSonde::maxGroundTemperature;

  // find range for scaling of graph
  for (i=0; i<CUSONDE_NUMLEVELS-1; i++) {
    if (CuSonde::cslevels[i].nmeasurements) {

      hmin = min(hmin, (float)i);
      hmax = max(hmax, (float)i);
      tmin = min(tmin, (float)min(CuSonde::cslevels[i].tempDry,
			   (double)min(CuSonde::cslevels[i].airTemp,
                                      (double)CuSonde::cslevels[i].dewpoint)));
      tmax = max(tmax, (float)max(CuSonde::cslevels[i].tempDry,
			   (double)max(CuSonde::cslevels[i].airTemp,
			       (double)CuSonde::cslevels[i].dewpoint)));
    }
  }
  if (hmin>= hmax) {
    DrawNoData(hdc, rc);
    return;
  }

  ScaleYFromValue(rc, hmin);
  ScaleYFromValue(rc, hmax);
  ScaleXFromValue(rc, tmin);
  ScaleXFromValue(rc, tmax);

  bool labelDry = false;
  bool labelAir = false;
  bool labelDew = false;

  int ipos = 0;

  for (i=0; i<CUSONDE_NUMLEVELS-1; i++) {

    if (CuSonde::cslevels[i].nmeasurements &&
	CuSonde::cslevels[i+1].nmeasurements) {

      ipos++;

      DrawLine(hdc, rc,
	       CuSonde::cslevels[i].tempDry, i,
	       CuSonde::cslevels[i+1].tempDry, (i+1), 
	       STYLE_REDTHICK);

      DrawLine(hdc, rc,
	       CuSonde::cslevels[i].airTemp, i,
	       CuSonde::cslevels[i+1].airTemp, (i+1), 
	       STYLE_MEDIUMBLACK);

      DrawLine(hdc, rc,
	       CuSonde::cslevels[i].dewpoint, i,
	       CuSonde::cslevels[i+1].dewpoint, i+1, 
	       STYLE_BLUETHIN);

      if (ipos> 2) {
	if (!labelDry) {
	  DrawLabel(hdc, rc, TEXT("DALR"), 
		    CuSonde::cslevels[i+1].tempDry, i);
	  labelDry = true;
	} else {
	  if (!labelAir) {
	    DrawLabel(hdc, rc, TEXT("Air"), 
		      CuSonde::cslevels[i+1].airTemp, i);
	    labelAir = true;
	  } else {
	    if (!labelDew) {
	      DrawLabel(hdc, rc, TEXT("Dew"), 
			CuSonde::cslevels[i+1].dewpoint, i);
	      labelDew = true;
	    }
	  }
	}
      }
    }
  }

  DrawXLabel(hdc, rc, TEXT("T")TEXT(DEG));
  DrawYLabel(hdc, rc, TEXT("h"));
}


// from Calculations.cpp
#include "windanalyser.h"
extern WindAnalyser *windanalyser;

void Statistics::RenderWind(HDC hdc, const RECT rc) 
{
  int numsteps=10;
  int i;
  double h;
  Vector wind;
  bool found=true;
  double mag;

  LeastSquares windstats_mag;

  if (flightstats.Altitude_Ceiling.y_max
      -flightstats.Altitude_Ceiling.y_min<=10) {
    DrawNoData(hdc, rc);
    return;
  }

  for (i=0; i<numsteps ; i++) {

    h = (flightstats.Altitude_Ceiling.y_max-flightstats.Altitude_Base.y_min)*
      i/(double)(numsteps-1)+flightstats.Altitude_Base.y_min; 

    wind = windanalyser->windstore.getWind(GPS_INFO.Time, h, &found);
    mag = sqrt(wind.x*wind.x+wind.y*wind.y);

    windstats_mag.least_squares_update(mag, h);

  }

  //

  ResetScale();

  ScaleXFromData(rc, &windstats_mag);
  ScaleXFromValue(rc, 0);
  ScaleXFromValue(rc, 10.0);

  ScaleYFromData(rc, &windstats_mag);

  DrawXGrid(hdc, rc, 5/SPEEDMODIFY, 0, STYLE_THINDASHPAPER, 5.0, true);
  DrawYGrid(hdc, rc, 1000/ALTITUDEMODIFY, 0, STYLE_THINDASHPAPER,
            1000.0, true);

  DrawLineGraph(hdc, rc, &windstats_mag,
                STYLE_MEDIUMBLACK);

#define WINDVECTORMAG 25

  numsteps = (int)((rc.bottom-rc.top)/WINDVECTORMAG)-1;

  // draw direction vectors

  POINT wv[4];
  double dX, dY;
  double angle;
  double hfact;
  for (i=0; i<numsteps ; i++) {
    hfact = (i+1)/(double)(numsteps+1);
    h = (flightstats.Altitude_Ceiling.y_max-flightstats.Altitude_Base.y_min)*
      hfact+flightstats.Altitude_Base.y_min;

    wind = windanalyser->windstore.getWind(GPS_INFO.Time, h, &found);
    if (windstats_mag.x_max == 0)
      windstats_mag.x_max=1;  // prevent /0 problems
    wind.x /= windstats_mag.x_max;
    wind.y /= windstats_mag.x_max;
    mag = sqrt(wind.x*wind.x+wind.y*wind.y);
    if (mag<= 0.0) continue;

    angle = atan2(wind.x,-wind.y)*RAD_TO_DEG;

    wv[0].y = (int)((1-hfact)*(rc.bottom-rc.top-BORDER_Y))+rc.top;
    wv[0].x = (rc.right+rc.left-BORDER_X)/2+BORDER_X;

    dX = (mag*WINDVECTORMAG);
    dY = 0;
    rotate(dX,dY,angle);
    wv[1].x = (int)(wv[0].x + dX);
    wv[1].y = (int)(wv[0].y + dY);
    StyleLine(hdc, wv[0], wv[1], STYLE_MEDIUMBLACK, rc);

    dX = (mag*WINDVECTORMAG-5);
    dY = -3;
    rotate(dX,dY,angle);
    wv[2].x = (int)(wv[0].x + dX);
    wv[2].y = (int)(wv[0].y + dY);
    StyleLine(hdc, wv[1], wv[2], STYLE_MEDIUMBLACK, rc);

    dX = (mag*WINDVECTORMAG-5);
    dY = 3;
    rotate(dX,dY,angle);
    wv[3].x = (int)(wv[0].x + dX);
    wv[3].y = (int)(wv[0].y + dY);

    StyleLine(hdc, wv[1], wv[3], STYLE_MEDIUMBLACK, rc);

  }

  DrawXLabel(hdc, rc, TEXT("w"));
  DrawYLabel(hdc, rc, TEXT("h"));

}


static double fMaxAltToday=3300.0f;
int Statistics::CalcHeightCoordinat(double fHeight, const RECT rc)
{
  int y0 = rc.bottom-BORDER_Y;
  double alt = GPS_INFO.Altitude;
//  if(alt > fMaxAltToday)
//    fMaxAltToday = round( alt*1.5f/100.0f)*100.0f;
  fMaxAltToday = 3300;
  double hmin = max(0.0, alt-2300);
  double hmax = max(fMaxAltToday, alt+1000);
  double gfh = (fHeight-hmin)/(hmax-hmin);
  int yPos = (int)(gfh*(rc.top-rc.bottom+BORDER_Y)+y0)-1;

  return yPos;
//  fHeigh
}


bool PtInRect(int X,int Y, RECT rcd )
{
  if( X  > rcd.left   )
    if( X  < rcd.right  )
      if( Y  > rcd.bottom )
        if( Y  < rcd.top    )
          return true;
  return false;
}



void Statistics::RenderAirspace(HDC hdc, const RECT rc) {

  double range = 50.0*1000; // km
  double aclat, aclon, ach, acb, alt, speed, calc_average30s;
  double fi, fj;
  double wpt_dist;
  double wpt_altarriv;
  double wpt_altitude;
  double wpt_altarriv_mc0;
  double fMC0 = 0.0f;
  int overindex=-1;
  bool show_mc0= true;
  
  SIZE tsize;
  TCHAR text[80];
  TCHAR buffer[80];
  BOOL bDrawRightSide =false;

  LockFlightData();
  fMC0 = GlidePolar::SafetyMacCready;
  aclat = GPS_INFO.Latitude;
  aclon = GPS_INFO.Longitude;
  ach = GPS_INFO.Altitude;
  acb = GPS_INFO.TrackBearing;

  speed = GPS_INFO.Speed;
  calc_average30s = CALCULATED_INFO.Average30s;

  if (GPS_INFO.BaroAltitudeAvailable &&
		  EnableNavBaroAltitude) {
    alt = GPS_INFO.BaroAltitude;
  } else {
    alt = GPS_INFO.Altitude;
  }



  overindex=GetOvertargetIndex();
  double  wpt_brg =  AngleLimit360( GPS_INFO.TrackBearing );
  wpt_dist         = 0.0;
  wpt_altarriv     = 0.0;
  wpt_altarriv_mc0 = 0.0;
  wpt_altitude     = 0.0;
  fMC0 = 0.0;

  if (asp_heading_task) {
    // Show towards target
    if (overindex>=0) {
      double wptlon = WayPointList[overindex].Longitude;
      double wptlat = WayPointList[overindex].Latitude;
      DistanceBearing(aclat, aclon, wptlat, wptlon, &wpt_dist, &acb);

      wpt_brg =AngleLimit360(wpt_brg - acb +90);
      range = max(5.0*1000.0, wpt_dist*1.25);   // 20% more distance to show, minimum 5km
      wpt_altarriv     = WayPointCalc[overindex].AltArriv[ALTA_MC ];
      wpt_altarriv_mc0 = WayPointCalc[overindex].AltArriv[ALTA_MC0];
      wpt_altitude     = WayPointList[overindex].Altitude;
       // calculate the MC=0 arrival altitude

//if( wpt_altarriv_mc0 >3300) /* BUGFIX calc not calculated somewhere else (init value 3333m indicates not calculated ) */
// ToDo remove this function when safety Mc als is calculated in calc thread
{
      wpt_altarriv_mc0 =   CALCULATED_INFO.NavAltitude -
        GlidePolar::MacCreadyAltitude( fMC0,
                                       wpt_dist,
                                       acb,
                                       CALCULATED_INFO.WindSpeed,
                                       CALCULATED_INFO.WindBearing,
                                       0, 0, true,
                                       0)  - WayPointList[overindex].Altitude;
      if (IsSafetyAltitudeInUse(overindex)) wpt_altarriv_mc0 -= SAFETYALTITUDEARRIVAL;


      wpt_altarriv =   CALCULATED_INFO.NavAltitude -
        GlidePolar::MacCreadyAltitude( MACCREADY,
                                       wpt_dist,
                                       acb,
                                       CALCULATED_INFO.WindSpeed,
                                       CALCULATED_INFO.WindBearing,
                                       0, 0, true,
                                       0)  - WayPointList[overindex].Altitude;
      if (IsSafetyAltitudeInUse(overindex)) wpt_altarriv -= SAFETYALTITUDEARRIVAL;
}

    } else {
      // no selected target
      DrawNoData(hdc, rc);
      return;
    }
  }
  UnlockFlightData();
  
  double hmin = max(0.0, alt-2300);
  double hmax = max(fMaxAltToday, alt+1000);
  RECT rcd;

  double d_lat[AIRSPACE_SCANSIZE_X];
  double d_lon[AIRSPACE_SCANSIZE_X];
  double d_alt[AIRSPACE_SCANSIZE_X];
  double d_h[AIRSPACE_SCANSIZE_H];
  AirSpaceSideViewSTRUCT d_airspace[AIRSPACE_SCANSIZE_H][AIRSPACE_SCANSIZE_X];


  int i,j,k;

#define FRACT 0.75
  RasterTerrain::Lock();
  // want most accurate rounding here
  RasterTerrain::SetTerrainRounding(0,0);

  for (j=0; j< AIRSPACE_SCANSIZE_X; j++) { // scan range
    fj = j*1.0/(AIRSPACE_SCANSIZE_X-1);
    FindLatitudeLongitude(aclat, aclon, acb, range*fj,
                          &d_lat[j], &d_lon[j]);
    d_alt[j] = RasterTerrain::GetTerrainHeight(d_lat[j], d_lon[j]);
    if (d_alt[j] == TERRAIN_INVALID) d_alt[j]=0; //@ 101027 BUGFIX
    hmax = max(hmax, d_alt[j]);
  }
  RasterTerrain::Unlock();

  double fh = (ach-hmin)/(hmax-hmin);

  ResetScale();
  ScaleXFromValue(rc, 0);
  ScaleXFromValue(rc, range);
  ScaleYFromValue(rc, hmin);
  ScaleYFromValue(rc, hmax);

  double dfi = 1.0/(AIRSPACE_SCANSIZE_H-1);
  double dfj = 1.0/(AIRSPACE_SCANSIZE_X-1);

  for (i=0; i< AIRSPACE_SCANSIZE_H; i++) { // scan height
    d_h[i] = (hmax-hmin)*i*dfi+hmin;
  }
  for (i=0; i< AIRSPACE_SCANSIZE_H; i++) { // scan height
    for (j=0; j< AIRSPACE_SCANSIZE_X; j++) { // scan range
      d_airspace[i][j].iType =  -1; // no airspace
    }
  }
  CAirspaceManager::Instance().ScanAirspaceLine(d_lat, d_lon, d_h, d_alt, d_airspace);
  int type;

  double dx = dfj*(rc.right-rc.left-BORDER_X);
  int x0 = rc.left+BORDER_X;
  double dy = dfi*(rc.top-rc.bottom+BORDER_Y);
  int y0 = rc.bottom-BORDER_Y;

  BOOL bFound = false;
  for ( k=0 ; k < GC_MAX_NO; k++)
  {
	wsprintf(   asDrawn[k].szAS_Name, TEXT(""));
	asDrawn[k].aiLable = ID_NO_LABLE;
	asDrawn[k].iType   = -1;
	asDrawn[k].iIdx    = -1;
	asDrawn[k].psAS    = NULL;
    wsprintf( pHandeled[k].szAS_Name, TEXT(""));
    pHandeled[k].aiLable = ID_NO_LABLE;
    pHandeled[k].iType   = -1;
    pHandeled[k].iIdx    = -1;
    pHandeled[k].psAS    = NULL;

  }
  int iNoOfDrawnNames =0;
  bool bDrawn = false;


  HPEN mpen = (HPEN)CreatePen(PS_NULL, 0, RGB(0xf0,0xf0,0xb0));
  HPEN oldpen = (HPEN)SelectObject(hdc, (HPEN)mpen);

  iNohandeldSpaces=0;
  for (i=0; i< AIRSPACE_SCANSIZE_H; i++)
  { // scan height
    fi = i*dfi;
    for (j=0; j< AIRSPACE_SCANSIZE_X; j++)
    { // scan range
      fj = j*dfj;
      type = d_airspace[i][j].iType;
      {
        if (type>=0)
        {
	      rcd.left = iround((j-FRACT)*dx)+x0;
	      rcd.right = iround((j+FRACT)*dx)+x0;
	      rcd.bottom = iround(((i)+FRACT)*dy)+y0;
	      rcd.top = iround(((i)-FRACT)*dy)+y0;
		  bFound = false;
          if(rcd.top > y0)
        	rcd.top = y0;

		  if(d_airspace[i][j].bRectAllowed == false)
		  {
		    SelectObject(hdc, MapWindow::GetAirspaceBrushByClass(type));
		    SetTextColor(hdc, MapWindow::GetAirspaceColourByClass(type));
	        Rectangle(hdc,rcd.left,rcd.top,rcd.right,rcd.bottom);
		  }

          for (k=0 ; k < iNohandeldSpaces; k++)
          {
            if(k < GC_MAX_NO)
            {
			  if(  pHandeled[k].iIdx  ==  d_airspace[i][j].iIdx )
			  {
			    bFound = true;
			    if( rcd.left   < pHandeled[k].rc.left  )  pHandeled[k].rc.left    = rcd.left;
			    if( rcd.right  > pHandeled[k].rc.right )  pHandeled[k].rc.right   = rcd.right;
			    if( rcd.bottom < pHandeled[k].rc.bottom)  pHandeled[k].rc.bottom  = rcd.bottom;
			    if( rcd.top    > pHandeled[k].rc.top   )  pHandeled[k].rc.top     = rcd.top;
			  }
            }
          }
	      if(!bFound)
	      {
	        if(iNohandeldSpaces < GC_MAX_NO)
	        {
		      pHandeled[iNohandeldSpaces].iType =  d_airspace[i][j].iType;
		      pHandeled[iNohandeldSpaces].iIdx  =  d_airspace[i][j].iIdx;
		      pHandeled[iNohandeldSpaces].bRectAllowed =  d_airspace[i][j].bRectAllowed;
		      pHandeled[iNohandeldSpaces].psAS   = d_airspace[i][j].psAS;
		      pHandeled[iNohandeldSpaces].rc    =  rcd;
			  _tcsncpy(pHandeled[iNohandeldSpaces].szAS_Name, d_airspace[i][j].szAS_Name, NAME_SIZE-1);
		      iNohandeldSpaces++;
	        }
	      }
        }
      }
    }
  }



  SelectObject(hdc, (HPEN)mpen);
  for (k=0 ; k < iNohandeldSpaces; k++)
  {
	if( pHandeled[k].iIdx != -1)
    {
#ifdef DDd
	  if((X_PUSH_POS!=-1) && (Y_PUSH_POS!=-1))
	  {
		 if (PtInRect(X_PUSH_POS,Y_PUSH_POS,pHandeled[k].rc ))
		 {
				PlayResource(TEXT("IDR_WAV_BTONE4"));
		//   if (EnableSoundModes) PlayResource(TEXT("IDR_WAV_CLICK"));
			 dlgAirspaceDetails(pHandeled[k].psAS);


			 X_PUSH_POS=-1; Y_PUSH_POS=-1;
		 }
	  }
#endif
	  if(pHandeled[k].bRectAllowed == true)
	  {
        int  type = pHandeled[k].iType;
	    SelectObject(hdc, MapWindow::GetAirspaceBrushByClass(type));
	    SetTextColor(hdc, MapWindow::GetAirspaceColourByClass(type));
        RECT rcd =pHandeled[k].rc;
        Rectangle(hdc,rcd.left,rcd.top,rcd.right,rcd.bottom);
	  }
#ifdef FRAMES
      SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
      HPEN Newpen = (HPEN)CreatePen(PS_SOLID, 1, MapWindow::GetAirspaceColourByClass(type));
      SelectObject(hdc, (Newpen));
	Rectangle(hdc,rcd.left,rcd.top,rcd.right,rcd.bottom);
#endif
    }
  }
  //

  // draw ground
  POINT ground[4];
  HPEN   hpHorizonGround;
  HBRUSH hbHorizonGround;
  int itemp;
  hpHorizonGround = (HPEN)CreatePen(PS_SOLID, IBLSCALE(1), GROUND_COLOUR);
  hbHorizonGround = (HBRUSH)CreateSolidBrush(GROUND_COLOUR);
  SelectObject(hdc, hpHorizonGround);
  SelectObject(hdc, hbHorizonGround);

  for (j=1; j< AIRSPACE_SCANSIZE_X; j++) { // scan range
    ground[0].x = iround((j-1)*dx)+x0;
    ground[1].x = ground[0].x;
    ground[2].x = iround(j*dx)+x0;
    ground[3].x = ground[2].x;
    ground[0].y = y0;
    itemp = iround((d_alt[j-1]-hmin)/(hmax-hmin)*(rc.top-rc.bottom+BORDER_Y))+y0;
    if (itemp>y0) itemp = y0;
    ground[1].y = itemp;
    itemp = iround((d_alt[j]-hmin)/(hmax-hmin)*(rc.top-rc.bottom+BORDER_Y))+y0;
    if (itemp>y0) itemp = y0;
    ground[2].y = itemp;
    ground[3].y = y0;
    if ((ground[1].y == y0) && (ground[2].y == y0)) continue;
    Polygon(hdc, ground, 4);
  }


  double xtick = 1.0;
  if (range>10.0*1000.0) xtick = 5.0;
  if (range>50.0*1000.0) xtick = 10.0;
  if (range>100.0*1000.0) xtick = 20.0;
  if (range>200.0*1000.0) xtick = 25.0;
  if (range>250.0*1000.0) xtick = 50.0;
  if (range>500.0*1000.0) xtick = 100.0;
  SelectObject(hdc, GetStockObject(WHITE_PEN));
  SelectObject(hdc, GetStockObject(WHITE_BRUSH));
  SetTextColor(hdc, RGB(0xff,0xff,0xff));
  DrawXGrid(hdc, rc, xtick/DISTANCEMODIFY, 0,  STYLE_THINDASHPAPER, xtick, true);
  DrawYGrid(hdc, rc, 500.0/ALTITUDEMODIFY, 0, STYLE_THINDASHPAPER, 500.0, true);



  HFONT hfOld = (HFONT)SelectObject(hdc, LK8PanelUnitFont); // LK8MapFont
    for (k=0 ; k < iNohandeldSpaces; k++)
    {
  	if( pHandeled[k].iIdx != -1)
      {
        bDrawn = false;
        for (i=0; i < iNoOfDrawnNames; i++)
        {
          if( bDrawn == false)
          {

  	      if(_tcsncmp((TCHAR*)asDrawn[i].szAS_Name, (TCHAR*)pHandeled[k].szAS_Name,NAME_SIZE) == 0)
  //			if(  pHandeled[k].iIdx  ==  asDrawn[i].iIdx )
  	    	  if(pHandeled[k].iType == asDrawn[i].iType)
  	           bDrawn = true;
          }
        }

        if(bDrawn == false)
        {
            int  type = pHandeled[k].iType;
  		  SelectObject(hdc, MapWindow::GetAirspaceBrushByClass(type));
  		  SetTextColor(hdc, RGB_MENUTITLEFG);
  		  RECT rcd =pHandeled[k].rc;

  		  int x = rcd.left + (rcd.right - rcd.left)/2;
  		  int y = rcd.top  - (rcd.top   - rcd.bottom)/2;

  		  _tcsncpy(text, pHandeled[k].szAS_Name,NAME_SIZE-1/* sizeof(text)/sizeof(text[0])*/);
  		  GetTextExtentPoint(hdc, text, _tcslen(text), &tsize);
  		  x -= tsize.cx/2; // - NIBLSCALE(5);
  		  y -= tsize.cy;   // - NIBLSCALE(5);
  		  if( asDrawn[iNoOfDrawnNames].aiLable < ID_FULL_LABLE) /* already drawn ? */
  			if(tsize.cx < (rcd.right-rcd.left))
  			  if((y)  < rcd.top)
  				if((y + tsize.cy)  > rcd.bottom)
  				{

  				   ExtTextOut(hdc, x, y, ETO_OPAQUE, NULL, text, _tcslen(text), NULL);

  				   _tcsncpy((wchar_t*)  asDrawn[iNoOfDrawnNames].szAS_Name, (wchar_t*) pHandeled[k].szAS_Name, NAME_SIZE);
  				   asDrawn[iNoOfDrawnNames].aiLable = ID_FULL_LABLE;
  				   asDrawn[iNoOfDrawnNames].iType = type;
  				   asDrawn[iNoOfDrawnNames].iIdx  = pHandeled[k].iIdx;
  				   iNoOfDrawnNames++;
  				   y = rcd.top  - (rcd.top   - rcd.bottom)/2;
  				}


  		  _tcsncpy((wchar_t*)text, (wchar_t*) CAirspaceManager::Instance().GetAirspaceTypeShortText( pHandeled[k].iType), NAME_SIZE);
  		  GetTextExtentPoint(hdc, text, _tcslen(text), &tsize);
  		  x = rcd.left + (rcd.right - rcd.left)/2;
  		  if( asDrawn[iNoOfDrawnNames].aiLable == ID_NO_LABLE) /* already drawn ? */
  			if(tsize.cx < (rcd.right-rcd.left))
  			  if((y)  < rcd.top)
  				if((y + tsize.cy)  > rcd.bottom)
  				{
  				  x -= tsize.cx/2; // - NIBLSCALE(5);
  				  ExtTextOut(hdc, x, y, ETO_OPAQUE, NULL, text, _tcslen(text), NULL);
  				   _tcsncpy((wchar_t*)  asDrawn[iNoOfDrawnNames].szAS_Name, (wchar_t*) pHandeled[k].szAS_Name, NAME_SIZE);
  				   asDrawn[iNoOfDrawnNames].aiLable = ID_SHORT_LABLE;
  				   asDrawn[iNoOfDrawnNames].iType = type;
  				   asDrawn[iNoOfDrawnNames].iIdx  = pHandeled[k].iIdx;
  				  iNoOfDrawnNames++;
  				}

        }
      }
    }
  SelectObject(hdc, hfOld);

  //
  POINT line[4];

  // draw target symbolic line
  int iWpPos =  (int)(x0 + ((rc.right - rc.left - BORDER_X) / range) * wpt_dist);
  if (asp_heading_task)
  {
    if(WayPointCalc[overindex].IsLandable == 0)
	{
    // Mark wpt with a vertical marker line
    line[0].x = (int)(x0 + ((rc.right - rc.left - BORDER_X) / range) * wpt_dist);
    line[0].y = y0;
    line[1].x = line[0].x;
    line[1].y = rc.top;
    StyleLine(hdc, line[0], line[1], STYLE_WHITETHICK, rc);
  }
	else
    {
      line[0].x = iWpPos;
      line[0].y = CalcHeightCoordinat( wpt_altitude,   rc);
      line[1].x = line[0].x;
      line[1].y = CalcHeightCoordinat( SAFETYALTITUDEARRIVAL+wpt_altitude,   rc);
      StyleLine(hdc, line[0], line[1], STYLE_ORANGETHICK, rc);

      float fArrHight = 0.0f;
      if(wpt_altarriv > 0.0f)
      {
    	fArrHight = wpt_altarriv;
        line[0].x = iWpPos;
        line[0].y = CalcHeightCoordinat( SAFETYALTITUDEARRIVAL+wpt_altitude,   rc);
        line[1].x = line[0].x;
        line[1].y = CalcHeightCoordinat( SAFETYALTITUDEARRIVAL+wpt_altitude+fArrHight,   rc);
        StyleLine(hdc, line[0], line[1], STYLE_GREENTHICK, rc);

      }
      // Mark wpt with a vertical marker line
      line[0].x = iWpPos;
      line[0].y = CalcHeightCoordinat( SAFETYALTITUDEARRIVAL+wpt_altitude+fArrHight,   rc);
      line[1].x = line[0].x;
      line[1].y = rc.top;
      StyleLine(hdc, line[0], line[1], STYLE_WHITETHICK, rc);
    }
  }

  // Draw estimated gliding line (blue)
  if (speed>10.0) {
    if (asp_heading_task) {

         double altarriv;
#define MC0_LINE
#ifdef MC0_LINE
        // Draw estimated gliding line MC=0 (green)
        if( show_mc0 )
        {
          altarriv = wpt_altarriv_mc0 + wpt_altitude;
          if (IsSafetyAltitudeInUse(overindex)) altarriv += SAFETYALTITUDEARRIVAL;
          line[0].x = x0;
          line[0].y = CalcHeightCoordinat( alt,   rc);
          line[1].x = (int)(x0 + ((rc.right - rc.left - BORDER_X) / range) * wpt_dist);
          line[1].y = CalcHeightCoordinat( altarriv ,   rc);
          StyleLine(hdc, line[0], line[1], STYLE_BLUETHIN, rc);
        }
#endif
        altarriv = wpt_altarriv + wpt_altitude;
        if (IsSafetyAltitudeInUse(overindex)) altarriv += SAFETYALTITUDEARRIVAL;
        line[0].x = x0;
        line[0].y = CalcHeightCoordinat( alt,   rc);
        line[1].x = (int)(x0 + ((rc.right - rc.left - BORDER_X) / range) * wpt_dist);
        line[1].y = CalcHeightCoordinat( altarriv ,   rc);
        StyleLine(hdc, line[0], line[1], STYLE_BLUETHIN, rc);

    } else {
      double t = range/speed;
      double gfh = (ach+calc_average30s*t-hmin)/(hmax-hmin);
      line[0].x = x0;
      line[0].y = (int)(fh*(rc.top-rc.bottom+BORDER_Y)+y0)-1;
      line[1].x = rc.right;
      line[1].y = (int)(gfh*(rc.top-rc.bottom+BORDER_Y)+y0)-1;
      StyleLine(hdc, line[0], line[1], STYLE_BLUETHIN, rc);
    }
  }

  SelectObject(hdc, GetStockObject(WHITE_PEN));
  SelectObject(hdc, GetStockObject(WHITE_BRUSH));
  SetTextColor(hdc, RGB(0xff,0xff,0xff));


  //Draw wpt info texts
  if (asp_heading_task) {
    line[0].x = (int)(x0 + ((rc.right - rc.left - BORDER_X) / range) * wpt_dist);



    // Print wpt name next to marker line
    _tcsncpy(text, WayPointList[overindex].Name, sizeof(text)/sizeof(text[0]));
    GetTextExtentPoint(hdc, text, _tcslen(text), &tsize);
    int x = line[0].x - tsize.cx - NIBLSCALE(5);

    if (x<x0)
      bDrawRightSide =true;
    if (bDrawRightSide) x = line[0].x + NIBLSCALE(5);
    int y = rc.top + 3*tsize.cy;

    SetTextColor(hdc, RGB_WHITE);
    ExtTextOut(hdc, x, y, ETO_OPAQUE, NULL, text, _tcslen(text), NULL);

    // Print wpt distance
    Units::FormatUserDistance(wpt_dist, text, 7);
    GetTextExtentPoint(hdc, text, _tcslen(text), &tsize);
    x = line[0].x - tsize.cx - NIBLSCALE(5);
    if (bDrawRightSide) x = line[0].x + NIBLSCALE(5);
    y += tsize.cy;
    ExtTextOut(hdc, x, y, ETO_OPAQUE, NULL, text, _tcslen(text), NULL);
    double altarriv = wpt_altarriv_mc0; // + wpt_altitude;
    if (IsSafetyAltitudeInUse(overindex)) altarriv += SAFETYALTITUDEARRIVAL;


HFONT hfOld = (HFONT)SelectObject(hdc, LK8PanelUnitFont);

    SetTextColor(hdc, RGB_WHITE);
    if (wpt_altarriv_mc0 > ALTDIFFLIMIT)
    {
      _stprintf(text, TEXT("Mc %3.1f: "),fMC0);
      Units::FormatUserArrival(wpt_altarriv_mc0, buffer, 7);
      _tcscat(text,buffer);
    } else {
      _tcsncpy(text, TEXT("---"), sizeof(text)/sizeof(text[0]));
    }
    GetTextExtentPoint(hdc, text, _tcslen(text), &tsize);
    x = line[0].x - tsize.cx - NIBLSCALE(5);
    if (bDrawRightSide) x = line[0].x + NIBLSCALE(5);   // Show on right side if left not possible
    y += tsize.cy;

    if((wpt_altarriv_mc0 > 0) && (  WayPointList[overindex].Reachable))
  	  SetTextColor(hdc, RGB_GREEN);
    else
  	  SetTextColor(hdc, RGB_LIGHTORANGE);
    ExtTextOut(hdc, x, y, ETO_OPAQUE, NULL, text, _tcslen(text), NULL);
    
    // Print arrival altitude
    if (wpt_altarriv > ALTDIFFLIMIT) {
      _stprintf(text, TEXT("Mc %3.1f: "),MACCREADY);

      Units::FormatUserArrival(wpt_altarriv, buffer, 7);
      _tcscat(text,buffer);
    } else {
      _tcsncpy(text, TEXT("---"), sizeof(text)/sizeof(text[0]));
    }


    if(  WayPointList[overindex].Reachable)
  	  SetTextColor(hdc, RGB_GREEN);
    else
  	  SetTextColor(hdc, RGB_LIGHTORANGE);
    GetTextExtentPoint(hdc, text, _tcslen(text), &tsize);
    x = line[0].x - tsize.cx - NIBLSCALE(5);
    if (bDrawRightSide) x = line[0].x + NIBLSCALE(5);   // Show on right side if left not possible
    y += tsize.cy;
    ExtTextOut(hdc, x, y, ETO_OPAQUE, NULL, text, _tcslen(text), NULL);

  	SetTextColor(hdc, RGB_WHITE);
    x = line[0].x + NIBLSCALE(5);   // Show on right side if left not possible


    // Print araival Elevation
    SetTextColor(hdc, RGB_BLACK);
    if(wpt_altitude- hmin > 0)
    {
  	  Units::FormatUserAltitude(wpt_altitude, buffer, 7);
      _tcsncpy(text, gettext(TEXT("_@M1743_")), sizeof(text)/sizeof(text[0]));   // ELV:
      _tcscat(text,buffer);
      GetTextExtentPoint(hdc, text, _tcslen(text), &tsize);
      y = CalcHeightCoordinat(  (wpt_altitude-hmin),   rc);
      if(2.4*tsize.cy < (rc.bottom - y))
        ExtTextOut(hdc, x-tsize.cx/2, rc.bottom -(int)(2.4 * tsize.cy) /* rc.top-tsize.cy*/, ETO_OPAQUE, NULL, text, _tcslen(text), NULL);
    }


    // Print araival AGL
    altarriv = wpt_altarriv;
    if (IsSafetyAltitudeInUse(overindex)) altarriv += SAFETYALTITUDEARRIVAL;
    if(wpt_altarriv  > 10)
    {
  	  Units::FormatUserAltitude(altarriv, buffer, 7);
      _tcsncpy(text, gettext(TEXT("_@M1742_")), sizeof(text)/sizeof(text[0]));
      _tcscat(text,buffer);
      y = CalcHeightCoordinat(  altarriv + wpt_altitude ,   rc);

      if(  WayPointList[overindex].Reachable)
    	SetTextColor(hdc, RGB_GREEN);
      else
    	SetTextColor(hdc, RGB_LIGHTORANGE);
      ExtTextOut(hdc, x, y, ETO_OPAQUE, NULL, text, _tcslen(text), NULL);
    }


	// Print current Terrain height
    x =  tsize.cx / 2 ; // - NIBLSCALE(5);
    SetTextColor(hdc, RGB_BLACK);
	altarriv = CALCULATED_INFO.TerrainAlt;
    if(CALCULATED_INFO.TerrainAlt - hmin > 0)
	{
       Units::FormatUserAltitude(CALCULATED_INFO.TerrainAlt, buffer, 7);
       _tcsncpy(text, gettext(TEXT("_@M1743_")), sizeof(text)/sizeof(text[0]));
       _tcscat(text,buffer);
       GetTextExtentPoint(hdc, text, _tcslen(text), &tsize);
       y = CalcHeightCoordinat(  (CALCULATED_INFO.TerrainAlt-hmin),   rc);
       x = (int)(x0 + ((rc.right - rc.left - BORDER_X) / range) * wpt_dist);
       if(x > (tsize.cx))
    	 if((2.2*tsize.cy) < (rc.bottom - y))
           ExtTextOut(hdc, x0+IBLSCALE(1), rc.bottom - (int) (2.2* tsize.cy), ETO_OPAQUE, NULL, text, _tcslen(text), NULL);
	}


	// Print current AGL
	if(CALCULATED_INFO.AltitudeAGL - hmin > 0)
	{
	  SetTextColor(hdc, RGB_LIGHTBLUE);
	  Units::FormatUserAltitude(CALCULATED_INFO.AltitudeAGL, buffer, 7);
      _tcsncpy(text, gettext(TEXT("_@M1742_")), sizeof(text)/sizeof(text[0]));
      _tcscat(text,buffer);
      GetTextExtentPoint(hdc, text, _tcslen(text), &tsize);
	  y = CalcHeightCoordinat(  (CALCULATED_INFO.TerrainAlt +  CALCULATED_INFO.AltitudeAGL)*0.8,   rc);
  //    if(x0 > tsize.cx)
        if((tsize.cy) < ( CalcHeightCoordinat(  CALCULATED_INFO.TerrainAlt, rc)-y))
	      ExtTextOut(hdc, x0+IBLSCALE(1), y, ETO_OPAQUE, NULL, text, _tcslen(text), NULL);
	}
    SetBkMode(hdc, TRANSPARENT);
    SelectObject(hdc, hfOld);
  }


  //        _stprintf(text, TEXT("Mc %3.1f: "),wpt_brg);
  
  // draw aircraft
if (!asp_heading_task)
{
  int delta;
  SelectObject(hdc, GetStockObject(WHITE_PEN));
  SelectObject(hdc, GetStockObject(WHITE_BRUSH));
  line[0].x = (int)(rc.left+(rc.right-rc.left-BORDER_X)/16-1);
  line[0].y = (int)(fh*(rc.top-rc.bottom+BORDER_Y)+rc.bottom-BORDER_Y)-1;
  line[1].x = rc.left;
  line[1].y = line[0].y;
  delta = (line[0].x-line[1].x);
  line[2].x = line[1].x;
  line[2].y = line[0].y-delta/2;
  line[3].x = (line[1].x+line[0].x)/2;
  line[3].y = line[0].y;
  Polygon(hdc, line, 4);
}
else
{
#define NO_AP_PTS 17


int deg = DEG_TO_INT(AngleLimit360(wpt_brg));
double fCos = COSTABLE[deg];
double fSin = SINETABLE[deg];


int TAIL   = 6;
int PROFIL = 1;
int FINB   = 3;
int BODY   = 2;
int NOSE   = 7;
int WING   = (int) (22.0 );
int TUBE   = (int) (14.0  ) ;
int FINH   = 6+BODY;

  POINT Start;
int HEAD = TUBE / 2;
 TUBE =  3 * TUBE/ 2;
  POINT AircraftSide
  [8] = {
		  {(int)(fSin * (HEAD+0   )    ), -BODY-1},  // 1
		  {(int)(fSin * (HEAD+NOSE)    ),  0},       // 2
		  {(int)(fSin * (HEAD+0   )    ),  BODY+1},  // 3
		  {(int)(fSin * (-TUBE)        ),  BODY},    // 4   -1
		  {(int)(fSin * -TUBE          ), -FINH},    // 5
		  {(int)(fSin * (-TUBE+FINB)   ), -FINH},    // 6
		  {(int)(fSin * (-TUBE+FINB+3) ), -BODY+1},  // 7  +1
		  {(int)(fSin * (HEAD+0)       ), -BODY-1}     // 8
  };

#define  FACT 2

  BODY = (int)((double)(BODY+1) * fCos * fCos);

  int DIA = (BODY + PROFIL);

  /* both wings */
  POINT AircraftWing
  [13] = {
		  {(int)(fCos * BODY              ) ,  -DIA},    // 1
		  {(int)(fCos * (int)( FACT*BODY) ), -PROFIL},    // 2
		  {(int)(fCos * WING              ) ,  -PROFIL},    // 3
		  {(int)(fCos * WING              ), 0* PROFIL},    // 4
		  {(int)(fCos * (int)( FACT*BODY) ) , PROFIL},    // 5
		  {(int)(fCos *  BODY             ), DIA},    // 6
		  {(int)(fCos * -BODY             ) , DIA},    // 7
		  {(int)(fCos * (int)( -FACT*BODY)), PROFIL},    // 8
		  {(int)(fCos * -WING             ), 0* PROFIL  },    // 9
		  {(int)(fCos * -WING             ) , -PROFIL}  ,    // 10
		  {(int)(fCos * (int)( -FACT*BODY)), -PROFIL},    // 11
		  {(int)(fCos * -BODY             ) , -DIA},    // 12
		  {(int)(fCos *  BODY             ), -DIA}    // 13
  };


  POINT AircraftWingL
  [7] = {

		  {(int)(0 * -BODY             ),  DIA       },    // 1
		  {(int)(fCos * (int)( -FACT*BODY)),  PROFIL    },    // 2
		  {(int)(fCos * -WING             ),  0* PROFIL },    // 3
		  {(int)(fCos * -WING             ), -PROFIL    },    // 4
		  {(int)(fCos * (int)( -FACT*BODY)), -PROFIL    },    // 5
		  {(int)(0 * -BODY             ), -DIA       },    // 6
		  {(int)(0 * -BODY             ),  DIA       }     // 7
  };


  POINT AircraftWingR
  [7] = {
		  {(int)(0 * BODY              ) ,  -DIA    },   // 1
		  {(int)(fCos * (int)( FACT*BODY) ) , -PROFIL  },   // 2
		  {(int)(fCos * WING              ) ,  -PROFIL },   // 3
		  {(int)(fCos * WING              ) , 0* PROFIL},   // 4
		  {(int)(fCos * (int)( FACT*BODY) ) , PROFIL   },   // 5
		  {(int)(0 *  BODY             ) , DIA      },   // 6
		  {(int)(0 *  BODY             ) , -DIA     }    // 7
  };



  POINT AircraftTail
  [5] = {
		  {(int)(fCos *  TAIL - fSin*TUBE), -FINH},    // 1
		  {(int)(fCos *  TAIL - fSin*TUBE), -FINH +PROFIL},    // 2
		  {(int)(fCos * -TAIL - fSin*TUBE), -FINH +PROFIL},    // 3
		  {(int)(fCos * -TAIL - fSin*TUBE), -FINH },    // 4
		  {(int)(fCos *  TAIL - fSin*TUBE), -FINH},    // 5

  };



  Start.y =  (int)(fh*(rc.top-rc.bottom+BORDER_Y)+rc.bottom-BORDER_Y)-1;
  Start.x = rc.left +2* NIBLSCALE(11);

  Start.y =  (int)(fh*(rc.top-rc.bottom+BORDER_Y)+rc.bottom-BORDER_Y)-1;
  Start.x = rc.left +2* NIBLSCALE(11);


  SelectObject(hdc, GetStockObject(WHITE_PEN));
  SelectObject(hdc, GetStockObject(WHITE_BRUSH));
//  SelectObject(hdc, GetStockObject(BLACK_PEN));
  PolygonRotateShift(AircraftWing, 13,  Start.x, Start.y,  0);
  PolygonRotateShift(AircraftSide, 8,   Start.x, Start.y,  0);
  PolygonRotateShift(AircraftTail, 5,   Start.x, Start.y,  0);
  PolygonRotateShift(AircraftWingL, 7,   Start.x, Start.y,  0);
  PolygonRotateShift(AircraftWingR, 7,   Start.x, Start.y,  0);
  HBRUSH oldBrush;
  HBRUSH GreenBrush = CreateSolidBrush(COLORREF RGB_GREEN);
  HBRUSH RedBrush = CreateSolidBrush(COLORREF RGB_RED);
  if((wpt_brg < 180))
  {
    oldBrush= (HBRUSH)  SelectObject(hdc, RedBrush);
	Polygon(hdc,AircraftWingL ,7 );

	SelectObject(hdc, oldBrush);
	Polygon(hdc,AircraftSide  ,8 );

    oldBrush= (HBRUSH)  SelectObject(hdc, GreenBrush);
	Polygon(hdc,AircraftWingR ,7 );

	SelectObject(hdc, oldBrush);
  }
  else
  {

    oldBrush= (HBRUSH)  SelectObject(hdc, GreenBrush);
	Polygon(hdc,AircraftWingR ,7 );

    SelectObject(hdc, oldBrush);
  Polygon(hdc,AircraftSide  ,8 );

	oldBrush= (HBRUSH)  SelectObject(hdc, RedBrush);
	Polygon(hdc,AircraftWingL ,7 );

	SelectObject(hdc, oldBrush);



 //   Polygon(hdc,AircraftWing  ,13);


  }
  if((wpt_brg < 90)|| (wpt_brg > 270))
    Polygon(hdc,AircraftTail  ,5 );

  DeleteObject(RedBrush);
  DeleteObject(GreenBrush);

}


// Draw north arrow
  POINT Arrow[5] = { {0,-11}, {-5,9}, {0,3}, {5,9}, {0,-11}};
  POINT Start;
  Start.y = rc.top + NIBLSCALE(11);
  Start.x = rc.right - NIBLSCALE(11);

  // Direction arrow
  PolygonRotateShift(Arrow, 5, Start.x, Start.y, acb);
  SelectObject(hdc, GetStockObject(WHITE_PEN));
  SelectObject(hdc, GetStockObject(WHITE_BRUSH));
  Polygon(hdc,Arrow,5);
  SelectObject(hdc, GetStockObject(BLACK_PEN));
  Polygon(hdc,Arrow,5);




  // Print Bearing difference
  TCHAR BufferValue[LKSIZEBUFFERVALUE];
  TCHAR BufferUnit[LKSIZEBUFFERUNIT];
  TCHAR BufferTitle[LKSIZEBUFFERTITLE];

  bool ret = false;
  // Borrowed from LKDrawLook8000.cpp
  switch (OvertargetMode) {
    case OVT_TASK:
      // Do not use FormatBrgDiff for TASK, could be AAT!
      ret = MapWindow::LKFormatValue(LK_BRGDIFF, false, BufferValue, BufferUnit, BufferTitle);
      break;
    case OVT_ALT1:
      MapWindow::LKFormatBrgDiff(Alternate1, false, BufferValue, BufferUnit);
      ret = true;
      break;
    case OVT_ALT2:
      MapWindow::LKFormatBrgDiff(Alternate2, false, BufferValue, BufferUnit);
      ret = true;
      break;
    case OVT_BALT:
      MapWindow::LKFormatBrgDiff(BestAlternate, false, BufferValue, BufferUnit);
      ret = true;
      break;
    case OVT_THER:
      MapWindow::LKFormatBrgDiff(RESWP_LASTTHERMAL, true, BufferValue, BufferUnit);
      ret = true;
      break;
    case OVT_HOME:
      MapWindow::LKFormatBrgDiff(HomeWaypoint, false, BufferValue, BufferUnit);
      ret = true;
      break;
    case OVT_MATE:
      MapWindow::LKFormatBrgDiff(RESWP_TEAMMATE, true, BufferValue, BufferUnit);
      ret = true;
      break;
    case OVT_FLARM:
      MapWindow::LKFormatBrgDiff(RESWP_FLARMTARGET, true, BufferValue, BufferUnit);
      ret = true;
      break;
    default:
      ret = MapWindow::LKFormatValue(LK_BRGDIFF, false, BufferValue, BufferUnit, BufferTitle);
      break;
  }


  if (ret) {
    SIZE tsize;
    SelectObject(hdc, LK8MediumFont);
    GetTextExtentPoint(hdc, BufferValue, _tcslen(BufferValue), &tsize);
    SetBkMode(hdc, OPAQUE);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB_WHITE);
    ExtTextOut(hdc, (rc.left + rc.right - tsize.cx)/2, rc.top, ETO_OPAQUE, NULL, BufferValue, _tcslen(BufferValue), NULL);
    SetBkMode(hdc, TRANSPARENT);
  }

    
  DrawXLabel(hdc, rc, TEXT("D"));
  DrawYLabel(hdc, rc, TEXT("h"));

  SelectObject(hdc, (HPEN)oldpen);
  DeleteObject(mpen);
  DeleteObject(hpHorizonGround);
  DeleteObject(hbHorizonGround);


}



static int page=0;
static WndForm *wf=NULL;
static WndOwnerDrawFrame *wGrid=NULL;
static WndOwnerDrawFrame *wInfo=NULL;
static WndButton *wCalc=NULL;

static void SetCalcCaption(const TCHAR* caption) {
  if (wCalc) {
    wCalc->SetCaption(gettext(caption));
  }
}

static void OnAnalysisPaint(WindowControl * Sender, HDC hDC){

  RECT  rcgfx;
  HFONT hfOld;

  CopyRect(&rcgfx, Sender->GetBoundRect());

  // background is painted in the base-class

  hfOld = (HFONT)SelectObject(hDC, Sender->GetFont());

  SetBkMode(hDC, TRANSPARENT);
  SetTextColor(hDC, Sender->GetForeColor());

  switch (page) {
  case ANALYSIS_PAGE_BAROGRAPH:
    SetCalcCaption(gettext(TEXT("_@M885_"))); // Settings
    Statistics::RenderBarograph(hDC, rcgfx);
    break;
  case ANALYSIS_PAGE_CLIMB:
    SetCalcCaption(gettext(TEXT("_@M886_"))); // Task calc
    Statistics::RenderClimb(hDC, rcgfx);
    break;
  case ANALYSIS_PAGE_WIND:
    SetCalcCaption(gettext(TEXT("_@M887_"))); // Set wind
    Statistics::RenderWind(hDC, rcgfx);
    break;
  case ANALYSIS_PAGE_POLAR:
    SetCalcCaption(gettext(TEXT("_@M885_"))); // Settings
    Statistics::RenderGlidePolar(hDC, rcgfx);
    break;
  case ANALYSIS_PAGE_TEMPTRACE:
    SetCalcCaption(gettext(TEXT("_@M885_"))); // Settings
    Statistics::RenderTemperature(hDC, rcgfx);
    break;
  case ANALYSIS_PAGE_TASK:
    SetCalcCaption(gettext(TEXT("_@M886_"))); // Task calc
    LockTaskData();
    Statistics::RenderTask(hDC, rcgfx, false);
    UnlockTaskData();
    break;
  case ANALYSIS_PAGE_CONTEST:
    SetCalcCaption(gettext(TEXT("_@M1451_"))); // Change
    LockTaskData();
    Statistics::RenderTask(hDC, rcgfx, true);
    UnlockTaskData();
    break;
  case ANALYSIS_PAGE_AIRSPACE:
    SetCalcCaption(gettext(TEXT("_@M888_"))); // Warnings
    Statistics::RenderAirspace(hDC, rcgfx);
    break;
  case ANALYSIS_PAGE_TASK_SPEED:
    SetCalcCaption(gettext(TEXT("_@M886_"))); // Task calc
    LockTaskData();
    Statistics::RenderSpeed(hDC, rcgfx);
    UnlockTaskData();
    break;
  default:
    // should never get here!
    break;
  }
  SelectObject(hDC, hfOld);

}



static void Update(void){
  TCHAR sTmp[1000];
  //  WndProperty *wp;

  // Hide airspace heading switch button by default if not on ASP page
  if (page != ANALYSIS_PAGE_AIRSPACE) {
    WndButton *wb = (WndButton *)wf->FindByName(TEXT("cmdAspBear"));
    if(wb) {
      wb->SetVisible(false);
    }
  }
  
  switch(page){
    case ANALYSIS_PAGE_BAROGRAPH:
      _stprintf(sTmp, TEXT("%s: %s"),
	// LKTOKEN  _@M93_ = "Analysis" 
                gettext(TEXT("_@M93_")), 
	// LKTOKEN  _@M127_ = "Barograph" 
                gettext(TEXT("_@M127_")));
      wf->SetCaption(sTmp);
      if (flightstats.Altitude_Ceiling.sum_n<2) {
        _stprintf(sTmp, TEXT("\0"));
      } else if (flightstats.Altitude_Ceiling.sum_n<4) {
        _stprintf(sTmp, TEXT("%s:\r\n  %.0f-%.0f %s"),
	// LKTOKEN  _@M823_ = "Working band" 
                  gettext(TEXT("_@M823_")),
                  flightstats.Altitude_Base.y_ave*ALTITUDEMODIFY,
                  flightstats.Altitude_Ceiling.y_ave*ALTITUDEMODIFY,
                  Units::GetAltitudeName());
        
      } else {

        _stprintf(sTmp, TEXT("%s:\r\n  %.0f-%.0f %s\r\n\r\n%s:\r\n  %.0f %s/hr"),
	// LKTOKEN  _@M823_ = "Working band" 
                  gettext(TEXT("_@M823_")),
                  flightstats.Altitude_Base.y_ave*ALTITUDEMODIFY,
                  flightstats.Altitude_Ceiling.y_ave*ALTITUDEMODIFY,
                  Units::GetAltitudeName(),
	// LKTOKEN  _@M165_ = "Ceiling trend" 
                  gettext(TEXT("_@M165_")),
                  flightstats.Altitude_Ceiling.m*ALTITUDEMODIFY,
                  Units::GetAltitudeName());
      }
      wInfo->SetCaption(sTmp);

    break;
    case ANALYSIS_PAGE_CLIMB:
      _stprintf(sTmp, TEXT("%s: %s"), 
	// LKTOKEN  _@M93_ = "Analysis" 
                gettext(TEXT("_@M93_")),
	// LKTOKEN  _@M182_ = "Climb" 
                gettext(TEXT("_@M182_")));
      wf->SetCaption(sTmp);

      if (flightstats.ThermalAverage.sum_n==0) {
        _stprintf(sTmp, TEXT("\0"));
      } else if (flightstats.ThermalAverage.sum_n==1) {
        _stprintf(sTmp, TEXT("%s:\r\n  %3.1f %s"),
	// LKTOKEN  _@M116_ = "Av climb" 
                  gettext(TEXT("_@M116_")),
                  flightstats.ThermalAverage.y_ave*LIFTMODIFY,
                  Units::GetVerticalSpeedName()
                  );
      } else {
        _stprintf(sTmp, TEXT("%s:\r\n  %3.1f %s\r\n\r\n%s:\r\n  %3.2f %s"),
	// LKTOKEN  _@M116_ = "Av climb" 
                  gettext(TEXT("_@M116_")),
                  flightstats.ThermalAverage.y_ave*LIFTMODIFY,
                  Units::GetVerticalSpeedName(),                    
	// LKTOKEN  _@M181_ = "Climb trend" 
                  gettext(TEXT("_@M181_")),
                  flightstats.ThermalAverage.m*LIFTMODIFY,
                  Units::GetVerticalSpeedName()
                  );
      }

      wInfo->SetCaption(sTmp);

    break;
    case ANALYSIS_PAGE_WIND:
      _stprintf(sTmp, TEXT("%s: %s"), 
	// LKTOKEN  _@M93_ = "Analysis" 
                gettext(TEXT("_@M93_")),
	// LKTOKEN  _@M820_ = "Wind at Altitude" 
                gettext(TEXT("_@M820_")));
      wf->SetCaption(sTmp);
      _stprintf(sTmp, TEXT(" "));
      wInfo->SetCaption(sTmp);
    break;
    case ANALYSIS_PAGE_POLAR:
      _stprintf(sTmp, TEXT("%s: %s (%s %3.0f kg)"), 
	// LKTOKEN  _@M93_ = "Analysis" 
                gettext(TEXT("_@M93_")),
	// LKTOKEN  _@M325_ = "Glide Polar" 
                gettext(TEXT("_@M325_")),
                gettext(TEXT("_@M889_")), // Mass
                GlidePolar::GetAUW());
      wf->SetCaption(sTmp);
      if (ScreenLandscape) {
        _stprintf(sTmp, TEXT("%s:\r\n  %3.0f\r\n  at %3.0f %s\r\n\r\n%s:\r\n%3.2f %s\r\n  at %3.0f %s"),
	// LKTOKEN  _@M140_ = "Best LD" 
                  gettext(TEXT("_@M140_")),
                  GlidePolar::bestld,
                  GlidePolar::Vbestld*SPEEDMODIFY,
                  Units::GetHorizontalSpeedName(),
	// LKTOKEN  _@M437_ = "Min sink" 
                  gettext(TEXT("_@M437_")),
                  GlidePolar::minsink*LIFTMODIFY,
                  Units::GetVerticalSpeedName(),
                  GlidePolar::Vminsink*SPEEDMODIFY,
                  Units::GetHorizontalSpeedName()
                  );
      } else {
        _stprintf(sTmp, TEXT("%s:\r\n  %3.0f at %3.0f %s\r\n%s:\r\n  %3.2f %s at %3.0f %s"),
	// LKTOKEN  _@M140_ = "Best LD" 
                  gettext(TEXT("_@M140_")),
                  GlidePolar::bestld,
                  GlidePolar::Vbestld*SPEEDMODIFY,
                  Units::GetHorizontalSpeedName(),
	// LKTOKEN  _@M437_ = "Min sink" 
                  gettext(TEXT("_@M437_")),
                  GlidePolar::minsink*LIFTMODIFY,
                  Units::GetVerticalSpeedName(),
                  GlidePolar::Vminsink*SPEEDMODIFY,
                  Units::GetHorizontalSpeedName());
      }
      wInfo->SetCaption(sTmp);
    break;
  case ANALYSIS_PAGE_TEMPTRACE:
    _stprintf(sTmp, TEXT("%s: %s"), 
	// LKTOKEN  _@M93_ = "Analysis" 
              gettext(TEXT("_@M93_")),
	// LKTOKEN  _@M701_ = "Temp trace" 
              gettext(TEXT("_@M701_")));
    wf->SetCaption(sTmp);

    _stprintf(sTmp, TEXT("%s:\r\n  %5.0f %s\r\n\r\n%s:\r\n  %5.0f %s\r\n"),
	// LKTOKEN  _@M714_ = "Thermal height" 
	      gettext(TEXT("_@M714_")),
	      CuSonde::thermalHeight*ALTITUDEMODIFY,
	      Units::GetAltitudeName(),
	// LKTOKEN  _@M187_ = "Cloud base" 
	      gettext(TEXT("_@M187_")),
	      CuSonde::cloudBase*ALTITUDEMODIFY,
	      Units::GetAltitudeName());

    wInfo->SetCaption(sTmp);
    break;
  case ANALYSIS_PAGE_TASK_SPEED:
    _stprintf(sTmp, TEXT("%s: %s"), 
	// LKTOKEN  _@M93_ = "Analysis" 
              gettext(TEXT("_@M93_")),
	// LKTOKEN  _@M697_ = "Task speed" 
              gettext(TEXT("_@M697_")));
    wf->SetCaption(sTmp);
    wInfo->SetCaption(TEXT(""));
    break;
  case ANALYSIS_PAGE_TASK:
    _stprintf(sTmp, TEXT("%s: %s"), 
	// LKTOKEN  _@M93_ = "Analysis" 
              gettext(TEXT("_@M93_")),
	// LKTOKEN  _@M699_ = "Task" 
              gettext(TEXT("_@M699_")));
    wf->SetCaption(sTmp);

    RefreshTaskStatistics();

    if (!ValidTaskPoint(ActiveWayPoint)) {
	// LKTOKEN  _@M476_ = "No task" 
      _stprintf(sTmp, gettext(TEXT("_@M476_")));
    } else {
      TCHAR timetext1[100];
      TCHAR timetext2[100];
      if (AATEnabled) {
        Units::TimeToText(timetext1, (int)CALCULATED_INFO.TaskTimeToGo);
        Units::TimeToText(timetext2, (int)CALCULATED_INFO.AATTimeToGo);
       
        if (ScreenLandscape) {
          _stprintf(sTmp, 
                    TEXT("%s:\r\n  %s\r\n%s:\r\n  %s\r\n%s:\r\n  %5.0f %s\r\n%s%.0f %s\r\n"), // 100429
	// LKTOKEN  _@M698_ = "Task to go" 
                    gettext(TEXT("_@M698_")),
                    timetext1,
	// LKTOKEN  _@M42_ = "AAT to go" 
                    gettext(TEXT("_@M42_")),
                    timetext2,
	// LKTOKEN  _@M242_ = "Dist to go" 
                    gettext(TEXT("_@M242_")),
                    DISTANCEMODIFY*CALCULATED_INFO.AATTargetDistance,
                    Units::GetDistanceName(),
	// LKTOKEN  _@M626_ = "Sp " 
                    gettext(TEXT("_@M626_")),
                    TASKSPEEDMODIFY*CALCULATED_INFO.AATTargetSpeed,
                    Units::GetTaskSpeedName()		
                    );
        } else {
          _stprintf(sTmp, 
                    TEXT("%s: %s\r\n%s: %s\r\n%s: %5.0f %s\r\n%s: %5.0f %s\r\n"),
	// LKTOKEN  _@M698_ = "Task to go" 
                    gettext(TEXT("_@M698_")),
                    timetext1,
	// LKTOKEN  _@M42_ = "AAT to go" 
                    gettext(TEXT("_@M42_")),
                    timetext2,
	// LKTOKEN  _@M242_ = "Dist to go" 
                    gettext(TEXT("_@M242_")),
                    DISTANCEMODIFY*CALCULATED_INFO.AATTargetDistance,
                    Units::GetDistanceName(),
	// LKTOKEN  _@M681_ = "Targ.speed" 
                    gettext(TEXT("_@M681_")),
                    TASKSPEEDMODIFY*CALCULATED_INFO.AATTargetSpeed,
                    Units::GetTaskSpeedName()		
                    );
        }
      } else {
        Units::TimeToText(timetext1, (int)CALCULATED_INFO.TaskTimeToGo);
        _stprintf(sTmp, TEXT("%s: %s\r\n%s: %5.0f %s\r\n"),
	// LKTOKEN  _@M698_ = "Task to go" 
                  gettext(TEXT("_@M698_")),
                  timetext1,
	// LKTOKEN  _@M242_ = "Dist to go" 
                  gettext(TEXT("_@M242_")),
                  DISTANCEMODIFY*CALCULATED_INFO.TaskDistanceToGo,
                  Units::GetDistanceName());
      }
    } 
    wInfo->SetCaption(sTmp);
    break;
  case ANALYSIS_PAGE_CONTEST:
    _stprintf(sTmp, TEXT("%s: %s - %s"), 
              // LKTOKEN  _@M93_ = "Analysis" 
              gettext(TEXT("_@M93_")),
              // LKTOKEN  _@M1450_ = "Contest" 
              gettext(TEXT("_@M1450_")),
              CContestMgr::TypeToString(contestType));
    wf->SetCaption(sTmp);
    
    {
      CContestMgr::CResult result = CContestMgr::Instance().Result(contestType, false);
      if(result.Type() == contestType) {
        TCHAR distStr[50];
        _stprintf(distStr, _T("%.1f %s\r\n"),
                  DISTANCEMODIFY * result.Distance(),
                  Units::GetDistanceName());
        
        TCHAR speedStr[50];
        _stprintf(speedStr, TEXT("%.1f %s\r\n"),
                  TASKSPEEDMODIFY * result.Speed(),
                  Units::GetTaskSpeedName());
        
        TCHAR timeTempStr[50];
        Units::TimeToText(timeTempStr, result.Duration());
        TCHAR timeStr[50];
        _stprintf(timeStr, _T("%s\r\n"), timeTempStr);
        
        TCHAR scoreStr[50] = _T("");
        if(result.Type() != CContestMgr::TYPE_FAI_3_TPS &&
           result.Type() != CContestMgr::TYPE_FAI_3_TPS_PREDICTED)
          _stprintf(scoreStr, TEXT("%.2f pt\r\n"), result.Score());
        
        TCHAR plusStr[50] = _T("");
        if(result.Type() == CContestMgr::TYPE_OLC_CLASSIC ||
           result.Type() == CContestMgr::TYPE_OLC_CLASSIC_PREDICTED ||
           result.Type() == CContestMgr::TYPE_OLC_FAI ||
           result.Type() == CContestMgr::TYPE_OLC_FAI_PREDICTED) {
          CContestMgr::TType type = (result.Type() == CContestMgr::TYPE_OLC_CLASSIC_PREDICTED ||
                                     result.Type() == CContestMgr::TYPE_OLC_FAI_PREDICTED) ?
            CContestMgr::TYPE_OLC_PLUS_PREDICTED : CContestMgr::TYPE_OLC_PLUS;
          CContestMgr::CResult resultPlus = CContestMgr::Instance().Result(type, false);
          if(ScreenLandscape)
            _stprintf(plusStr, TEXT("\r\n%s:\r\n%.2f pt"),
                      CContestMgr::TypeToString(type),
                      resultPlus.Score());
          else
            _stprintf(plusStr, TEXT("\r\n%s: %.2f pt"),
                      CContestMgr::TypeToString(type),
                      resultPlus.Score());
        }
        
        _stprintf(sTmp, _T("%s%s%s%s%s"), distStr, speedStr, timeStr, scoreStr, plusStr);
      }
      else {
        _stprintf(sTmp, TEXT("%s\r\n"),
                  // LKTOKEN  _@M477_ = "No valid path" 
                  gettext(TEXT("_@M477_")));
      }
      wInfo->SetCaption(sTmp);
    }

    break;
  case ANALYSIS_PAGE_AIRSPACE:
    _stprintf(sTmp, TEXT("%s: %s"), 
	// LKTOKEN  _@M93_ = "Analysis" 
              gettext(TEXT("_@M93_")),
	// LKTOKEN  _@M68_ = "Airspace" 
              gettext(TEXT("_@M68_")));
    wf->SetCaption(sTmp);
    WndButton *wb = (WndButton *)wf->FindByName(TEXT("cmdAspBear"));
    if(wb) {
      wb->SetVisible(true);
      if (asp_heading_task) {
        wb->SetCaption(gettext(TEXT("_@M1287_")));                               //_@M1287_ "Heading"
        int overindex = GetOvertargetIndex();
        if (overindex>=0) {
          TCHAR ovtname[LKSIZEBUFFERLARGE];
          GetOvertargetName(ovtname);
          _stprintf(sTmp, TEXT("%s: %s"), gettext(TEXT("_@M1288_")), ovtname);                //_@M1288_ "Showing towards next waypoint"
          wInfo->SetCaption(sTmp);
        } else {
          _stprintf(sTmp, TEXT("%s: %s"), gettext(TEXT("_@M1288_")), gettext(TEXT("_@M479_")));                    //_@M1288_ "Showing towards next waypoint"  _@M479_ "None"
          wInfo->SetCaption(sTmp);
        }
      } else {
        wb->SetCaption(gettext(TEXT("_@M1289_")));                               //_@M1289_ "Next WP"
        wInfo->SetCaption(gettext(TEXT("_@M1290_")));                            //_@M1290_ "Showing towards heading"
      }
    }
    break;
  }

  wGrid->SetVisible(page<MAXPAGE+1);

  if (wGrid != NULL)
    wGrid->Redraw();

}

static void NextPage(int Step){
  page += Step;
  if (page > MAXPAGE)
    page = 0;
  if (page < 0)
    page = MAXPAGE;
  Update();
}


static void OnNextClicked(WindowControl * Sender){
	(void)Sender;
  NextPage(+1);
}

static void OnPrevClicked(WindowControl * Sender){
	(void)Sender;
  NextPage(-1);
}

static void OnCloseClicked(WindowControl * Sender){
	(void)Sender;

  wf->SetModalResult(mrOK);
}


static int FormKeyDown(WindowControl * Sender, WPARAM wParam, LPARAM lParam){
  (void)Sender; (void)lParam;

  if (wGrid->GetFocused())
    return(0);
  
  switch(wParam & 0xffff){
    case VK_LEFT:
    case '6':
      SetFocus(((WndButton *)wf->FindByName(TEXT("cmdPrev")))->GetHandle());
      NextPage(-1);
      //((WndButton *)wf->FindByName(TEXT("cmdPrev")))->SetFocused(true, NULL);
    return(0);
    case VK_RIGHT:
    case '7':
      SetFocus(((WndButton *)wf->FindByName(TEXT("cmdNext")))->GetHandle());
      NextPage(+1);
      //((WndButton *)wf->FindByName(TEXT("cmdNext")))->SetFocused(true, NULL);
    return(0);
  }
  return(1);
}

static void OnCalcClicked(WindowControl * Sender, 
			  WndListFrame::ListInfo_t *ListInfo){
  (void)ListInfo;
  (void)Sender;
  if (page==ANALYSIS_PAGE_BAROGRAPH) {
    dlgBasicSettingsShowModal();
  }
  if (page==ANALYSIS_PAGE_CLIMB) {
    wf->SetVisible(false);
    dlgTaskCalculatorShowModal();
    wf->SetVisible(true);
  }
  if (page==ANALYSIS_PAGE_WIND) {
    dlgWindSettingsShowModal();
  }
  if (page==ANALYSIS_PAGE_POLAR) {
    dlgBasicSettingsShowModal();
  }
  if (page==ANALYSIS_PAGE_TEMPTRACE) {
    dlgBasicSettingsShowModal();
  }
  if ((page==ANALYSIS_PAGE_TASK) || (page==ANALYSIS_PAGE_TASK_SPEED)) {
    wf->SetVisible(false);
    dlgTaskCalculatorShowModal();
    wf->SetVisible(true);
  }
  if (page==ANALYSIS_PAGE_CONTEST) {
    // Rotate presented contest
    switch(contestType) {
    case CContestMgr::TYPE_OLC_CLASSIC:
      contestType = CContestMgr::TYPE_OLC_FAI;
      break;
    case CContestMgr::TYPE_OLC_FAI:
      contestType = CContestMgr::TYPE_OLC_CLASSIC_PREDICTED;
      break;
    case CContestMgr::TYPE_OLC_CLASSIC_PREDICTED:
      contestType = CContestMgr::TYPE_OLC_FAI_PREDICTED;
      break;
    case CContestMgr::TYPE_OLC_FAI_PREDICTED:
      contestType = CContestMgr::TYPE_OLC_LEAGUE;
      break;
    case CContestMgr::TYPE_OLC_LEAGUE:
      contestType = CContestMgr::TYPE_FAI_3_TPS;
      break;
    case CContestMgr::TYPE_FAI_3_TPS:
      contestType = CContestMgr::TYPE_FAI_3_TPS_PREDICTED;
      break;
    case CContestMgr::TYPE_FAI_3_TPS_PREDICTED:
      contestType = CContestMgr::TYPE_OLC_CLASSIC;
      break;
    default:
      contestType = CContestMgr::TYPE_OLC_CLASSIC;
    }
  }
  if (page==ANALYSIS_PAGE_AIRSPACE) {
    dlgAirspaceWarningParamsShowModal();
  }
  Update();
}

static void OnAspBearClicked(WindowControl * Sender){
  (void)Sender;
    asp_heading_task = !asp_heading_task;
    Update();
}


static CallBackTableEntry_t CallBackTable[]={
  DeclareCallBackEntry(OnAnalysisPaint),
  DeclareCallBackEntry(OnNextClicked),
  DeclareCallBackEntry(OnPrevClicked),
  DeclareCallBackEntry(OnCalcClicked),
  DeclareCallBackEntry(OnAspBearClicked),
  DeclareCallBackEntry(NULL)
};


static int OnTimerNotify(WindowControl *Sender)
{
  static short i=0;
  if(i++ % 2 == 0)
    return 0;
  
  // run once per second
  Update();
  return 0;
}


static int TouchKeyDown(WindowControl * Sender, WPARAM wParam, LPARAM lParam){
        (void)lParam;
        (void)wParam;

 int X = LOWORD(lParam);
 int Y = HIWORD(lParam);
 int k;


#define ULLI_ASPSELECTION
#ifdef ULLI_ASPSELECTION


if(page ==ANALYSIS_PAGE_AIRSPACE)
  if (TouchContext ==  TCX_PROC_UP)
 for (k=0 ; k < iNohandeldSpaces; k++)
 {

   if( pHandeled[k].psAS != NULL)
   {
     RECT rcd =pHandeled[k].rc;

     if (PtInRect(X,Y,rcd ))
     {
		  #ifndef DISABLEAUDIO
		    if (EnableSoundModes)PlayResource(TEXT("IDR_WAV_BTONE4"));
		  #endif
	      CAirspace	 airspace_copy = CAirspaceManager::Instance().GetAirspaceCopy(pHandeled[k].psAS);
		  dlgAirspaceDetails(&airspace_copy);
       return 0;
     }
   }
	}
#endif // ULLI

//#define PAOLO_FAST_SWITCH
#ifdef PAOLO_FAST_SWITCH
  if (X<180) return 1;
  if (TouchContext< TCX_PROC_UP) {
    #ifndef DISABLEAUDIO
    if (EnableSoundModes) PlayResource(TEXT("IDR_WAV_CLICK"));
    #endif
    NextPage(+1);
    return 0;
 }
#endif
 return 1;
}



void dlgAnalysisShowModal(int inpage){
static bool entered = false;
	 if (entered == true) /* prevent re entrance */
		 return;

  wf=NULL;
  wGrid=NULL;
  wInfo=NULL;
  wCalc=NULL;
 entered = true;
  if (!ScreenLandscape) {
    char filename[MAX_PATH];
    LocalPathS(filename, TEXT("dlgAnalysis_L.xml"));
    wf = dlgLoadFromXML(CallBackTable, 
                        
                        filename, 
                        hWndMainWindow,
                        TEXT("IDR_XML_ANALYSIS_L"));
  } else  {
    char filename[MAX_PATH];
    LocalPathS(filename, TEXT("dlgAnalysis.xml"));
    wf = dlgLoadFromXML(CallBackTable, 
                        filename, 
                        hWndMainWindow,
                        TEXT("IDR_XML_ANALYSIS"));
  }

  if (!wf) return;

  penThinSignal = CreatePen(PS_SOLID, 2 , RGB(50,243,45));

  wf->SetLButtonUpNotify(TouchKeyDown);
  wf->SetKeyDownNotify(FormKeyDown);

  wGrid = (WndOwnerDrawFrame*)wf->FindByName(TEXT("frmGrid"));
  wInfo = (WndOwnerDrawFrame*)wf->FindByName(TEXT("frmInfo"));
  
  wCalc = ((WndButton *)wf->FindByName(TEXT("cmdCalc")));

  WndButton *wClose = (WndButton *)wf->FindByName(TEXT("cmdClose"));
  if(wClose) {
    wClose->SetOnClickNotify(OnCloseClicked);
  }

  /*
    Does Not Work, Because wf->GetHeigth() Is WindowHeight and ClientRect Is Calculated By OnPaint
    Why WndForm Do Not Use NonClientArea for Border and Caption ?
  */
  /*
  WndFrame *wBtFrm = (WndFrame*)wf->FindByName(TEXT("frmButton"));
  if(wBtFrm) {
    wBtFrm->SetTop(wf->GetHeigth()- wBtFrm->GetHeight());
    if(wInfo) {
      wInfo->SetHeight(wBtFrm->GetTop()-wInfo->GetTop());
    }
  }
  */

  if(wGrid) {
    wGrid->SetWidth( wf->GetWidth() - wGrid->GetLeft()-6);
  }

  wf->SetTimerNotify(OnTimerNotify);

  Update();

  if (inpage!=ANALYSYS_PAGE_DEFAULT) page=inpage;

  wf->ShowModal();

  delete wf;

  wf = NULL;

  DeleteObject(penThinSignal);

  MapWindow::RequestFastRefresh();
  FullScreen();
  entered = false;
}


