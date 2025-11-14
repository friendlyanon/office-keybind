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

#define N "\r\n"

typedef unsigned char u8;
typedef short i16;
typedef unsigned short u16;
typedef ptrdiff_t iz;
typedef size_t uz;

#define maxof(x) \
  ((x) - 1 < 1 ? (((x)1 << (sizeof(x) * 8 - 2)) - 1) * 2 + 1 : (x) - 1)
#define sizeof(x) ((iz)sizeof(x))
#define countof(x) (sizeof(x) / sizeof(*(x)))
#define lengthof(s) (countof(s) - 1)

#ifdef _MSC_VER
#  pragma intrinsic(memcpy)
#  define assert(c) \
    do { \
      if (!(c)) { \
        __debugbreak(); \
      } \
    } while (0)
#  ifndef ALIGNOF
#    define ALIGNOF __alignof
#  endif
#  define ALIGN(x) __declspec(align(x))
#  define NORETURN __declspec(noreturn)
#  define W32(ret) __declspec(dllimport) ret __stdcall
#  define NT(ret) __declspec(dllimport) ret __cdecl
/* clang-format off */
#  define STRING(name, str) \
    __pragma(warning(suppress : 4295)) \
    ALIGN(1) \
    static u8 const name[lengthof(str)] = str
#  define WSTRING(name, str) \
    __pragma(warning(suppress : 4045)) \
    __pragma(warning(suppress : 4295)) \
    __declspec(align(2)) \
    static u16 const name[lengthof(str)] = L"" str
/* clang-format on */
#else
#  define assert(c) \
    do { \
      if (__builtin_expect(!(c), 0)) { \
        __builtin_trap(); \
      } \
    } while (0)
#  ifndef ALIGNOF
#    define ALIGNOF __alignof__
#  endif
#  define ALIGN(x) __attribute__((__aligned__(x)))
#  define NORETURN __attribute__((__noreturn__))
#  define W32(ret) __attribute__((__dllimport__, __stdcall__)) ret
#  define NT(ret) __attribute__((__dllimport__, __cdecl__)) ret
/* clang-format off */
#  define STRING(name, str) \
    ALIGN(1) \
    __attribute__((__nonstring__)) \
    static u8 const name[lengthof(str)] = str
#  define WSTRING(name, str) \
    ALIGN(2) \
    static u16 const name[lengthof(str)] = L"" str
/* clang-format on */
#endif

#define new(memory, count, T) ((T*)alloc(memory, count, sizeof(T), ALIGNOF(T)))

NORETURN W32(void) ExitProcess(int);
W32(int) GetLastError(void);
W32(int) GetWindowsDirectoryW(u16*, int);
W32(int) MessageBoxW(iz, u16 const*, u16 const*, int);
W32(void) OutputDebugStringA(u8 const*);
W32(int) RegisterHotKey(iz, int, int, int);
W32(int) RmEndSession(int);
W32(int) RmRegisterResources(int, int, u16 const**, int, iz, int, iz);
W32(int) RmRestart(int, int, iz);
W32(int) RmShutdown(int, int, iz);
W32(int) RmStartSession(int*, int, u16*);
W32(void) Sleep(int);
W32(int) UnregisterHotKey(iz, int);
NT(void*) memcpy(void*, void const*, uz);

#ifdef _MSC_VER
#  define memcpy(a, b, c) ((void)memcpy((a), (b), iuz(c)))
#else
#  define memcpy(a, b, c) ((void)__builtin_memcpy((a), (b), iuz(c)))
#endif

static uz iuz(iz x)
{
  assert(x >= 0);
  return (uz)x;
}

static void* alloc(u8** memory, iz count, iz size, iz align)
{
  u8* mem = *memory;
  iz pad = (iz) - (uz)mem & (align - 1);
  u8* r = mem + pad;
  *memory = r + count * size;
  return r;
}

struct string
{
  u8 const* data;
  int size;
};

static struct string sA(u8 const* data, int size)
{
  struct string str;
  str.data = data;
  str.size = size;
  return str;
}

#define S(x) (sA((x), countof(x)))

#define buf8_members \
  u8* data; \
  int size; \
  int capacity; \
  int error

struct buf8
{
  buf8_members;
};

static void buf8cat(struct buf8* buf, struct string s)
{
  int available = buf->capacity - buf->size;
  int count = s.size < available ? s.size : available;
  memcpy(buf->data + buf->size, s.data, count);
  buf->size += count;
  buf->error |= count < s.size;
}

static void buf8c8(struct buf8* buf, u8 c)
{
  buf8cat(buf, sA(&c, 1));
}

static void buf8int(struct buf8* buf, int x)
{
  u8 b[11];
  int sign = x < 0;
  u8* const end = b + countof(b);
  u8* p = end;
  do {
    u8 tmp = (u8)(x % 10);
    x /= 10;
    *--p = 48 + (u8)(sign ? -tmp : tmp);
  } while (x != 0);
  if (sign) {
    *--p = 45;
  }
  buf8cat(buf, sA(p, (int)(end - p)));
}

struct err
{
  buf8_members;
  int ok;
};

#define as_buf8(x) ((struct buf8*)(x))

STRING(crlf, N);
STRING(check_frag1, "error: '");
STRING(check_frag2, "' failed (");
STRING(check_frag3, ": ");
STRING(check_frag4, ")" N "  line ");

static void trace_(struct buf8* b, struct string op, int line)
{
  STRING(frag, "' failed" N "  line ");
  if (b->size != 0) {
    buf8cat(b, sA(frag + (countof(frag) - 7), 7));
  } else {
    buf8cat(b, S(check_frag1));
    buf8cat(b, op);
    buf8cat(b, S(frag));
  }
  buf8int(b, line);
  buf8cat(b, S(crlf));
}

#define trace_impl(into, x, y, ret) \
  do { \
    into = (x); \
    if (into != 0) { \
      STRING(op, y); \
      trace_(as_buf8(err), S(op), __LINE__); \
      err->ok = 0; \
      ret; \
    } \
  } while (0)

#define EMPTY
#define trace_r(into, x) trace_impl(into, x, #x, return into)
#define trace(into, x) trace_impl(into, x, #x, EMPTY)

static void check_le_(struct buf8* b, struct string op, int line)
{
  STRING(frag, "GetLastError");
  buf8cat(b, S(check_frag1));
  buf8cat(b, op);
  buf8cat(b, S(check_frag2));
  buf8cat(b, S(frag));
  buf8cat(b, S(check_frag3));
  buf8int(b, GetLastError());
  buf8cat(b, S(check_frag4));
  buf8int(b, line);
  buf8cat(b, S(crlf));
}

#define check_le_impl(into, x, y, cmp, ret) \
  do { \
    into = (x) cmp; \
    if (into != 0) { \
      STRING(op, y); \
      check_le_(as_buf8(err), S(op), __LINE__); \
      err->ok = 0; \
      ret; \
    } \
  } while (0)

#define check_le_r(into, x, cmp) check_le_impl(into, x, #x, cmp, return into)
#define check_le(into, x, cmp) check_le_impl(into, x, #x, cmp, EMPTY)

static void check_(struct buf8* b, struct string op, int line, int result)
{
  STRING(frag, "return");
  buf8cat(b, S(check_frag1));
  buf8cat(b, op);
  buf8cat(b, S(check_frag2));
  buf8cat(b, S(frag));
  buf8cat(b, S(check_frag3));
  buf8int(b, result);
  buf8cat(b, S(check_frag4));
  buf8int(b, line);
  buf8cat(b, S(crlf));
}

#define check_impl(into, x, y, cmp, ret) \
  do { \
    int check_result_ = (x); \
    into = check_result_ cmp; \
    if (into != 0) { \
      STRING(op, y); \
      check_(as_buf8(err), S(op), __LINE__, check_result_); \
      err->ok = 0; \
      ret; \
    } \
  } while (0)

#define check_r(into, x, cmp) check_impl(into, x, #x, cmp, return into)
#define check(into, x, cmp) check_impl(into, x, #x, cmp, EMPTY)
#define check2(into, x) check_impl(into, x, #x, EMPTY, EMPTY)

WSTRING(explorer_exe, "\\explorer.exe\0");

static int start_session(struct err* err, int* session, u8* scratch)
{
  u16* explorer = new (&scratch, maxof(i16), u16);
  u16 session_key[33];
  u16 const* resources[1];
  int result;
  int copied;
  check_le_r(result, copied = GetWindowsDirectoryW(explorer, maxof(i16)), == 0);

  memcpy(explorer + copied, explorer_exe, sizeof(explorer_exe));

  check_r(result, RmStartSession(session, 0, session_key), != 0);

  resources[0] = explorer;
  check(result, RmRegisterResources(*session, 1, resources, 0, 0, 0, 0), != 0);
  if (result) {
    goto exit;
  }

  check(result, RmShutdown(*session, 1, 0), != 0);
  if (result) {
    goto exit;
  }

  return 0;

exit:
  check(result, RmEndSession(*session), != 0);
  return result + 1;
}

static int const keys[] = {0, 32, 68, 76, 78, 79, 80, 84, 87, 88, 89};

static int register_keybinds(struct err* err)
{
  int i = 0;
  int limit = countof(keys);
  for (; i != limit; ++i) {
    int result;
    check_le_r(result, RegisterHotKey(0, i, 16399, keys[i]), == 0);
  }

  return 0;
}

static int restart_explorer(struct err* err, int session)
{
  int result;
  int result2;
  check2(result, RmRestart(session, 0, 0));
  check(result2, RmEndSession(session), != 0);
  return result + result2;
}

static int unregister_keybinds(struct err* err)
{
  int i = 0;
  int limit = countof(keys);
  for (; i != limit; ++i) {
    int result;
    check_le_r(result, UnregisterHotKey(0, i), == 0);
  }

  return 0;
}

static int try_main(struct err* err, u8* memory)
{
  int session;
  int result;
  trace_r(result, start_session(err, &session, memory));
  trace(result, register_keybinds(err));
  if (result) {
    int result2;
    trace(result2, restart_explorer(err, session));
    return result + result2;
  }

  trace_r(result, restart_explorer(err, session));
  Sleep(5000);
  trace_r(result, unregister_keybinds(err));
  return 0;
}

void WinMainCRTStartup(void)
{
  ALIGN(2) static u8 memory[1 << 21];
  u8* memory_ptr = memory;
  struct err err = {0};
  int code;
  err.ok = 1;
  err.data = new (&memory_ptr, 65535, u8);
  err.capacity = 65535;
  code = try_main(&err, memory_ptr);
  if (code != 0 || err.ok == 0 || err.size != 0) {
    WSTRING(text, "An error occured. Attach a debugger for a stacktrace.\0");
    WSTRING(caption, "keybind\0");
    int result = MessageBoxW(0, text, caption, 273);
    if (result != 0) {
      if (result == 1) {
        buf8c8(as_buf8(&err), 0);
        if (err.error == 0) {
          OutputDebugStringA(err.data);
        } else {
          STRING(msg, "The error buffer is in an error state.\0");
          OutputDebugStringA(msg);
        }
      }
    } else {
      ++code;
    }
  }

  ExitProcess(code);
}
