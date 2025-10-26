/* SPDX-License-Identifier: GPL-3.0 */

/*
printf "%s\n" "LIBRARY ntdll" EXPORTS memcpy wcscmp > keybind.def
dlltool -d keybind.def -l keybind.lib
gcc -std=c89 -Os -s -fno-asynchronous-unwind-tables -Wl,--gc-sections,--tsaware,--subsystem,windows -nostdlib -fno-builtin -o keybind.exe keybind.c -lkernel32 -luser32 -L. -lkeybind
rm keybind.def keybind.lib
*/
/*
> keybind.def (echo LIBRARY ntdll& echo EXPORTS& echo memcpy& echo wcscmp)
> stub.vbs echo Set M=CreateObject^(^"System.IO.MemoryStream^"^):M.SetLength 64:M.WriteByte 77:M.WriteByte 90:Set S=CreateObject^(^"ADODB.Stream^"^):S.Open:S.Type=1:S.Write M.ToArray^(^):S.SaveToFile WScript.ScriptFullName^&^"\..\stub.bin^",2:S.Close
cscript //nologo stub.vbs
lib /nologo /machine:amd64 /def:keybind.def /out:keybind.lib
cl /nologo /GS- /Gs1000000000 /O2 /Oi keybind.c /link kernel32.lib user32.lib keybind.lib /subsystem:windows /stack:0x200000,200000 /emittoolversioninfo:no /stub:stub.bin /emitpogophaseinfo /nocoffgrpinfo /manifest:no /merge:.edata=.rdata /merge:.pdata=.rdata /emitvolatilemetadata:no /ignore:4060 /safeseh:no
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

struct si
{
  int cb;
  iz a[3];
  int b[7];
  int flags;
  u16 c[2];
  iz d[4];
};

struct pi
{
  iz process;
  iz thread;
  int pid;
  int tid;
};

struct pe32
{
  int size;
  int a;
  int id;
  iz b;
  int c[5];
  u16 file[260];
};

W32(int) CloseHandle(iz);
W32(int) CreateProcessW(iz, u16*, iz, iz, int, int, iz, iz, struct si*, struct pi*);
W32(iz) CreateToolhelp32Snapshot(int, int);
NORETURN W32(void) ExitProcess(int);
W32(int) GetLastError(void);
W32(iz) OpenProcess(int, int, int);
W32(int) Process32FirstW(iz, struct pe32*);
W32(int) Process32NextW(iz, struct pe32*);
W32(int) RegisterHotKey(iz, int, int, int);
W32(void) Sleep(int);
W32(int) TerminateProcess(iz, int);
W32(int) UnregisterHotKey(iz, int);
NT(void*) memcpy(void*, void const*, iz);
NT(int) wcscmp(u16 const*, u16 const*);

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

static int terminate_process(int id)
{
  iz handle = OpenProcess(1, 0, id);
  int result;
  if (handle == 0) {
    return 1;
  }

  result = TerminateProcess(handle, 0) == 0;
  CloseHandle(handle);
  return result;
}

ALIGN(2) static u16 const explorer_exe[] = {101, 120, 112, 108, 111, 114, 101, 114, 46, 101, 120, 101, 0};

static int terminate_explorer(u8* memory)
{
  iz snap = CreateToolhelp32Snapshot(2, 0);
  struct pe32* pe;
  int result = 0;
  if (snap == -1) {
    return 1;
  }

  pe = (struct pe32*)memory;
  pe->size = sizeof(*pe);
  if (Process32FirstW(snap, pe) == 0) {
    result = GetLastError() != 18;
    goto exit;
  }

  do {
    if (wcscmp(pe->file, explorer_exe) == 0) {
      goto found;
    }

    if (Process32NextW(snap, pe) == 0) {
      result = GetLastError() != 18;
      break;
    }
  } while (1);

exit:
  CloseHandle(snap);
  return result;

found:
  CloseHandle(snap);
  return terminate_process(pe->id);
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

static int start_explorer(u8* memory)
{
  struct si si = {0};
  struct pi pi;
  int result;
  u16* cmdline = (u16*)memory;
  si.cb = sizeof(si);
  memcpy(cmdline, explorer_exe, sizeof(explorer_exe));
  result = CreateProcessW(0, cmdline, 0, 0, 0, 67108872, 0, 0, &si, &pi) == 0;
  CloseHandle(pi.process);
  CloseHandle(pi.thread);
  return result;
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
  int result;
  checked(terminate_explorer(memory));
  result = register_keybinds();
  if (result != 0) {
    return result + start_explorer(memory);
  }

  Sleep(4000);
  checked(start_explorer(memory));
  return unregister_keybinds();
}

#undef checked

void WinMainCRTStartup(void)
{
  static u8 memory[1 << 15];
  ExitProcess(try_main(memory));
}
