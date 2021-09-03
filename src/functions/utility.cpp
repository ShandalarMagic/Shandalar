// -*- c-basic-offset:2 -*-

#include <algorithm>
#include <set>
#include <unordered_map>

#include "manalink.h"

/*********************************
* C interface to std::set<void*> *
*********************************/

struct SetVoidptr_t
{
  std::set<void*> m;
};

// Inserts val into the set described by s, creating the set first if s is NULL.  Returns nonzero if val was already in the set.
int
SetVoidptr_insert(SetVoidptr* s, void* val)
{
  if (!*s)
	*s = new SetVoidptr_t;

  return (*s)->m.insert(val).second;
}

/***************************
* C interface to std::sort *
***************************/

static bool
less_const_char(const char* lhs, const char* rhs)
{
  return strcmp(lhs, rhs) < 0;
}

// In-place textual sort on a num_elements-length array of const char*.
void
sort_ArrayConstCharPtr(const char** arr, int num_elements)
{
  std::sort(arr, arr + num_elements, less_const_char);
}

/**********************************************
* C Interface to std::unordered_map<int, int> *
**********************************************/

struct UnorderedMapIntInt_t
{
  std::unordered_map<int, int> m;
};

// Returns a pointer to the value for key in the map described by m.  If key isn't in the map, returns NULL.
int*
UnorderedMapIntInt_fetch(UnorderedMapIntInt* m, int key)
{
  if (!*m)
	return NULL;

  std::unordered_map<int, int>::iterator it = (*m)->m.find(key);
  if (it == (*m)->m.end())
	return NULL;
  else
	return &(it->second);
}

// Returns a pointer to the value for key in the map described by m, creating the map first if m is NULL.  If key isn't in the map, inserts it first.
int*
UnorderedMapIntInt_set(UnorderedMapIntInt* m, int key)
{
  if (!*m)
	*m = new UnorderedMapIntInt_t;

  return &((*m)->m[key]);
}

// Destroys the map described by m.
void
UnorderedMapIntInt_delete(UnorderedMapIntInt* m)
{
  if (*m)
	{
	  delete *m;
	  *m = NULL;
	}
}
