#include "../../lib/rts2fits/rts2appdbimage.h"
#include "../../lib/rts2fits/imagedb.h"
#include "rts2config.h"

#include <iostream>

#include <list>

class Rts2MoveArchive:public Rts2AppDbImage
{
	protected:
		virtual int processImage (rts2image::ImageDb * image)
		{
			double val;
			int ret;
			std::cout << "Processing " << image->getFileName () << "..";
			try
			{
				image->getValue ("CRVAL1", val);
				ret = image->toArchive ();
				std::cout << (ret ? "failed (cannot move?)" : "archive") << std::endl;
			}
			catch (rts2core::Error &er)
			{
				ret = image->toTrash ();
				std::cout << (ret ? "failed (cannot move?)" : "trash") << std::endl;
			}
			return 0;
		}
	public:
		Rts2MoveArchive (int in_argc, char **in_argv):Rts2AppDbImage (in_argc,
			in_argv, true)
		{
			Rts2Config *config;
			config = Rts2Config::instance ();
		}
};

int
main (int argc, char **argv)
{
	Rts2MoveArchive app = Rts2MoveArchive (argc, argv);
	return app.run ();
}
