#include "model/telmodel.h"
#include "../utils/libnova_cpp.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

class Rts2DevTelescopeModelTest:public Rts2DevTelescope
{
public:
  Rts2DevTelescopeModelTest ():Rts2DevTelescope (0, NULL)
  {
    createConstValue (telLongtitude, "LONG", "telescope longtitude");
    createConstValue (telLatitude, "LAT", "telescope latitude");
    createConstValue (telAltitude, "ALTI", "telescope altitude");

    telLongtitude->setValueDouble (Rts2Config::instance ()->getObserver ()->
				   lng);
    telLatitude->setValueDouble (Rts2Config::instance ()->getObserver ()->
				 lat);
  }

  void setObserverLat (double in_lat)
  {
    telLatitude->setValueDouble (in_lat);
  }
};

class TelModelTest:public Rts2App
{
private:
  char *modelFile;
  Rts2TelModel *model;
    std::vector < std::string > runFiles;
  Rts2DevTelescopeModelTest *telescope;
  int errors;
  bool verbose;

  void test (double ra, double dec);
  void runOnFile (std::string filename, std::ostream & os);
protected:
    virtual int processOption (int in_opt);
  virtual int processArgs (const char *arg);

public:
    TelModelTest (int in_argc, char **in_argv);
    virtual ~ TelModelTest (void);

  virtual int init ();
  virtual int run ();
};

TelModelTest::TelModelTest (int in_argc, char **in_argv):
Rts2App (in_argc, in_argv)
{
  Rts2Config *config;
  config = Rts2Config::instance ();
  modelFile = NULL;
  model = NULL;
  telescope = NULL;
  errors = 0;
  verbose = false;
  addOption ('m', NULL, 1, "Model file to use");
  addOption ('e', NULL, 0, "Print errors");
  addOption ('v', NULL, 0, "Report model progress");
  addOption ('i', NULL, 1, "Print model for given RA and DEC");
}

TelModelTest::~TelModelTest (void)
{
  delete telescope;
  runFiles.clear ();
}

int
TelModelTest::processOption (int in_opt)
{
  switch (in_opt)
    {
    case 'm':
      modelFile = optarg;
      break;
    case 'e':
      errors++;
      break;
    case 'v':
      verbose = true;
      break;
    case 'i':
      break;
    default:
      return Rts2App::processOption (in_opt);
    }
  return 0;
}

int
TelModelTest::processArgs (const char *in_arg)
{
  runFiles.push_back (in_arg);
  return 0;
}

int
TelModelTest::init ()
{
  int ret;
  ret = Rts2App::init ();
  if (ret)
    return ret;

  if (runFiles.empty ())
    {
      help ();
      return -1;
    }

  telescope = new Rts2DevTelescopeModelTest ();

  model = new Rts2TelModel (telescope, modelFile);
  ret = model->load ();

  return ret;
}

void
TelModelTest::test (double ra, double dec)
{
  struct ln_equ_posn pos;
  pos.ra = ra;
  pos.dec = dec;
  model->apply (&pos);
  std::cout << "RA: " << ra << " DEC: " << dec << " -> " << pos.
    ra << " " << pos.dec << std::endl;
}

void
TelModelTest::runOnFile (std::string filename, std::ostream & os)
{
  char caption[81];
  double temp;
  double press;
  std::ifstream is (filename.c_str ());
  char firstChar;
  bool latLine = false;

  is.getline (caption, 80);
  os << caption << std::endl;
  while (!is.eof ())
    {
      // ignore
      firstChar = is.peek ();
      if (firstChar == ':')
	{
	  is.getline (caption, 80);
	  os << caption << std::endl;
	}
      else if (firstChar == 'E')
	{
	  std::string nd;
	  is >> nd;
	  if (nd == "END")
	    {
	      os << "END" << std::endl;
	      return;
	    }
	  logStream (MESSAGE_ERROR) << "Invalid end? " << nd << sendLog;
	}
      // first line contains lat
      else if (!latLine)
	{
	  struct ln_dms lat;
	  struct ln_date date;
	  std::string line;
	  std::getline (is, line);
	  std::istringstream iss (line);
	  date.hours = 0;
	  date.minutes = 0;
	  date.seconds = 0;
	  // get sign
	  if (isblank (firstChar))
	    iss >> firstChar;

	  iss >> lat.degrees >> lat.minutes >> lat.seconds;

	  iss >> date.years >> date.months >> date.days;
	  if (iss.fail ())
	    {
	      date.years = 2000;
	      date.months = 1;
	      date.days = 1;
	      temp = nan ("f");
	      press = nan ("f");
	    }
	  else
	    {
	      iss >> temp >> press;
	      if (iss.fail ())
		{
		  temp = nan ("f");
		  press = nan ("f");
		}
	    }
	  if (firstChar == '-')
	    lat.neg = 1;
	  else
	    lat.neg = 0;
	  telescope->setObserverLat (ln_dms_to_deg (&lat));
	  latLine = true;
	  os << " " << LibnovaDeg90 (ln_dms_to_deg (&lat)) << " "
	    << date.years << " " << date.months << " " << date.days;
	  if (!isnan (temp))
	    {
	      os << " " << temp;
	      if (!isnan (press))
		os << " " << press;
	    }
	  os << std::endl;
	}
      // data lines
      else
	{
	  double aux1;
	  double aux2;
	  int m, d;
	  double epoch;

	  if (firstChar == '!')
	    firstChar = is.get ();

	  LibnovaRaDec _in;
	  LibnovaRaDec _out;
	  LibnovaHaM lst;

	  is >> _in >> m >> d >> epoch >> _out >> lst >> aux1 >> aux2;
	  is.ignore (2000, is.widen ('\n'));
	  if (is.fail ())
	    {
	      logStream (MESSAGE_ERROR) << "Failed during file read" <<
		sendLog;
	      return;
	    }
	  // calculate model position, output it,..
	  struct ln_equ_posn pos;
	  _out.getPos (&pos);
	  pos.ra = ln_range_degrees (lst.getRa () - pos.ra);
	  if (verbose)
	    model->applyVerbose (&pos);
	  else
	    model->apply (&pos);
	  // print it out
	  pos.ra = ln_range_degrees (lst.getRa () - pos.ra);
	  std::cout.precision (1);
	  LibnovaRaDec _out_in (&pos);

	  if (errors)
	    {
	      struct ln_equ_posn pos_in, pos_out;
	      _in.getPos (&pos_in);
	      _out_in.getPos (&pos_out);
	      std::
		cout <<
		LibnovaDegDist (ln_get_angular_separation
				(&pos_in, &pos_out));
	      if (errors > 1)
		{
		  std::cout << LibnovaDegArcMin (pos_in.ra -
						 pos_out.
						 ra) << " " <<
		    LibnovaDegArcMin (pos_in.dec - pos_out.dec);
		}
	    }

	  std::cout << "  " << _out_in << " "
	    << m << " "
	    << d << " " << epoch << "   " << _out << "   " << lst << " ";
	  std::cout.precision (6);
	  std::cout << aux1 << " " << aux2 << std::endl;
	}
    }
}

int
TelModelTest::run ()
{
  if (!runFiles.empty ())
    {
      for (std::vector < std::string >::iterator iter = runFiles.begin ();
	   iter != runFiles.end (); iter++)
	runOnFile ((*iter), std::cout);
    }
  else
    {
      // some generic tests
      test (10, 20);
      test (30, 50);
    }
  return 0;
}

int
main (int argc, char **argv)
{
  int ret;
  TelModelTest app (argc, argv);
  ret = app.init ();
  if (ret)
    return ret;
  return app.run ();
}
