/* SPDX-License-Identifier: GPL-3.0 */

/*
printf "%s\n" "LIBRARY ntdll" EXPORTS memcpy > keybind.def
dlltool -d keybind.def -l keybind.lib
gcc -std=c89 -Os -s -fno-asynchronous-unwind-tables -Wl,--gc-sections,--tsaware,--subsystem,windows -nostdlib -fno-builtin -o keybind.exe keybind.c -lkernel32 -luser32 -lrstrtmgr -L. -lkeybind
rm keybind.def keybind.lib
*/
/*
> keybind.def (echo LIBRARY ntdll& echo EXPORTS& echo memcpy)
> stub.vbs echo Set M=CreateObject^(^"System.IO.MemoryStream^"^):M.SetLength 64:M.WriteByte 77:M.WriteByte 90:Set S=CreateObject^(^"ADODB.Stream^"^):S.Open:S.Type=1:S.Write M.ToArray^(^):S.SaveToFile WScript.ScriptFullName^&^"\..\stub.bin^",2:S.Close
cscript //nologo stub.vbs
lib /nologo /machine:amd64 /def:keybind.def /out:keybind.lib
cl /nologo /GS- /Gs1000000000 /O2 /Oi keybind.c /link kernel32.lib user32.lib rstrtmgr.lib keybind.lib /subsystem:windows /stack:0x200000,200000 /emittoolversioninfo:no /stub:stub.bin /emitpogophaseinfo /nocoffgrpinfo /manifest:no /merge:.edata=.rdata /merge:.pdata=.rdata /emitvolatilemetadata:no /ignore:4060 /safeseh:no
del keybind.def stub.vbs stub.bin keybind.exp keybind.lib keybind.obj
*/

#include <stddef.h>

typedef unsigned char u8;
typedef unsigned short u16;
typedef ptrdiff_t iz;
typedef size_t uz;

#define sizeof(x) ((iz)sizeof(x))
#define countof(x) (sizeof(x) / sizeof(*(x)))

#ifdef _MSC_VER
#  pragma intrinsic(memcpy)
#  define ALIGN(x) __declspec(align(x))
#  define NORETURN __declspec(noreturn)
#  define W32(ret) __declspec(dllimport) ret __stdcall
#  define NT(ret) __declspec(dllimport) ret __cdecl
#else
#  define assert(c) \
    do { \
      if (__builtin_expect(!(c), 0)) { \
        __builtin_trap(); \
      } \
    } while (0)
#  define ALIGN(x) __attribute__((__aligned__(x)))
#  define NORETURN __attribute__((__noreturn__))
#  define W32(ret) __attribute__((__dllimport__, __stdcall__)) ret
#  define NT(ret) __attribute__((__dllimport__, __cdecl__)) ret
#endif

NORETURN W32(void) ExitProcess(int);
W32(int) GetWindowsDirectoryW(u16*, int);
W32(int) RegisterHotKey(iz, int, int, int);
W32(int) RmEndSession(int);
W32(int) RmRegisterResources(int, int, u16 const**, int, iz, int, iz);
W32(int) RmRestart(int, int, iz);
W32(int) RmShutdown(int, int, iz);
W32(int) RmStartSession(int*, int, u16*);
W32(void) Sleep(int);
W32(int) UnregisterHotKey(iz, int);
NT(void*) memcpy(void*, void const*, iz);

#ifdef _MSC_VER
#  define memcpy(a, b, c) ((void)memcpy((a), (b), (c)))
#else
#  define memcpy(a, b, c) ((void)__builtin_memcpy((a), (b), iuz(c)))

static uz iuz(iz x)
{
  assert(x >= 0);
  return (uz)x;
}
#endif

ALIGN(2) static u16 const explorer_exe[] = {92, 101, 120, 112, 108, 111, 114, 101, 114, 46, 101, 120, 101, 0};

static int start_session(int* session, u8* memory)
{
  u16* explorer = (u16*)memory;
  int copied = GetWindowsDirectoryW(explorer, 1 << 15);
  u16 session_key[33];
  u16 const* resources[] = {explorer};
  if (copied == 0) {
    return 1;
  }

  memcpy(explorer + copied, explorer_exe, sizeof(explorer_exe));

  if (RmStartSession(session, 0, session_key) != 0) {
    return 1;
  }

  if (RmRegisterResources(*session, 1, resources, 0, 0, 0, 0) != 0) {
    goto exit;
  }

  if (RmShutdown(*session, 1, 0) != 0) {
    goto exit;
  }

  return 0;

exit:
  return 1 + RmEndSession(*session) != 0;
}

static int const keys[] = {0, 32, 68, 76, 78, 79, 80, 84, 87, 88, 89};

static int register_keybinds(void)
{
  int i = 0;
  int limit = countof(keys);
  for (; i != limit; ++i) {
    if (RegisterHotKey(0, i, 16399, keys[i]) == 0) {
      return 1;
    }
  }

  return 0;
}

static int restart_explorer(int session)
{
  int result = RmRestart(session, 0, 0);
  return result + RmEndSession(session) != 0;
}

static int unregister_keybinds(void)
{
  int i = 0;
  int limit = countof(keys);
  for (; i != limit; ++i) {
    if (UnregisterHotKey(0, i) == 0) {
      return 1;
    }
  }

  return 0;
}

#define checked(x) \
  do { \
    result = (x); \
    if (result != 0) { \
      return result; \
    } \
  } while (0)

static int try_main(u8* memory)
{
  int session;
  int result;
  checked(start_session(&session, memory));
  result = register_keybinds();
  if (result != 0) {
    return result + restart_explorer(session);
  }

  checked(restart_explorer(session));
  Sleep(5000);
  return unregister_keybinds();
}

#undef checked

void WinMainCRTStartup(void)
{
  ALIGN(2) static u8 memory[1 << 15];
  ExitProcess(try_main(memory));
}
