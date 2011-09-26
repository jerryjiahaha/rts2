#ifndef __RTS2_APP_IMAGE__
#define __RTS2_APP_IMAGE__

#include "../rts2/rts2cliapp.h"
#include "image.h"

#include <list>

namespace rts2image
{

class AppImageCore:public Rts2CliApp
{
	protected:
		std::list < const char *>imageNames;
		bool readOnly;
		virtual int processImage (Image * image) { return 0; }
	public:
		AppImageCore (int in_argc, char **in_argv, bool in_readOnly):Rts2CliApp (in_argc, in_argv)
		{
			readOnly = in_readOnly;
		}
		virtual ~ AppImageCore (void)
		{
			imageNames.clear ();
		}
		virtual int processArgs (const char *in_arg)
		{
			imageNames.push_back (in_arg);
			return 0;
		}

		virtual int doProcessing ()
		{
			int ret = 0;
			std::list < const char *>::iterator img_iter;
			for (img_iter = imageNames.begin (); img_iter != imageNames.end (); img_iter++)
			{
				const char *an_name = *img_iter;
				Image *image = new Image ();
				image->openImage (an_name, false, readOnly);
				ret = processImage (image);
				delete image;
				if (ret)
					return ret;
			}
			return ret;
		}
};

}

#endif							 /* !__RTS2_APP_IMAGE__ */