// -*- c-basic-offset: 2 -*-
#include <stdio.h>
#include <stdlib.h>

#include <windows.h>

USHORT WINAPI RtlCaptureStackBackTrace(ULONG FramesToSkip, ULONG FramesToCapture, PVOID *BackTrace, PULONG BackTraceHash);

void popup(const char* title, const char* fmt, ...);

void
show_backtrace(const char* title, const char* header)
{
  PVOID frames[100];

  int numframes = RtlCaptureStackBackTrace(0, 30, &frames[0], NULL);

  char buf[4000];
  char* p = &buf[0];
  p += sprintf(p, "%s\n", header);
  *p = 0;
  int i;
  for (i = 0; i < numframes; ++i)
	p += sprintf(p, "%d: 0x%p\n", i, frames[i]);

  FILE* f = fopen("dump.dmp", "w");
  fprintf(f, "%s\n%s", title, buf);
  fclose(f);

  p += sprintf(p, "\n"
			   "This information has been included in a file\n"
			   "\"dump.dmp\" in your Manalink program directory.\n"
			   "Please include it (NOT a screenshot) if you\n"
			   "report this bug.");

  popup(title, buf);
}

void
backtrace_and_abort(const char* header)
{
  show_backtrace("Assertion failed", header);
  abort();
}
