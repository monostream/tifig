/**
 * This file is bluntly stolen from vips internal exif logic. We need this to load exif data into the generated image.
 * Everything we don't need (like thumbnail loading) was removed. The original source can be found here:
 *
 * https://github.com/jcupitt/libvips/blob/master/libvips/foreign/exif.c
 */

/* parse EXIF metadata block out into a set of fields, and reassemble EXIF
 * block from original block, plus modified fields
 *
 * 7/11/16
 *      - from jpeg2vips
 * 14/10/17
 * 	- only read orientation from ifd0
 */

/*

    This file is part of VIPS.

    VIPS is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301  USA

 */

/*

    These files are distributed with VIPS - http://www.vips.ecs.soton.ac.uk

 */

#include <vips/intl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <vips/vips.h>
#include <vips/debug.h>

#include <libexif/exif-data.h>


/* Like exif_data_new_from_data(), but don't default missing fields.
 *
 * If we do exif_data_new_from_data(), then missing fields are set to
 * their default value and we won't know about it.
 */
static ExifData *
vips_exif_load_data_without_fix( void *data, int length )
{
	ExifData *ed;

	if( !(ed = exif_data_new()) ) {
		vips_error( "exif", "%s", _( "unable to init exif" ) );
		return( NULL );
	}

	exif_data_unset_option( ed, EXIF_DATA_OPTION_FOLLOW_SPECIFICATION );
	exif_data_load_data( ed, data, length );

	return( ed );
}

static int
vips_exif_get_int( ExifData *ed,
	ExifEntry *entry, unsigned long component, int *out )
{
	ExifByteOrder bo = exif_data_get_byte_order( ed );
	size_t sizeof_component = entry->size / entry->components;
	size_t offset = component * sizeof_component;

	if( entry->format == EXIF_FORMAT_SHORT )
		*out = exif_get_short( entry->data + offset, bo );
	else if( entry->format == EXIF_FORMAT_SSHORT )
		*out = exif_get_sshort( entry->data + offset, bo );
	else if( entry->format == EXIF_FORMAT_LONG )
		/* This won't work for huge values, but who cares.
		 */
		*out = (int) exif_get_long( entry->data + offset, bo );
	else if( entry->format == EXIF_FORMAT_SLONG )
		*out = exif_get_slong( entry->data + offset, bo );
	else
		return( -1 );

	return( 0 );
}

static int
vips_exif_get_rational( ExifData *ed,
	ExifEntry *entry, unsigned long component, ExifRational *out )
{
	if( entry->format == EXIF_FORMAT_RATIONAL ) {
		ExifByteOrder bo = exif_data_get_byte_order( ed );
		size_t sizeof_component = entry->size / entry->components;
		size_t offset = component * sizeof_component;

		*out = exif_get_rational( entry->data + offset, bo );
	}
	else
		return( -1 );

	return( 0 );
}

static int
vips_exif_get_srational( ExifData *ed,
	ExifEntry *entry, unsigned long component, ExifSRational *out )
{
	if( entry->format == EXIF_FORMAT_SRATIONAL ) {
		ExifByteOrder bo = exif_data_get_byte_order( ed );
		size_t sizeof_component = entry->size / entry->components;
		size_t offset = component * sizeof_component;

		*out = exif_get_srational( entry->data + offset, bo );
	}
	else
		return( -1 );

	return( 0 );
}

static int
vips_exif_get_double( ExifData *ed,
	ExifEntry *entry, unsigned long component, double *out )
{
	ExifRational rv;
	ExifSRational srv;

	if( !vips_exif_get_rational( ed, entry, component, &rv ) )
		*out = (double) rv.numerator / rv.denominator;
	else if( !vips_exif_get_srational( ed, entry, component, &srv ) )
		*out = (double) srv.numerator / srv.denominator;
	else
		return( -1 );

	return( 0 );
}

/* Save an exif value to a string in a way that we can restore. We only bother
 * for the simple formats (that a client might try to change) though.
 *
 * Keep in sync with vips_exif_from_s() below.
 */
static void
vips_exif_to_s( ExifData *ed, ExifEntry *entry, VipsBuf *buf )
{
	unsigned long i;
	int iv;
	ExifRational rv;
	ExifSRational srv;
	char txt[256];

	if( entry->format == EXIF_FORMAT_ASCII )  {
		/* libexif does not null-terminate strings. Copy out and add
		 * the \0 ourselves.
		 */
		int len = VIPS_MIN( 254, entry->size );

		memcpy( txt, entry->data, len );
		txt[len] = '\0';
		vips_buf_appendf( buf, "%s ", txt );
	}
	else if( entry->components < 10 &&
		!vips_exif_get_int( ed, entry, 0, &iv ) ) {
		for( i = 0; i < entry->components; i++ ) {
			vips_exif_get_int( ed, entry, i, &iv );
			vips_buf_appendf( buf, "%d ", iv );
		}
	}
	else if( entry->components < 10 &&
		!vips_exif_get_rational( ed, entry, 0, &rv ) ) {
		for( i = 0; i < entry->components; i++ ) {
			vips_exif_get_rational( ed, entry, i, &rv );
			vips_buf_appendf( buf, "%u/%u ",
				rv.numerator, rv.denominator );
		}
	}
	else if( entry->components < 10 &&
		!vips_exif_get_srational( ed, entry, 0, &srv ) ) {
		for( i = 0; i < entry->components; i++ ) {
			vips_exif_get_srational( ed, entry, i, &srv );
			vips_buf_appendf( buf, "%d/%d ",
				srv.numerator, srv.denominator );
		}
	}
	else
		vips_buf_appendf( buf, "%s ",
			exif_entry_get_value( entry, txt, 256 ) );

	vips_buf_appendf( buf, "(%s, %s, %lu components, %d bytes)",
		exif_entry_get_value( entry, txt, 256 ),
		exif_format_get_name( entry->format ),
		entry->components,
		entry->size );
}

typedef struct _VipsExifParams {
	VipsImage *image;
	ExifData *ed;
} VipsExifParams;

static void
vips_exif_attach_entry( ExifEntry *entry, VipsExifParams *params )
{
	const char *tag_name;
	char vips_name_txt[256];
	VipsBuf vips_name = VIPS_BUF_STATIC( vips_name_txt );
	char value_txt[256];
	VipsBuf value = VIPS_BUF_STATIC( value_txt );

	if( !(tag_name = exif_tag_get_name( entry->tag )) )
		return;

	vips_buf_appendf( &vips_name, "exif-ifd%d-%s",
		exif_entry_get_ifd( entry ), tag_name );
	vips_exif_to_s( params->ed, entry, &value );

	/* Can't do anything sensible with the error return.
	 */
	(void) vips_image_set_string( params->image,
		vips_buf_all( &vips_name ), vips_buf_all( &value ) );
}

static void
vips_exif_get_content( ExifContent *content, VipsExifParams *params )
{
        exif_content_foreach_entry( content,
		(ExifContentForeachEntryFunc) vips_exif_attach_entry, params );
}

static int
vips_exif_entry_get_double( ExifData *ed, int ifd, ExifTag tag, double *out )
{
	ExifEntry *entry;

	if( !(entry = exif_content_get_entry( ed->ifd[ifd], tag )) ||
		entry->components != 1 )
		return( -1 );

	return( vips_exif_get_double( ed, entry, 0, out ) );
}

static int
vips_exif_entry_get_int( ExifData *ed, int ifd, ExifTag tag, int *out )
{
	ExifEntry *entry;

	if( !(entry = exif_content_get_entry( ed->ifd[ifd], tag )) ||
		entry->components != 1 )
		return( -1 );

	return( vips_exif_get_int( ed, entry, 0, out ) );
}

/* Set the image resolution from the EXIF tags.
 */
static int
vips_image_resolution_from_exif( VipsImage *image, ExifData *ed )
{
	double xres, yres;
	int unit;

	/* The main image xres/yres are in ifd0. ifd1 has xres/yres of the
	 * image thumbnail, if any.
	 *
	 * Don't warn about missing res fields, it's very common, especially for
	 * things like webp.
	 */
	if( vips_exif_entry_get_double( ed, 0, EXIF_TAG_X_RESOLUTION, &xres ) ||
		vips_exif_entry_get_double( ed,
			0, EXIF_TAG_Y_RESOLUTION, &yres ) ||
		vips_exif_entry_get_int( ed,
			0, EXIF_TAG_RESOLUTION_UNIT, &unit ) )
		return( -1 );

	switch( unit ) {
	case 1:
		/* No unit ... just pass the fields straight to vips.
		 */
		vips_image_set_string( image,
			VIPS_META_RESOLUTION_UNIT, "none" );
		break;

	case 2:
		/* In inches.
		 */
		xres /= 25.4;
		yres /= 25.4;
		vips_image_set_string( image,
			VIPS_META_RESOLUTION_UNIT, "in" );
		break;

	case 3:
		/* In cm.
		 */
		xres /= 10.0;
		yres /= 10.0;
		vips_image_set_string( image,
			VIPS_META_RESOLUTION_UNIT, "cm" );
		break;

	default:
		vips_warn( "exif",
			"%s", _( "unknown EXIF resolution unit" ) );
		return( -1 );
	}

	image->Xres = xres;
	image->Yres = yres;

	return( 0 );
}

/* Need to fwd ref this.
 */
static int
vips_exif_resolution_from_image( ExifData *ed, VipsImage *image );

/* Scan the exif block on the image, if any, and make a set of vips metadata
 * tags for what we find.
 */
int
vips_exif_parse(VipsImage *image)
{
	void *data;
	size_t length;
	ExifData *ed;
	VipsExifParams params;
	const char *str;

	if( !vips_image_get_typeof( image, VIPS_META_EXIF_NAME ) )
		return( 0 );
	if( vips_image_get_blob( image, VIPS_META_EXIF_NAME, &data, &length ) )
		return( -1 );
	if( !(ed = vips_exif_load_data_without_fix( data, length )) )
		return( -1 );

	/* Look for resolution fields and use them to set the VIPS xres/yres
	 * fields.
	 *
	 * If the fields are missing, set them from the image, which will have
	 * previously had them set from something like JFIF.
	 */
	if( vips_image_resolution_from_exif( image, ed ) &&
		vips_exif_resolution_from_image( ed, image ) ) {
		exif_data_free( ed );
		return( -1 );
	}

	/* Make sure all required fields are there before we attach to vips
	 * metadata.
	 */
	exif_data_fix( ed );

	/* Attach informational fields for what we find.
	 */
	params.image = image;
	params.ed = ed;
	exif_data_foreach_content( ed,
		(ExifDataForeachContentFunc) vips_exif_get_content, &params );

	exif_data_free( ed );

	/* Orientation handling. ifd0 has the Orientation tag for the main
	 * image.
	 */
	if( vips_image_get_typeof( image, "exif-ifd0-Orientation" ) != 0 &&
		!vips_image_get_string( image,
			"exif-ifd0-Orientation", &str ) ) {
		int orientation;

		orientation = atoi( str );
		orientation = VIPS_CLIP( 1, orientation, 8 );
		vips_image_set_int( image, VIPS_META_ORIENTATION, orientation );
	}

	return( 0 );
}

static void
vips_exif_set_int( ExifData *ed,
	ExifEntry *entry, unsigned long component, void *data )
{
	int value = *((int *) data);

	ExifByteOrder bo;
	size_t sizeof_component;
	size_t offset = component;

	if( entry->components <= component ) {
		VIPS_DEBUG_MSG( "vips_exif_set_int: too few components\n" );
		return;
	}

	/* Wait until after the component check to make sure we cant get /0.
	 */
	bo = exif_data_get_byte_order( ed );
	sizeof_component = entry->size / entry->components;
	offset = component * sizeof_component;

	VIPS_DEBUG_MSG( "vips_exif_set_int: %s = %d\n",
		exif_tag_get_name( entry->tag ), value );

	if( entry->format == EXIF_FORMAT_SHORT )
		exif_set_short( entry->data + offset, bo, value );
	else if( entry->format == EXIF_FORMAT_SSHORT )
		exif_set_sshort( entry->data + offset, bo, value );
	else if( entry->format == EXIF_FORMAT_LONG )
		exif_set_long( entry->data + offset, bo, value );
	else if( entry->format == EXIF_FORMAT_SLONG )
		exif_set_slong( entry->data + offset, bo, value );
}

static void
vips_exif_double_to_rational( double value, ExifRational *rv )
{
	/* We will usually set factors of 10, so use 1000 as the denominator
	 * and it'll probably be OK.
	 */
	rv->numerator = value * 1000;
	rv->denominator = 1000;
}

static void
vips_exif_double_to_srational( double value, ExifSRational *srv )
{
	/* We will usually set factors of 10, so use 1000 as the denominator
	 * and it'll probably be OK.
	 */
	srv->numerator = value * 1000;
	srv->denominator = 1000;
}

/* Does both signed and unsigned rationals from a double*.
 *
 * Don't change the exit entry if the value currently there is a good
 * approximation of the double we are trying to set.
 */
static void
vips_exif_set_double( ExifData *ed,
	ExifEntry *entry, unsigned long component, void *data )
{
	double value = *((double *) data);

	ExifByteOrder bo;
	size_t sizeof_component;
	size_t offset;
	double old_value;

	if( entry->components <= component ) {
		VIPS_DEBUG_MSG( "vips_exif_set_double: "
			"too few components\n" );
		return;
	}

	/* Wait until after the component check to make sure we cant get /0.
	 */
	bo = exif_data_get_byte_order( ed );
	sizeof_component = entry->size / entry->components;
	offset = component * sizeof_component;

	VIPS_DEBUG_MSG( "vips_exif_set_double: %s = %g\n",
		exif_tag_get_name( entry->tag ), value );

	if( entry->format == EXIF_FORMAT_RATIONAL ) {
		ExifRational rv;

		rv = exif_get_rational( entry->data + offset, bo );
		old_value = (double) rv.numerator / rv.denominator;
		if( VIPS_FABS( old_value - value ) > 0.0001 ) {
			vips_exif_double_to_rational( value, &rv );

			VIPS_DEBUG_MSG( "vips_exif_set_double: %u / %u\n",
				rv.numerator,
				rv.denominator );

			exif_set_rational( entry->data + offset, bo, rv );
		}
	}
	else if( entry->format == EXIF_FORMAT_SRATIONAL ) {
		ExifSRational srv;

		srv = exif_get_srational( entry->data + offset, bo );
		old_value = (double) srv.numerator / srv.denominator;
		if( VIPS_FABS( old_value - value ) > 0.0001 ) {
			vips_exif_double_to_srational( value, &srv );

			VIPS_DEBUG_MSG( "vips_exif_set_double: %d / %d\n",
				srv.numerator, srv.denominator );

			exif_set_srational( entry->data + offset, bo, srv );
		}
	}
}

typedef void (*write_fn)( ExifData *ed,
	ExifEntry *entry, unsigned long component, void *data );

/* Write a tag. Update what's there, or make a new one.
 */
static void
vips_exif_set_tag( ExifData *ed, int ifd, ExifTag tag, write_fn fn, void *data )
{
	ExifEntry *entry;

	if( (entry = exif_content_get_entry( ed->ifd[ifd], tag )) ) {
		fn( ed, entry, 0, data );
	}
	else {
		entry = exif_entry_new();

		/* tag must be set before calling exif_content_add_entry.
		 */
		entry->tag = tag;

		exif_content_add_entry( ed->ifd[ifd], entry );
		exif_entry_initialize( entry, tag );
		exif_entry_unref( entry );

		fn( ed, entry, 0, data );
	}
}

/* Set the EXIF resolution from the vips xres/yres tags.
 */
static int
vips_exif_resolution_from_image( ExifData *ed, VipsImage *image )
{
	double xres, yres;
	const char *p;
	int unit;

	VIPS_DEBUG_MSG( "vips_exif_resolution_from_image: vips res of %g, %g\n",
		image->Xres, image->Yres );

	/* Default to inches, more progs support it.
	 */
	unit = 2;
	if( vips_image_get_typeof( image, VIPS_META_RESOLUTION_UNIT ) &&
		!vips_image_get_string( image,
			VIPS_META_RESOLUTION_UNIT, &p ) ) {
		if( vips_isprefix( "cm", p ) )
			unit = 3;
		else if( vips_isprefix( "none", p ) )
			unit = 1;
	}

	switch( unit ) {
	case 1:
		xres = image->Xres;
		yres = image->Yres;
		break;

	case 2:
		xres = image->Xres * 25.4;
		yres = image->Yres * 25.4;
		break;

	case 3:
		xres = image->Xres * 10.0;
		yres = image->Yres * 10.0;
		break;

	default:
		vips_warn( "exif",
			"%s", _( "unknown EXIF resolution unit" ) );
		return( 0 );
	}

	/* Main image xres/yres/unit are in ifd0. ifd1 has the thumbnail
	 * xres/yres/unit.
	 */
	vips_exif_set_tag( ed, 0, EXIF_TAG_X_RESOLUTION,
		vips_exif_set_double, (void *) &xres );
	vips_exif_set_tag( ed, 0, EXIF_TAG_Y_RESOLUTION,
		vips_exif_set_double, (void *) &yres );
	vips_exif_set_tag( ed, 0, EXIF_TAG_RESOLUTION_UNIT,
		vips_exif_set_int, (void *) &unit );

	return( 0 );
}
