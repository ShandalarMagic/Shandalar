#ifndef PATCH_H	// -*- tab-width:8 -*-
#define PATCH_H	1

// Wherein I do all the things one should not do in a header file.

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#define NOMINMAX
#include <windows.h>

const char* filename;

void
popup(const char* title, const char* fmt, ...)
{
  char buf[8000];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, 8000, fmt, args);
  va_end(args);

  MessageBox(0, buf, title, MB_ICONINFORMATION|MB_TASKMODAL);
}

void
die(const char* fmt, ...)
{
  char e[3000];
  strcpy(e, strerror(errno));

  char buf[8000];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, 8000, fmt, args);
  va_end(args);

  strcat(buf, ": ");
  strcat(buf, e);

  MessageBox(0, buf, "Fatal error", MB_ICONERROR|MB_TASKMODAL);
  exit(1);
}

const char*
buf_to_hexstr(const char* buf, size_t len)
{
  char* hexstr = (char*)malloc(11 * len + 1);	// really only need 5*len + 1, but we're going to die here anyway, so make sure we can fit " 0xffffffff"
  char* p = hexstr;
  size_t i;
  for (i = 0; i < len; ++i, ++buf)
    p += sprintf(p, " 0x%x", (unsigned char)(*buf));
  return hexstr;
}

void
seek_and_write(FILE* f, int seekpos, const char* buf, size_t len)
{
  if (fseek(f, seekpos, SEEK_SET))
    die("Couldn't seek to %x in %s", seekpos, filename);

  if (fwrite(buf, 1, len, f) != len)
    die("Couldn't write%s at %x in %s: %s", buf_to_hexstr(buf, len), seekpos, filename);
}
#define SEEK_AND_WRITE(f, seekpos)	seek_and_write(f, seekpos, INJ, sizeof(INJ) - 1)

#define OPEN(path)			\
  filename = path;			\
  if (!(f = fopen(filename, "r+b")))	\
    die("Couldn't open %s", filename)

#define CLOSE()				\
  if (fclose(f))			\
    die("Couldn't close %s", filename)

#define SUCCESS(exename)				\
  popup(exename, "Successfully patched %s", filename);	\
  return 0

#endif
