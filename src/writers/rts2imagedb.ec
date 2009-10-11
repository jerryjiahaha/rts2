/* 
 * Database image access classes.
 * Copyright (C) 2003-2007 Petr Kubanek <petr@kubanek.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "imgdisplay.h"
#include "rts2imagedb.h"
#include "../utils/timestamp.h"
#include "../utils/libnova_cpp.h"

#include <iomanip>
#include <libnova/airmass.h>
#include <sstream>

EXEC SQL include sqlca;

void
Rts2ImageDb::initDbImage ()
{
}


void
Rts2ImageDb::reportSqlError (const char *msg)
{
	logStream (MESSAGE_ERROR) << "SQL error #" << sqlca.sqlcode << " text " << sqlca.sqlerrm.sqlerrmc << " (in " <<
		msg << ")" << sendLog;
}


void Rts2ImageDb::getValueInd (const char *name, double &value, int &ind, char *comment)
{
	try
	{
		getValue (name, value, comment);
	}
	catch (rts2core::Error &er)
	{
		value = 100;
		ind = -1;
	}
}


void Rts2ImageDb::getValueInd (const char *name, float &value, int &ind, char *comment)
{
	try
	{
		getValue (name, value, comment);
	}
	catch (rts2core::Error &er)
	{
		value = 100;
		ind = -1;
	}
}


Rts2ImageDb::Rts2ImageDb (Rts2Image * in_image): Rts2Image (in_image)
{
}


Rts2ImageDb::Rts2ImageDb (Rts2Target *currTarget, Rts2DevClientCamera * camera, const struct timeval *expStart) :
Rts2Image (currTarget, camera, expStart)
{
	initDbImage ();
}


Rts2ImageDb::Rts2ImageDb (const char *in_filename, bool verbose, bool readOnly) : Rts2Image (in_filename, verbose, readOnly)
{
	initDbImage ();
}


Rts2ImageDb::Rts2ImageDb (int in_obs_id, int in_img_id) : Rts2Image ()
{
	initDbImage ();
	// fill in filter
	// fill in exposureStart
	// fill in average (for darks..)
	// fill in target_id
	// fill in epochId
	// fill in camera_name
	// fill in mount_name
	// fill in obsId
	// fill in imgId
	// fill in ra_err
	// fill in dec_err
	// fill in pos_astr (from astrometry)
	// fill in imageType
	// fill in imageName
}


Rts2ImageDb::Rts2ImageDb (long in_img_date, int in_img_usec, float in_img_exposure):Rts2Image (in_img_date, in_img_usec, in_img_exposure)
{
}


int
Rts2ImageDb::getOKCount ()
{
	EXEC SQL BEGIN DECLARE SECTION;
		int db_obs_id = getObsId ();
		int db_count = 0;
	EXEC SQL END DECLARE SECTION;

	EXEC SQL
		SELECT
		count (*)
		INTO
			:db_count
		FROM
			images
		WHERE
			obs_id = :db_obs_id
		AND ((process_bitfield & 2) = 2);
	EXEC SQL ROLLBACK;

	return db_count;
}


int
Rts2ImageDb::saveImage ()
{
	updateDB ();
	return Rts2Image::saveImage ();
}


int
Rts2ImageDb::renameImage (const char *new_filename)
{
	EXEC SQL BEGIN DECLARE SECTION;
	VARCHAR d_img_path[100];
	int d_img_id = imgId;
	int d_obs_id = obsId;
	EXEC SQL END DECLARE SECTION;

	int ret = Rts2Image::renameImage (new_filename);
	if (ret)
		return ret;

	strncpy (d_img_path.arr, getAbsoluteFileName (), 100);
	d_img_path.len = strlen (getAbsoluteFileName ()) > 100 ? 100: strlen (getAbsoluteFileName ());

	EXEC SQL UPDATE
		images
	SET
		img_path = :d_img_path
	WHERE
		img_id = :d_img_id AND obs_id = :d_obs_id;
	if (sqlca.sqlcode != 0)
	{
		reportSqlError ("renameImage");
		EXEC SQL ROLLBACK;
		return -1;
	}
	EXEC SQL COMMIT;
	return 0;
}


std::ostream & operator << (std::ostream &_os, Rts2ImageDb &img_db)
{
	img_db.print (_os);
	return _os;
}


/*********************************************************************
 *
 * Rts2ImageSkyDb class
 *
 ********************************************************************/

int
Rts2ImageSkyDb::updateDB ()
{
	EXEC SQL BEGIN DECLARE SECTION;
		VARCHAR d_img_path[100];
		int d_img_id = imgId;
		int d_obs_id = obsId;
		char d_obs_subtype = 'S';
		int d_img_date = getExposureSec ();
		int d_img_usec = getExposureUsec ();
		float d_img_temperature = -500;
		int d_img_temperature_ind;
		float d_img_exposure = -1;
		float d_img_alt = -100;
		float d_img_az = -100;
		int d_med_id = 0;
		int d_proccess_bitfield = processBitfiedl;
		float d_img_fwhm;
		int d_img_fwhm_ind;
		float d_img_limmag;
		int d_img_limmag_ind;
		float d_img_qmagmax;
		int d_img_qmagmax_ind;
		VARCHAR d_mount_name[8];
		VARCHAR d_camera_name[8];
		VARCHAR d_img_filter[3];
	EXEC SQL END DECLARE SECTION;

	strncpy (d_img_path.arr, getAbsoluteFileName (), 100);
	d_img_path.len = strlen (getAbsoluteFileName ()) > 100 ? 100: strlen (getAbsoluteFileName ());

	strncpy (d_mount_name.arr, getMountName (), 8);
	d_mount_name.len = strlen (getMountName ());

	strncpy (d_camera_name.arr, getCameraName (), 8);
	d_camera_name.len = strlen (getCameraName ());

	d_img_filter.len = strlen (getFilter()) > 3 ? 3 : strlen (getFilter());
	strncpy (d_img_filter.arr, getFilter(), d_img_filter.len);

	d_img_exposure = getExposureLength ();
	getValue ("ALT", d_img_alt);
	getValue ("AZ", d_img_az);

	getValueInd ("CCD_TEMP", d_img_temperature, d_img_temperature_ind);
	getValueInd ("FWHM", d_img_fwhm, d_img_fwhm_ind);
	getValueInd ("QMAGMAX", d_img_qmagmax, d_img_limmag_ind);
	getValueInd ("LIMMAG", d_img_limmag, d_img_limmag_ind);

	EXEC SQL
		INSERT INTO
			images
			(
			img_path,
			img_id,
			obs_id,
			obs_subtype,
			mount_name,
			camera_name,
			img_temperature,
			img_exposure,
			img_filter,
			img_alt,
			img_az,
			img_date,
			img_usec,
			med_id,
			process_bitfield,
			img_fwhm,
			img_limmag,
			img_qmagmax
			)
		VALUES
			(
			:d_img_path,
			:d_img_id,
			:d_obs_id,
			:d_obs_subtype,
			:d_mount_name,
			:d_camera_name,
			:d_img_temperature :d_img_temperature_ind,
			:d_img_exposure,
			:d_img_filter,
			:d_img_alt,
			:d_img_az,
			to_timestamp (:d_img_date),
			:d_img_usec,
			:d_med_id,
			:d_proccess_bitfield,
			:d_img_fwhm :d_img_fwhm_ind,
			:d_img_limmag :d_img_limmag_ind,
			:d_img_qmagmax :d_img_qmagmax_ind
			);
	// data found..do just update
	if (sqlca.sqlcode != 0)
	{
		if (sqlca.sqlcode != ECPG_PGSQL)
		{
			printf ("Cannot insert new image, triyng update (error: %li %s)\n",
				sqlca.sqlcode, sqlca.sqlerrm.sqlerrmc);
		}
		EXEC SQL ROLLBACK;
		EXEC SQL
			UPDATE
				images
			SET
				img_path = :d_img_path,
				img_date = to_timestamp (:d_img_date),
				img_usec = :d_img_usec,
				med_id   = :d_med_id,
				process_bitfield = :d_proccess_bitfield,
				img_fwhm = :d_img_fwhm :d_img_fwhm_ind,
				img_limmag = :d_img_limmag :d_img_limmag_ind,
				img_qmagmax = :d_img_qmagmax :d_img_qmagmax_ind
			WHERE
				img_id = :d_img_id AND obs_id = :d_obs_id;
		// still error.. return -1
		if (sqlca.sqlcode != 0)
		{
			reportSqlError ("image OBJECT update");
			EXEC SQL ROLLBACK;
			return -1;
		}
	}
	EXEC SQL COMMIT;
	return updateAstrometry ();
}


int
Rts2ImageSkyDb::updateAstrometry ()
{
	EXEC SQL BEGIN DECLARE SECTION;
		int d_obs_id = obsId;
		int d_img_id = imgId;

		double d_img_err_ra;
		double d_img_err_dec;
		double d_img_err;

		VARCHAR s_astrometry[2000];
	EXEC SQL END DECLARE SECTION;

	long a_naxis[2];
	char *ctype[2];
	double crpix[2];
	double crval[2];
	double cdelt[2];
	double crota[2];
	double equinox;

	ctype[0] = (char *) malloc (10);
	ctype[1] = (char *) malloc (10);

	try
	{
		getValues ("NAXIS", a_naxis, 2);
		getValues ("CTYPE", (char **) &ctype, 2);
		getValues ("CRPIX", crpix, 2);
		getValues ("CRVAL", crval, 2);
		getValues ("CDELT", cdelt, 2);
		getValues ("CROTA", crota, 2);
		getValue ("EQUINOX", equinox);
	}
	catch (rts2core::Error &er)
	{
		return -1;
	}

	d_img_err_ra = ra_err;
	d_img_err_dec = dec_err;
	d_img_err = getAstrometryErr ();

	snprintf (s_astrometry.arr, 2000,
		"NAXIS1 %ld NAXIS2 %ld CTYPE1 %s CTYPE2 %s CRPIX1 %f CRPIX2 %f "
		"CRVAL1 %f CRVAL2 %f CDELT1 %f CDELT2 %f CROTA %f EQUINOX %f",
		a_naxis[0], a_naxis[1], ctype[0], ctype[1], crpix[0], crpix[1],
		crval[0], crval[1], cdelt[0], cdelt[1], crota[0], equinox);
	s_astrometry.len = strlen (s_astrometry.arr);

	EXEC SQL UPDATE
			images
		SET
			astrometry  = :s_astrometry,
			img_err_ra  = :d_img_err_ra,
			img_err_dec = :d_img_err_dec,
			img_err     = :d_img_err,
			process_bitfield = process_bitfield | 1 | 2
		WHERE
			obs_id = :d_obs_id
		AND img_id = :d_img_id;
	EXEC SQL COMMIT;
	if (sqlca.sqlcode != 0)
	{
		reportSqlError ("astrometry update");
		return -1;
	}
	processBitfiedl |= ASTROMETRY_PROC | ASTROMETRY_OK;
	return 0;
}


void
Rts2ImageSkyDb::initDbImage ()
{
	processBitfiedl = 0;
	getValue ("PROC", processBitfiedl);
}


int
Rts2ImageSkyDb::isCalibrationImage ()
{
	return (getTargetType () == TYPE_CALIBRATION
		|| getTargetType () == TYPE_PHOTOMETRIC);
}


void
Rts2ImageSkyDb::updateCalibrationDb ()
{
	EXEC SQL BEGIN DECLARE SECTION;
		int db_air_last_image;
		float db_airmass;
		int db_obs_id = getObsId ();
		int db_img_id = getImgId ();
	EXEC SQL END DECLARE SECTION;

	double img_alt;

	// we have calibration image processed..update airmass_cal_images table
	if (isCalibrationImage ())
	{
		try
		{
			getValue ("ALT", img_alt);
			db_airmass = ln_get_airmass (img_alt, 750.0);
			db_air_last_image = getExposureSec ();
			// delete - unset our old references (if any exists..)
			EXEC SQL
				UPDATE
					airmass_cal_images
				SET
					air_last_image = to_timestamp (0),
					obs_id = NULL,
					img_id = NULL
				WHERE
					obs_id = :db_obs_id
				AND img_id = :db_img_id;

			if (sqlca.sqlcode && sqlca.sqlcode != ECPG_NOT_FOUND)
			{
				reportSqlError ("Rts2Image::updateCalibrationDb unseting airmass_cal_images");
				EXEC SQL ROLLBACK;
			}
			else
			{
				EXEC SQL COMMIT;
			}
			// if not processed, or processed and not failed..
			if (!(processBitfiedl & ASTROMETRY_PROC)
				|| (processBitfiedl & ASTROMETRY_OK))
			{
				EXEC SQL
					UPDATE
						airmass_cal_images
					SET
						air_last_image = to_timestamp (:db_air_last_image),
						obs_id = :db_obs_id,
						img_id = :db_img_id
					WHERE
						air_airmass_start <= :db_airmass
					AND :db_airmass < air_airmass_end
					AND air_last_image <= to_timestamp (:db_air_last_image);

				if (sqlca.sqlcode)
				{
					reportSqlError ("Rts2Image::toArchive updating airmass_cal_images");
					EXEC SQL ROLLBACK;
				}
				else
				{
					EXEC SQL COMMIT;
				}
			}
		}
		catch (rts2core::Error &er)
		{
		}
	}
}


Rts2ImageSkyDb::Rts2ImageSkyDb (Rts2Target *currTarget, Rts2DevClientCamera * camera, const struct timeval *expStart) :
Rts2ImageDb (currTarget, camera, expStart)
{
	initDbImage ();
}


Rts2ImageSkyDb::Rts2ImageSkyDb (const char *in_filename) : Rts2ImageDb (in_filename)
{
	initDbImage ();
}


Rts2ImageSkyDb::Rts2ImageSkyDb (Rts2Image * in_image): Rts2ImageDb (in_image)
{
	initDbImage ();
}


Rts2ImageSkyDb::Rts2ImageSkyDb (int in_obs_id, int in_img_id) : Rts2ImageDb (in_obs_id, in_img_id)
{
	initDbImage ();
}


Rts2ImageSkyDb::Rts2ImageSkyDb (int in_tar_id, int in_obs_id, int in_img_id, char in_obs_subtype, long in_img_date, int in_img_usec,
float in_img_exposure, float in_img_temperature, const char *in_img_filter, float in_img_alt, float in_img_az, const char *in_camera_name,
const char *in_mount_name, bool in_delete_flag, int in_process_bitfield, double in_img_err_ra, double in_img_err_dec,
double in_img_err, const char *_img_path) : Rts2ImageDb (in_img_date, in_img_usec, in_img_exposure)
{
	targetId = in_tar_id;
	targetIdSel = in_tar_id;
	targetType = in_obs_subtype;
	targetName = NULL;
	obsId = in_obs_id;
	imgId = in_img_id;
	cameraName = new char[strlen (in_camera_name) + 1];
	strcpy (cameraName, in_camera_name);
	mountName = new char[strlen (in_mount_name) + 1];
	strcpy (mountName, in_mount_name);
	focName = NULL;
	// construct image name..

	ra_err = in_img_err_ra;
	dec_err = in_img_err_dec;
	img_err = in_img_err;

	// TODO fill that..
	pos_astr.ra = nan ("f");
	pos_astr.dec = nan ("f");
	processBitfiedl = in_process_bitfield;

	setFilter (in_img_filter);
	setFileName (_img_path);
}


Rts2ImageSkyDb::~Rts2ImageSkyDb ()
{
}


int
Rts2ImageSkyDb::toArchive ()
{
	int ret;

	processBitfiedl |= ASTROMETRY_OK | ASTROMETRY_PROC;

	ret = Rts2Image::toArchive ();
	if (ret)
	{
		processBitfiedl |= IMG_ERR;
		return ret;
	}

	return 0;
}


int
Rts2ImageSkyDb::toTrash ()
{
	int ret;

	processBitfiedl &= (~ASTROMETRY_OK);
	processBitfiedl |= ASTROMETRY_PROC;

	ret = Rts2Image::toTrash ();
	if (ret)
		return ret;

	return 0;
}


// write changes of image to DB..

int
Rts2ImageSkyDb::saveImage ()
{
	if (!shouldSaveImage())
		return 0;
	int ret = Rts2ImageDb::saveImage ();
	if (ret)
		return ret;
	updateCalibrationDb ();
	setValue ("PROC", processBitfiedl, "procesing status; info in DB");
	return 0;
}


int
Rts2ImageSkyDb::deleteImage ()
{
	EXEC SQL BEGIN DECLARE SECTION;
		int d_img_id = imgId;
		int d_obs_id = obsId;
	EXEC SQL END DECLARE SECTION;

	if (getImageType () == IMGTYPE_OBJECT)
	{
		EXEC SQL
			DELETE FROM
				images
			WHERE
				img_id = :d_img_id
			AND obs_id = :d_obs_id;
	}
	if (sqlca.sqlcode)
	{
		reportSqlError ("Rts2ImageSkyDb::deleteImage");
		EXEC SQL ROLLBACK;
	}
	else
	{
		EXEC SQL COMMIT;
	}
	return Rts2Image::deleteImage ();
}


std::string
Rts2ImageSkyDb::getFileNameString ()
{
	std::ostringstream out;
	printFileName (out);
	return out.str();
}
