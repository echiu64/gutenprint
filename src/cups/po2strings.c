/*
 * "$Id$"
 *
 * Convert GNU gettext .po files to Apple .strings file (UTF-16 BE text file).
 *
 * Usage:
 *
 *   po2strings filename.po filename.strings
 *
 * Compile with:
 *
 *   gcc -o po2strings po2strings.c
 *
 * Contents:
 *
 *   main()          - Convert .po file to .strings.
 *   write_message() - Write a message translation pair.
 *   write_string()  - Write a string to the .strings file.
 *   write_utf16()   - Write a Unicode character using UTF-16.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iconv.h>


/*
 * GNU gettext uses a simple .po file format:
 *
 *   # comment
 *   msgid "id"
 *   "optional continuation"
 *   msgstr "str"
 *   "optional continuation"
 *
 * The .strings file format is also simple:
 *
 *   // comment
 *   "id" = "str";
 *
 * Both the id and str strings use standard C quoting for special characters
 * like newline and the double quote character.
 */

/*
 * Local functions...
 */

static void	write_message(FILE *strings, const char *id, const char *str);
static void	write_string(FILE *strings, const char *s);
static void	write_utf16(FILE *strings, int ch);


/*
 * 'main()' - Convert .po file to .strings.
 */

int					/* O - Exit code */
main(int  argc,				/* I - Number of command-line args */
     char *argv[])			/* I - Command-line arguments */
{
  FILE		*po,			/* .po file */
		*strings;		/* .strings file */
  char		line[4096],		/* Line buffer */
		*ptr,			/* Pointer into buffer */
		id[4096],		/* Translation ID */
		utf8id[4096],		/* UTF-8 translation ID */
		str[4096],		/* Translation string */
		utf8str[4096];		/* UTF-8 translation string */
  int		in_id,			/* Processing "id" string? */
		in_str,			/* Processing "str" string? */
		count,			/* Number of translations */
		linenum;		/* Line number in .po file */
  iconv_t	ic;			/* Transcoder to UTF-8 */
  size_t	inbytes,		/* Number of input buffer bytes */
		outbytes;		/* Number of output buffer bytes */
  char		*inptr,			/* Pointer into input buffer */
		*outptr;		/* Pointer into output buffer */


  if (argc != 3)
  {
    puts("Usage: po2strings filename.po filename.strings");
    return (1);
  }

  if ((po = fopen(argv[1], "rb")) == NULL)
  {
    perror(argv[1]);
    return (1);
  }

  if ((strings = fopen(argv[2], "wb")) == NULL)
  {
    perror(argv[2]);
    return (1);
  }

  write_utf16(strings, 0xfeff);

  count   = 0;
  linenum = 0;
  id[0]   = '\0';
  str[0]  = '\0';
  in_id   = 0;
  in_str  = 0;
  ic      = 0;

  while (fgets(line, sizeof(line), po))
  {
    linenum ++;

   /*
    * Skip blank and comment lines...
    */

    if (line[0] == '#' || line[0] == '\n')
      continue;

   /*
    * Strip the trailing quote...
    */

    if ((ptr = (char *)strrchr(line, '\"')) == NULL)
    {
      fprintf(stderr, "po2strings: Expected quoted string on line %d of %s!\n",
	      linenum, argv[1]);
      return (1);
    }

    *ptr = '\0';

   /*
    * Find start of value...
    */

    if ((ptr = strchr(line, '\"')) == NULL)
    {
      fprintf(stderr, "po2strings: Expected quoted string on line %d of %s!\n",
	      linenum, argv[1]);
      return (1);
    }

    ptr ++;

   /*
    * Create or add to a message...
    */

    if (!strncmp(line, "msgid", 5))
    {
      in_id  = 1;
      in_str = 0;

      if (id[0] && str[0])
      {
        if (ic)
	{
	 /*
	  * Convert ID to UTF-8...
	  */

	  inbytes  = strlen(id);
	  inptr    = id;
	  outbytes = sizeof(utf8id);
	  outptr   = utf8id;

          iconv(ic, &inptr, &inbytes, &outptr, &outbytes);
	  *outptr = '\0';

	 /*
	  * Convert string to UTF-8...
	  */

	  inbytes  = strlen(str);
	  inptr    = str;
	  outbytes = sizeof(utf8str);
	  outptr   = utf8str;

          iconv(ic, &inptr, &inbytes, &outptr, &outbytes);
	  *outptr = '\0';

	 /*
	  * Write it...
	  */

	  write_message(strings, utf8id, utf8str);
	}
	else
	  write_message(strings, id, str);

	count ++;
      }
      else if (!id[0] && str[0] && !ic)
      {
       /*
        * Look for the character set...
	*/

	const char	*charset = strstr(str, "charset=");
					/* Source character set definition */
	char		fromcode[255],	/* Source character set */
			*fromptr;	/* Pointer into fromcode */

	if (charset)
	{
	 /*
	  * Extract character set and setup a transcode context...
	  */

	  strlcpy(fromcode, charset + 8, sizeof(fromcode));
          for (fromptr = fromcode; *fromptr; fromptr ++)
	    if (!isalnum(*fromptr & 255) && *fromptr != '-')
	      break;
          *fromptr = '\0';

          if (strcasecmp(fromcode, "utf-8"))
	  {
            if ((ic = iconv_open("UTF-8", fromcode)) == (iconv_t)-1)
	    {
	      perror(fromcode);
	      ic = 0;
	    }
	  }
        }
      }

      strncpy(id, ptr, sizeof(id) - 1);
      id[sizeof(id) - 1] = '\0';
      str[0] = '\0';
    }
    else if (!strncmp(line, "msgstr", 6))
    {
      in_id  = 0;
      in_str = 1;

      strncpy(str, ptr, sizeof(str) - 1);
      str[sizeof(str) - 1] = '\0';
    }
    else if (line[0] == '\"' && in_str)
    {
      int	str_len = strlen(str),
		ptr_len = strlen(ptr);


      if ((str_len + ptr_len + 1) > sizeof(str))
        ptr_len = sizeof(str) - str_len - 1;

      if (ptr_len > 0)
      {
        memcpy(str + str_len, ptr, ptr_len);
	str[str_len + ptr_len] = '\0';
      }
    }
    else if (line[0] == '\"' && in_id)
    {
      int	id_len = strlen(id),
		ptr_len = strlen(ptr);


      if ((id_len + ptr_len + 1) > sizeof(id))
        ptr_len = sizeof(id) - id_len - 1;

      if (ptr_len > 0)
      {
        memcpy(id + id_len, ptr, ptr_len);
	id[id_len + ptr_len] = '\0';
      }
    }
    else
    {
      fprintf(stderr, "po2strings: Unexpected text on line %d of %s!\n",
	      linenum, argv[1]);
      return (1);
    }
  }

  if (id[0] && str[0])
  {
    if (ic)
    {
     /*
      * Convert ID to UTF-8...
      */

      inbytes  = strlen(id);
      inptr    = id;
      outbytes = sizeof(utf8id);
      outptr   = utf8id;

      iconv(ic, &inptr, &inbytes, &outptr, &outbytes);
      *outptr = '\0';

     /*
      * Convert string to UTF-8...
      */

      inbytes  = strlen(str);
      inptr    = str;
      outbytes = sizeof(utf8str);
      outptr   = utf8str;

      iconv(ic, &inptr, &inbytes, &outptr, &outbytes);
      *outptr = '\0';

     /*
      * Write it...
      */

      write_message(strings, utf8id, utf8str);
    }
    else
      write_message(strings, id, str);

    count ++;
  }

  printf("%s: %d messages.\n", argv[1], count);

  fclose(po);
  fclose(strings);

  return (0);
}


/*
 * 'write_message()' - Write a message translation pair.
 */

static void
write_message(FILE       *strings,	/* I - .strings file */
              const char *id,		/* I - Original text */
	      const char *str)		/* I - Translation text */
{
  write_string(strings, id);
  write_utf16(strings, ' ');
  write_utf16(strings, '=');
  write_utf16(strings, ' ');
  write_string(strings, str);
  write_utf16(strings, ';');
  write_utf16(strings, '\n');
}


/*
 * 'write_string()' - Write a string to the .strings file.
 */

static void
write_string(FILE       *strings,	/* I - .strings file */
             const char *s)		/* I - String to write */
{
  int	ch;				/* Current character */


  write_utf16(strings, '\"');

  while (*s)
  {
    ch = *s++;

    if ((ch & 0xe0) == 0xc0)
    {
     /*
      * Two-byte UTF-8...
      */

      ch = ((ch & 0x1f) << 6) | (*s++ & 0x3f);
    }
    else if ((ch & 0xf0) == 0xe0)
    {
     /*
      * Three-byte UTF-8...
      */

      ch = ((ch & 0x0f) << 6) | (*s++ & 0x3f);
      ch = (ch << 6) | (*s++ & 0x3f);
    }
    else if ((ch & 0xf8) == 0xf0)
    {
     /*
      * Four-byte UTF-8...
      */

      ch = ((ch & 0x07) << 6) | (*s++ & 0x3f);
      ch = (ch << 6) | (*s++ & 0x3f);
      ch = (ch << 6) | (*s++ & 0x3f);
    }

    write_utf16(strings, ch);
  }

  write_utf16(strings, '\"');
}


/*
 * 'write_utf16()' - Write a Unicode character using UTF-16.
 */

static void
write_utf16(FILE *strings,		/* I - .strings file */
            int  ch)			/* I - Unicode character */
{
  unsigned char	buffer[4];		/* Output buffer */


  if (ch < 0x10000)
  {
   /*
    * One-word UTF-16 big-endian...
    */

    buffer[0] = ch >> 8;
    buffer[1] = ch;

    putc(buffer[0], strings);
    putc(buffer[1], strings);
  }
  else
  {
   /*
    * Two-word UTF-16 big-endian...
    */

    ch -= 0x10000;

    buffer[0] = 0xd8 | (ch >> 18);
    buffer[1] = ch >> 10;
    buffer[2] = 0xdc | ((ch >> 8) & 0x03);
    buffer[3] = ch;

    fwrite(buffer, 1, 4, strings);
  }
}


/*
 * End of "$Id$".
 */
