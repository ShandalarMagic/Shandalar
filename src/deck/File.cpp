// -*- tab-width:8; c-basic-offset:2; -*-

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "deckdll.h"
#include "File.h"

// caller assumes responsibility for keeping filename live
File::File(const char* filename, const char* mode, bool no_die_on_failed_open)
{
  this->filename_ = filename;
  this->f_ = fopen(filename_, mode);
  if (!this->f_ && !no_die_on_failed_open)
    fatal("Couldn't open \"%s\": %s", this->filename_, strerror(errno));
}

File::~File(void)
{
  if (this->f_ && fclose(this->f_))
    fatal("Couldn't close \"%s\": %s", this->filename_, strerror(errno));
}

void
File::close(void)
{
  if (this->f_ && fclose(this->f_))
    {
      fatal("Couldn't close \"%s\": %s", this->filename_, strerror(errno));
      return;
    }

  this->f_ = NULL;
}

void
File::seek(long offset, int whence)
{
  if (!this->f_)
    {
      fatal("Couldn't seek in \"%s\": file already closed", this->filename_);
      return;
    }

  if (fseek(this->f_, offset, whence))
    fatal("Couldn't seek in \"%s\": %s", this->filename_, strerror(errno));
}

void
File::read(void* dest, size_t count, const char* dest_name)
{
  if (!this->f_)
    {
      fatal("Couldn't read from \"%s\": file already closed", this->filename_);
      return;
    }

  size_t bytes_read = fread(dest, 1, count, this->f_);
  if (bytes_read != count)
    {
      const char* err = strerror(errno);
      const char* eof = feof(this->f_) ? " (EOF)" : "";
      fatal("Read only %d bytes of %s; expected %d: %s%s", bytes_read, dest_name, count, err, eof);
    }
}

void
File::fgets(char* dest, int sz)
{
  if (!this->f_)
    {
      fatal("Couldn't read from \"%s\": file already closed", this->filename_);
      return;
    }

  if (!::fgets(dest, sz, this->f_))
    {
      if (feof(this->f_))
	*dest = 0;
      else
	{
	  const char* err = strerror(errno);
	  fatal("Couldn't read from %s: %s", this->filename_, err);
	}
    }
}

char*
File::readline(char* dest, int sz)
{
  dest[0] = dest[sz - 1] = 0;
  this->fgets(dest, sz);

  if (!dest[0])	// eof or other error
    return NULL;

  int l = strlen(dest);
  if (dest[l - 1] == '\n')
    {
      dest[l - 1] = 0;
      return dest;
    }

  // Line in file too long; read until eol
  char rest_of_line[512];
  do
    {
      rest_of_line[510] = rest_of_line[511] = 0;
      this->fgets(rest_of_line, 512);
    } while (rest_of_line[0] && rest_of_line[510] && rest_of_line[510] != '\n');
  return dest;
}

void
File::write(void* src, size_t count, const char* src_name)
{
  if (!this->f_)
    {
      fatal("Couldn't write from \"%s\": file already closed", this->filename_);
      return;
    }

  size_t bytes_written = fwrite(src, 1, count, this->f_);
  if (bytes_written != count)
    {
      const char* err = strerror(errno);
      const char* eof = feof(this->f_) ? " (EOF)" : "";
      fatal("Wrote only %d bytes of %s; expected %d: %s%s", bytes_written, src_name, count, err, eof);
    }
}

void
File::printf(const char* fmt, ...)
{
  if (!this->f_)
    {
      fatal("Couldn't write to \"%s\": file already closed", this->filename_);
      return;
    }

  va_list args;
  va_start(args, fmt);
  int rval = vfprintf(this->f_, fmt, args);
  int en = errno;
  va_end(args);

  if (rval < 0)
    fatal("Couldn't write to \"%s\": %s", this->filename_, strerror(en));
}

void
File::flush(void)
{
  if (!this->f_)
    {
      fatal("Couldn't flush \"%s\": file already closed", this->filename_);
      return;
    }

  if (fflush(this->f_))
    fatal("Couldn't flush \"%s\": %s", this->filename_, strerror(errno));
}
