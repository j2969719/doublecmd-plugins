#ifndef _COMMON_H
#define _COMMON_H

#ifdef __GNUC__

#include <stdint.h>

#if defined(__WIN32__) || defined(_WIN32) || defined(_WIN64)
  #define DCPCALL __attribute__((stdcall))
#else
  #define DCPCALL
#endif

#define MAX_PATH 260

#ifndef LONG
  typedef int32_t LONG;
#endif
#ifndef DWORD
  typedef uint32_t DWORD;
#endif
#ifndef WORD
  typedef uint16_t WORD;
#endif
typedef void *HANDLE;
#ifndef HICON
  typedef HANDLE HICON;
#endif
#ifndef HBITMAP
  typedef HANDLE HBITMAP;
#endif
#ifndef HWND
  typedef HANDLE HWND;
#endif
#ifndef BOOL
  typedef int BOOL;
#endif
#ifndef CHAR
  typedef char CHAR;
#endif
#ifndef WCHAR
  typedef uint16_t WCHAR;
#endif
#ifndef LPARAM
  typedef intptr_t LPARAM;
#endif
#ifndef WPARAM
  typedef uintptr_t WPARAM;
#endif


#pragma pack(push, 1)

typedef struct _RECT {
  LONG left;
  LONG top;
  LONG right;
  LONG bottom;
} RECT, *PRECT;

typedef struct _FILETIME {
	DWORD dwLowDateTime;
	DWORD dwHighDateTime;
} FILETIME,*PFILETIME,*LPFILETIME;

typedef struct _WIN32_FIND_DATAA {
	DWORD dwFileAttributes;
	FILETIME ftCreationTime;
	FILETIME ftLastAccessTime;
	FILETIME ftLastWriteTime;
	DWORD nFileSizeHigh;
	DWORD nFileSizeLow;
	DWORD dwReserved0;
	DWORD dwReserved1;
	CHAR cFileName[MAX_PATH];
	CHAR cAlternateFileName[14];
} WIN32_FIND_DATAA,*LPWIN32_FIND_DATAA;

typedef struct _WIN32_FIND_DATAW {
	DWORD dwFileAttributes;
	FILETIME ftCreationTime;
	FILETIME ftLastAccessTime;
	FILETIME ftLastWriteTime;
	DWORD nFileSizeHigh;
	DWORD nFileSizeLow;
	DWORD dwReserved0;
	DWORD dwReserved1;
	WCHAR cFileName[MAX_PATH];
	WCHAR cAlternateFileName[14];
} WIN32_FIND_DATAW,*LPWIN32_FIND_DATAW;

#pragma pack(pop)

#else

#if defined(_WIN32) || defined(_WIN64)
  #define DCPCALL __stdcall
#else
  #define DCPCALL __cdecl
#endif

#endif

#endif // _COMMON_H
