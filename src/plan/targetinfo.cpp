#include "imgdisplay.h"
#include "../utilsdb/rts2appdb.h"
#include "../utilsdb/rts2camlist.h"
#include "../utilsdb/target.h"
#include "../utilsdb/rts2obsset.h"
#include "../utils/rts2config.h"
#include "../utils/libnova_cpp.h"
#include "rts2script.h"

#include <iostream>
#include <iomanip>
#include <list>
#include <stdlib.h>

std::ostream & operator << (std::ostream & _os, struct ln_lnlat_posn *_pos)
{
  struct ln_dms dms;
  ln_deg_to_dms (_pos->lng, &dms);
  _os << std::setw (3) << std::setfill ('0') << dms.degrees << ":"
    << std::setw (2) << std::setfill ('0') << dms.minutes << ":"
    << std::setprecision (4) << dms.seconds;
  return _os;
}

double
get_norm_hour (double JD)
{
  struct ln_date tmp_date;
  double ret;

  ln_get_date (JD, &tmp_date);
  ret =
    tmp_date.hours + (double) tmp_date.minutes / 24.0 +
    (double) tmp_date.seconds / 3600.0;
  if (ret > 12.0)
    ret -= 24.0;
  return ret;
}

class Rts2TargetInfo:public Rts2AppDb
{
private:
  std::list < int >targets;
  Rts2CamList cameras;
  Target *target;
  struct ln_lnlat_posn *obs;
  void printTargetInfo ();
  void printTargetInfoGNU (double jd_start, double pbeg, double pend,
			   double step);
  int printExtendet;
  int printCalTargets;
  int printObservations;
  int printImages;
  int printCounts;
  bool printGNU;
  bool addMoon;
  char targetType;
  virtual int printTargets (Rts2TargetSet & set);

  double JD;
public:
    Rts2TargetInfo (int argc, char **argv);
    virtual ~ Rts2TargetInfo (void);

  virtual int processOption (int in_opt);

  virtual int processArgs (const char *arg);
  virtual int init ();
  virtual int run ();
};

Rts2TargetInfo::Rts2TargetInfo (int in_argc, char **in_argv):
Rts2AppDb (in_argc, in_argv)
{
  obs = NULL;
  printExtendet = 0;
  printCalTargets = 0;
  printObservations = 0;
  printImages = 0;
  printCounts = 0;
  printGNU = false;
  addMoon = true;
  targetType = '\0';

  JD = ln_get_julian_from_sys ();

  addOption ('e', "extended", 2,
	     "print extended informations (visibility prediction,..)");
  addOption ('g', "gnuplot", 0, "print in GNU plot format");
  addOption ('m', "moon", 0, "do not plot moon");
  addOption ('c', "calibartion", 0, "print recommended calibration targets");
  addOption ('o', "observations", 2,
	     "print observations (in given time range)");
  addOption ('i', "images", 2, "print images (in given time range)");
  addOption ('I', "images_summary", 0, "print image summary row");
  addOption ('p', "photometer", 2, "print counts (in given time range)");
  addOption ('P', "photometer_summary", 0, "print counts summary row");
  addOption ('t', "target_type", 1,
	     "search for target types, not for targets IDs");
  addOption ('d', "date", 1, "give informations for this data");
}

Rts2TargetInfo::~Rts2TargetInfo ()
{
  cameras.clear ();
}

int
Rts2TargetInfo::processOption (int in_opt)
{
  int ret;
  switch (in_opt)
    {
    case 'e':
      printExtendet = 1;
      break;
    case 'g':
      printGNU = true;
      break;
    case 'm':
      addMoon = false;
      break;
    case 'c':
      printCalTargets = 1;
      break;
    case 'o':
      printObservations = 1;
      break;
    case 'i':
      printImages |= DISPLAY_ALL;
      break;
    case 'I':
      printImages |= DISPLAY_SUMMARY;
      break;
    case 'p':
      printCounts |= DISPLAY_ALL;
      break;
    case 'P':
      printCounts |= DISPLAY_SUMMARY;
      break;
    case 't':
      targetType = *optarg;
      break;
    case 'd':
      ret = parseDate (optarg, JD);
      if (ret)
	return ret;
      break;
    default:
      return Rts2AppDb::processOption (in_opt);
    }
  return 0;
}

int
Rts2TargetInfo::processArgs (const char *arg)
{
  // try to create that target..
  int tar_id;
  char *test;
  tar_id = strtol (arg, &test, 10);
  if (*test)
    {
      std::cerr << "Invalid target id: " << arg << std::endl;
      return -1;
    }
  targets.push_back (tar_id);
  return 0;
}

void
Rts2TargetInfo::printTargetInfo ()
{
  target->sendInfo (std::cout, JD);
  // print scripts..
  Rts2CamList::iterator cam_names;
  if (printExtendet)
    {
      for (int i = 0; i < 10; i++)
	{
	  JD += 10;

	  std::cout << "==================================" << std::endl <<
	    "Date: " << LibnovaDate (JD) << std::endl;
	  target->sendPositionInfo (std::cout, JD);
	}
    }
  for (cam_names = cameras.begin (); cam_names != cameras.end (); cam_names++)
    {
      const char *cam_name = (*cam_names).c_str ();
      int ret;
      char script_buf[MAX_COMMAND_LENGTH];
      int failedCount;
      ret = target->getScript (cam_name, script_buf);
      std::
	cout << "Script for camera " << cam_name << ":'" << script_buf <<
	"' ret (" << ret << ")" << std::endl;
      // try to parse it..
      Rts2Script script = Rts2Script (NULL, cam_name, target);
      failedCount = script.getFaultLocation ();
      if (failedCount != -1)
	{
	  std::
	    cout << "PARSING of script '" << script_buf << "' FAILED!!! AT "
	    << failedCount << std::endl;
	}
    }
  // print recomended calibrations targets
  if (printCalTargets)
    {
      Rts2TargetSet *cal;
      cal = target->getCalTargets (JD);
      std::cout << "==================================" << std::endl <<
	"Calibration targets" << std::endl;
      cal->print (std::cout, JD);
      delete cal;
    }
  // print observations..
  if (printObservations)
    {
      Rts2ObsSet obsSet = Rts2ObsSet (target->getTargetID ());
      if (printImages)
	obsSet.printImages (printImages);
      if (printCounts)
	obsSet.printCounts (printCounts);
      std::cout << obsSet << std::endl;
    }
  else if (printImages || printCounts)
    {
      if (printImages)
	{
	  Rts2ImgSetTarget imgset = Rts2ImgSetTarget (target->getTargetID ());
	  imgset.load ();
	  imgset.print (std::cout, printImages);
	  imgset.clear ();
	}
    }
  return;
}

void
Rts2TargetInfo::printTargetInfoGNU (double jd_start, double pbeg, double pend,
				    double step)
{
  for (double i = pbeg; i <= pend; i += step)
    {
      double h = i;
      // normalizae h to <-12;12>
      if (h > 24.0)
	h -= 24.0;
      if (h > 12.0)
	h -= 24.0;
      std::cout << std::setw (10) << h << " ";
      target->printAltTableSingleCol (std::cout, jd_start, i, step);
      std::cout << std::endl;
    }
}

int
Rts2TargetInfo::printTargets (Rts2TargetSet & set)
{
  Rts2TargetSet::iterator iter;
  struct ln_rst_time t_rst;
  struct ln_rst_time n_rst;

  double sset;
  double rise;
  double nbeg;
  double nend;

  char old_fill;
  int old_p;
  std::ios_base::fmtflags old_settings;

  if (printGNU)
    {
      ln_get_body_next_rst_horizon (JD, obs, ln_get_solar_equ_coords,
				    LN_SOLAR_CIVIL_HORIZON, &t_rst);
      ln_get_body_next_rst_horizon (JD, obs, ln_get_solar_equ_coords,
				    LN_SOLAR_NAUTIC_HORIZON, &n_rst);

      if (t_rst.rise < t_rst.set)
	t_rst.rise += 1.0;
      if (n_rst.rise < n_rst.set)
	n_rst.rise += 1.0;

      sset = get_norm_hour (t_rst.set);
      rise = get_norm_hour (t_rst.rise);
      nbeg = get_norm_hour (n_rst.set);
      nend = get_norm_hour (n_rst.rise);

      old_fill = std::cout.fill ('0');
      old_p = std::cout.precision (7);
      old_settings = std::cout.flags ();
      std::cout.setf (std::ios_base::fixed, std::ios_base::floatfield);

      std::cout
	<< "sset=" << sset << std::endl
	<< "rise=" << rise << std::endl
	<< "nend=" << nend << std::endl
	<< "nbeg=" << nbeg << std::endl
	<< "set yrange [0:90] noreverse" << std::endl
	<< "set xrange [sset:rise] noreverse" << std::endl
	<< "set xlabel \"Time UT [h]\"" << std::endl
	<<
	"set xtics ( \"12\" -12, \"13\" -11, \"14\" -10, \"15\" -9, \"16\" -8, \"17\" -7, \"18\" -6, \"19\" -5, \"20\" -4, \"21\" -3, \"22\" -2, \"23\" -1, \"0\" 0, \"1\" 1, \"2\" 2, \"3\" 3, \"4\" 4, \"5\" 5, \"6\" 6, \"7\" 7, \"8\" 8, \"9\" 9, \"10\" 10, \"11\" 11, \"12\" 12)"
	<< std::
	endl << "set arrow from nbeg,0 to nbeg,90 nohead lt 0" << std::
	endl << "set arrow from nend,0 to nend,90 nohead lt 0" << std::
	endl << "set ylabel \"altitude\"" << std::
	endl << "set y2label \"airmass\"" << std::
	endl <<
	"set y2tics ( \"1.00\" 90, \"1.05\" 72.25, \"1.10\" 65.38, \"1.20\" 56.44, \"1.30\" 50.28 , \"1.50\" 41.81, \"2.00\" 30, \"3.00\" 20, \"6.00\" 10)"
	<< std::
	endl << "set arrow from sset,10 to rise,10 nohead lt 0" << std::
	endl << "set arrow from sset,20 to rise,20 nohead lt 0" << std::
	endl << "set arrow from sset,30 to rise,30 nohead lt 0" << std::
	endl << "set arrow from sset,41.81 to rise,41.81 nohead lt 0" << std::
	endl << "set arrow from sset,50.28 to rise,50.28 nohead lt 0" << std::
	endl << "set arrow from sset,56.44 to rise,56.44 nohead lt 0" << std::
	endl << "set arrow from sset,65.38 to rise,65.38 nohead lt 0" << std::
	endl << "set arrow from sset,72.25 to rise,72.25 nohead lt 0" << std::
	endl << "set arrow from sset,81.93 to rise,81.93 nohead lt 0" << std::
	endl << "set arrow from nbeg,0 to nbeg,90 nohead lt 0" << std::
	endl << "set arrow from nend,0 to nend,90 nohead lt 0" << std::
	endl <<
	"set arrow from (nend/2+nbeg/2),0 to (nend/2+nbeg/2),90 nohead lt 0"
	<< std::endl << "plot \\" << std::endl;

      if (addMoon)
	{
	  std::cout << "     \"-\" u 1:2 smooth csplines t \"Moon\"";
	}

      for (iter = set.begin (); iter != set.end (); iter++)
	{
	  target = *iter;
	  if (iter != set.begin () || addMoon)
	    std::cout << ", \\" << std::endl;
	  std::cout
	    << "     \"-\" u 1:2 smooth csplines t \""
	    << target->getTargetName () << "\"";
	}
      std::cout << std::endl;
    }

  sset -= 1.0;
  rise += 1.0;

  double jd_start = ((int) JD) - 0.5;
  double step = 0.2;

  if (printGNU && addMoon)
    {
      struct ln_hrz_posn moonHrz;
      struct ln_equ_posn moonEqu;
      for (double i = sset; i <= rise; i += step)
	{
	  ln_get_lunar_equ_coords (jd_start + i, &moonEqu);
	  ln_get_hrz_from_equ (&moonEqu, obs, JD, &moonHrz);
	  std::cout
	    << i << " " << moonHrz.alt << " " << moonHrz.az << std::endl;
	}
      std::cout << "e" << std::endl;
    }

  for (iter = set.begin (); iter != set.end (); iter++)
    {
      target = *iter;
      if (printGNU)
	{
	  printTargetInfoGNU (jd_start, sset, rise, step);
	  std::cout << "e" << std::endl;
	}
      else
	{
	  printTargetInfo ();
	}
    }

  if (printGNU)
    {
      std::cout.setf (old_settings);
      std::cout.precision (old_p);
      std::cout.fill (old_fill);
    }

  return (set.size () == 0 ? -1 : 0);
}

int
Rts2TargetInfo::init ()
{
  int ret;

  ret = Rts2AppDb::init ();
  if (ret)
    return ret;

  Rts2Config *config;
  config = Rts2Config::instance ();

  if (!obs)
    {
      obs = config->getObserver ();
    }

  cameras = Rts2CamList ();

  return 0;
}

int
Rts2TargetInfo::run ()
{
  if (targetType != '\0')
    {
      Rts2TargetSetType typeSet = Rts2TargetSetType (targetType);
      return printTargets (typeSet);
    }

  Rts2TargetSet tar_set = Rts2TargetSet (targets);
  return printTargets (tar_set);
}

int
main (int argc, char **argv)
{
  int ret;
  Rts2TargetInfo *app;
  app = new Rts2TargetInfo (argc, argv);
  ret = app->init ();
  if (ret)
    return 1;
  app->run ();
  delete app;
}
