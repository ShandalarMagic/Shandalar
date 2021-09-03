#ifndef FILE_H	// -*- tab-width:8; c-basic-offset:2; -*-
#define FILE_H 1

class File
{
public:
  File(const char* filename, const char* mode = "rb", bool no_die_on_failed_open = false);	// caller assumes responsibility for keeping filename live
  ~File(void);

  void close(void);
  bool ok(void)	{ return f_ != NULL; }

  void seek(long offset, int whence = SEEK_SET);
  void read(void* dest, size_t count, const char* dest_name);
#define READ(dest, count)	read(dest, count, #dest)
  void fgets(char* dest, int sz);
#define FGETS(dest, sz)		fgets(dest, sz)
  char* readline(char* dest, int sz);	// Like fgets, but reads an entire line from file, even if it's longer than sz; also strips the trailing '\n'.  Returns dest, or NULL if nothing read (not even a stripped \n)
  void write(void* src, size_t count, const char* dest_name);
#define WRITE(src, count)	write(src, count, #src)
  void printf(const char* fmt, ...) __attribute__ ((format(printf, 2, 3)));
  void flush(void);

private:
  FILE* f_;
  const char* filename_;
};

#endif
