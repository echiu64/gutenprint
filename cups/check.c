/*
 * "$Id$"
 *
 *   Raster data checking function for the CUPS driver development kit.
 *
 *   Copyright 1993-2000 by Easy Software Products.
 *
 *   These coded instructions, statements, and computer programs are the
 *   property of Easy Software Products and are protected by Federal
 *   copyright law.  Distribution and use rights are outlined in the file
 *   "LICENSE.txt" which should have been included with this file.  If this
 *   file is missing or damaged please contact Easy Software Products
 *   at:
 *
 *       Attn: CUPS Licensing Information
 *       Easy Software Products
 *       44141 Airport View Drive, Suite 204
 *       Hollywood, Maryland 20636-3111 USA
 *
 *       Voice: (301) 373-9603
 *       EMail: cups-info@cups.org
 *         WWW: http://www.cups.org
 *
 * Contents:
 *
 *   CheckBytes() - Check to see if all bytes match the given value.
 */

/*
 * Include necessary headers...
 */

#include "util.h"


/*
 * 'CheckBytes()' - Check to see if all bytes match the given value.
 */

int					/* O - 1 if they match */
CheckBytes(const unsigned char *bytes,	/* I - Bytes to check */
           int                 length)	/* I - Number of bytes to check */
{
  while (length > 7)
  {
    if (*bytes++)
      return (0);
    if (*bytes++)
      return (0);
    if (*bytes++)
      return (0);
    if (*bytes++)
      return (0);
    if (*bytes++)
      return (0);
    if (*bytes++)
      return (0);
    if (*bytes++)
      return (0);
    if (*bytes++)
      return (0);

    length -= 8;
  }

  while (length > 0)
    if (*bytes++)
      return (0);
    else
      length --;

  return (1);
}


/*
 * End of "$Id$".
 */
