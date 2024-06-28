#include "conptyprocess.h"
#include <QFile>
#include <QFileInfo>
#include <QThread>
#include <sstream>
#include <QTimer>
#include <QMutexLocker>
#include <QCoreApplication>
#include <QWinEventNotifier>

#include <qt_windows.h>

#ifdef QTCREATOR_PCH_H
#ifndef IN
#define IN
#endif
#ifndef OUT
#define OUT
#endif
#endif

#include <winternl.h>

#define READ_INTERVAL_MSEC 500

// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#define FILE_SYNCHRONOUS_IO_NONALERT 0x00000020

////////////////////////////////////////////////////////////////////////////////////////
/// Ported from wil/result_macros.h

#define FAILED_NTSTATUS(status) (((NTSTATUS)(status)) < 0)
#define SUCCEEDED_NTSTATUS(status) (((NTSTATUS)(status)) >= 0)

#define RETURN_IF_NTSTATUS_FAILED(call) if(FAILED_NTSTATUS(call)) return E_FAIL;
#define RETURN_IF_WIN32_BOOL_FALSE(call) if((call) == FALSE) return E_FAIL;
#define RETURN_IF_NULL_ALLOC(call) if((call) == nullptr) return E_OUTOFMEMORY;
#define RETURN_IF_FAILED(call) {HRESULT hr = (call); if(hr != S_OK)return hr; }

//! Set zero or more bitflags specified by `flags` in the variable `var`.
#define WI_SetAllFlags(var, flags) ((var) |= (flags))
//! Set a single compile-time constant `flag` in the variable `var`.
#define WI_SetFlag(var, flag) WI_SetAllFlags(var,flag)

////////////////////////////////////////////////////////////////////////////////////////
/// Ported from wil
class unique_hmodule : public std::unique_ptr<std::remove_pointer_t<HMODULE>, decltype(&FreeLibrary)>
{
public:
  unique_hmodule() : std::unique_ptr<std::remove_pointer_t<HMODULE>, decltype(&FreeLibrary)>(nullptr, FreeLibrary) {}
  unique_hmodule(HMODULE module) : std::unique_ptr<std::remove_pointer_t<HMODULE>, decltype(&FreeLibrary)>(module, FreeLibrary) {}
};

class unique_handle : public std::unique_ptr<std::remove_pointer_t<HANDLE>, decltype(&CloseHandle)>
{
public:
  unique_handle() : std::unique_ptr<std::remove_pointer_t<HANDLE>, decltype(&CloseHandle)>(nullptr, CloseHandle) {}
  unique_handle(HANDLE module) : std::unique_ptr<std::remove_pointer_t<HANDLE>, decltype(&CloseHandle)>(module, CloseHandle) {}


  class AddressOf {
  public:
    AddressOf(unique_handle &h) : m_h(h) {}
    ~AddressOf() {
      m_h.reset(m_dest);
    }

    operator PHANDLE() {return &m_dest;}

    HANDLE m_dest{INVALID_HANDLE_VALUE};
    unique_handle &m_h;
  };

  AddressOf addressof() {
    return AddressOf(*this);
  }
};

class unique_process_information : public PROCESS_INFORMATION
{
public:
  unique_process_information() {
   hProcess = 0;
   hThread = 0;
   dwProcessId = 0;
   dwThreadId = 0;
  }
  ~unique_process_information() {
    if (hProcess)
    {
        CloseHandle(hProcess);
    }

    if (hThread)
    {
        CloseHandle(hThread);
    }
  }

  PROCESS_INFORMATION* addressof() {
    return this;
  }
};

template <typename TLambda>
class on_scope_exit {
public:
  TLambda m_func;
  bool m_call{true};
  on_scope_exit(TLambda &&func) : m_func(std::move(func)) {}
  ~on_scope_exit() {if(m_call)m_func();}

  void release() {m_call = false;}
};

template <typename TLambda>
[[nodiscard]] inline auto scope_exit(TLambda&& lambda) noexcept
{
    return on_scope_exit<TLambda>(std::forward<TLambda>(lambda));
}

////////////////////////////////////////////////////////////////////////////////////////



class WinNTControl
{
public:
    [[nodiscard]] static NTSTATUS NtOpenFile(_Out_ PHANDLE FileHandle,
                                             _In_ ACCESS_MASK DesiredAccess,
                                             _In_ POBJECT_ATTRIBUTES ObjectAttributes,
                                             _Out_ PIO_STATUS_BLOCK IoStatusBlock,
                                             _In_ ULONG ShareAccess,
                                             _In_ ULONG OpenOptions);

private:
    WinNTControl();

    WinNTControl(WinNTControl const&) = delete;
    void operator=(WinNTControl const&) = delete;

    static WinNTControl& GetInstance();

    unique_hmodule const _NtDllDll;

    typedef NTSTATUS(NTAPI* PfnNtOpenFile)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, PIO_STATUS_BLOCK, ULONG, ULONG);
    PfnNtOpenFile const _NtOpenFile;
};

WinNTControl::WinNTControl() :
    // NOTE: Use LoadLibraryExW with LOAD_LIBRARY_SEARCH_SYSTEM32 flag below to avoid unneeded directory traversal.
    //       This has triggered CPG boot IO warnings in the past.
    _NtDllDll(/*THROW_LAST_ERROR_IF_NULL*/(LoadLibraryExW(L"ntdll.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32))),
    _NtOpenFile(reinterpret_cast<PfnNtOpenFile>(/*THROW_LAST_ERROR_IF_NULL*/(GetProcAddress(_NtDllDll.get(), "NtOpenFile"))))
{
}

// Routine Description:
// - Provides the singleton pattern for WinNT control. Stores the single instance and returns it.
// Arguments:
// - <none>
// Return Value:
// - Reference to the single instance of NTDLL.dll wrapped methods.
WinNTControl& WinNTControl::GetInstance()
{
    static WinNTControl Instance;
    return Instance;
}

// Routine Description:
// - Provides access to the NtOpenFile method documented at:
//   https://msdn.microsoft.com/en-us/library/bb432381(v=vs.85).aspx
// Arguments:
// - See definitions at MSDN
// Return Value:
// - See definitions at MSDN
[[nodiscard]] NTSTATUS WinNTControl::NtOpenFile(_Out_ PHANDLE FileHandle,
                                                _In_ ACCESS_MASK DesiredAccess,
                                                _In_ POBJECT_ATTRIBUTES ObjectAttributes,
                                                _Out_ PIO_STATUS_BLOCK IoStatusBlock,
                                                _In_ ULONG ShareAccess,
                                                _In_ ULONG OpenOptions)
{
  return GetInstance()._NtOpenFile(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, ShareAccess, OpenOptions);
}

namespace DeviceHandle {

/*++
Routine Description:
- This routine opens a handle to the console driver.

Arguments:
- Handle - Receives the handle.
- DeviceName - Supplies the name to be used to open the console driver.
- DesiredAccess - Supplies the desired access mask.
- Parent - Optionally supplies the parent object.
- Inheritable - Supplies a boolean indicating if the new handle is to be made inheritable.
- OpenOptions - Supplies the open options to be passed to NtOpenFile. A common
                option for clients is FILE_SYNCHRONOUS_IO_NONALERT, to make the handle
                synchronous.

Return Value:
- NTSTATUS indicating if the handle was successfully created.
--*/
[[nodiscard]] NTSTATUS
_CreateHandle(
    _Out_ PHANDLE Handle,
    _In_ PCWSTR DeviceName,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ HANDLE Parent,
    _In_ BOOLEAN Inheritable,
    _In_ ULONG OpenOptions)

{
    ULONG Flags = OBJ_CASE_INSENSITIVE;

    if (Inheritable)
    {
        WI_SetFlag(Flags, OBJ_INHERIT);
    }

    UNICODE_STRING Name;
#pragma warning(suppress : 26492) // const_cast is prohibited, but we can't avoid it for filling UNICODE_STRING.
    Name.Buffer = const_cast<wchar_t*>(DeviceName);
    //Name.Length = gsl::narrow_cast<USHORT>((wcslen(DeviceName) * sizeof(wchar_t)));
    Name.Length = static_cast<unsigned short>((wcslen(DeviceName) * sizeof(wchar_t)));
    Name.MaximumLength = Name.Length + sizeof(wchar_t);

    OBJECT_ATTRIBUTES ObjectAttributes;
#pragma warning(suppress : 26477) // The QOS part of this macro in the define is 0. Can't fix that.
    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               Flags,
                               Parent,
                               nullptr);

    IO_STATUS_BLOCK IoStatus;
    return WinNTControl::NtOpenFile(Handle,
                                    DesiredAccess,
                                    &ObjectAttributes,
                                    &IoStatus,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                    OpenOptions);
}

/*++
Routine Description:
- This routine creates a handle to an input or output client of the given
  server. No control io is sent to the server as this request must be coming
  from the server itself.

Arguments:
- Handle - Receives a handle to the new client.
- ServerHandle - Supplies a handle to the server to which to attach the
                 newly created client.
- Name - Supplies the name of the client object.
- Inheritable - Supplies a flag indicating if the handle must be inheritable.

Return Value:
- NTSTATUS indicating if the client was successfully created.
--*/
[[nodiscard]] NTSTATUS
CreateClientHandle(
    _Out_ PHANDLE Handle,
    _In_ HANDLE ServerHandle,
    _In_ PCWSTR Name,
    _In_ BOOLEAN Inheritable)
{
    return _CreateHandle(Handle,
                         Name,
                         GENERIC_WRITE | GENERIC_READ | SYNCHRONIZE,
                         ServerHandle,
                         Inheritable,
                         FILE_SYNCHRONOUS_IO_NONALERT);
}

/*++
Routine Description:
- This routine creates a new server on the driver and returns a handle to it.

Arguments:
- Handle - Receives a handle to the new server.
- Inheritable - Supplies a flag indicating if the handle must be inheritable.

Return Value:
- NTSTATUS indicating if the console was successfully created.
--*/
[[nodiscard]] NTSTATUS
CreateServerHandle(
    _Out_ PHANDLE Handle,
    _In_ BOOLEAN Inheritable)
{
    return _CreateHandle(Handle,
                         L"\\Device\\ConDrv\\Server",
                         GENERIC_ALL,
                         nullptr,
                         Inheritable,
                         0);
}


} // namespace DeviceHandle

[[nodiscard]] static inline NTSTATUS CreateClientHandle(PHANDLE Handle, HANDLE ServerHandle, PCWSTR Name, BOOLEAN Inheritable)
{
    return DeviceHandle::CreateClientHandle(Handle, ServerHandle, Name, Inheritable);
}

[[nodiscard]] static inline NTSTATUS CreateServerHandle(PHANDLE Handle, BOOLEAN Inheritable)
{
    return DeviceHandle::CreateServerHandle(Handle, Inheritable);
}

typedef struct _PseudoConsole
{
    HANDLE hSignal;
    HANDLE hPtyReference;
    HANDLE hConPtyProcess;
} PseudoConsole;

// Signals
// These are not defined publicly, but are used for controlling the conpty via
//      the signal pipe.
#define PTY_SIGNAL_CLEAR_WINDOW (2u)
#define PTY_SIGNAL_RESIZE_WINDOW (8u)

// CreatePseudoConsole Flags
// The other flag (PSEUDOCONSOLE_INHERIT_CURSOR) is actually defined in consoleapi.h in the OS repo
#ifndef PSEUDOCONSOLE_INHERIT_CURSOR
#define PSEUDOCONSOLE_INHERIT_CURSOR (0x1)
#endif
#define PSEUDOCONSOLE_RESIZE_QUIRK (0x2)
#define PSEUDOCONSOLE_WIN32_INPUT_MODE (0x4)

static QString qSystemDirectory()
{
    static const QString result = []() -> QString {
        QVarLengthArray<wchar_t, MAX_PATH> fullPath = {};
        UINT retLen = ::GetSystemDirectoryW(fullPath.data(), MAX_PATH);
        if (retLen > MAX_PATH) {
            fullPath.resize(retLen);
            retLen = ::GetSystemDirectoryW(fullPath.data(), retLen);
        }
        // in some rare cases retLen might be 0
        return QString::fromWCharArray(fullPath.constData(), int(retLen));
    }();
    return result;
}

// Function Description:
// - Returns the path to conhost.exe as a process heap string.
static QString _InboxConsoleHostPath()
{
    return QString("\\\\?\\%1\\conhost.exe").arg(qSystemDirectory());
}

// Function Description:
// - Returns the path to either conhost.exe or the side-by-side OpenConsole, depending on whether this
//   module is building with Windows and OpenConsole could be found.
// Return Value:
// - A pointer to permanent storage containing the path to the console host.
static const wchar_t* _ConsoleHostPath()
{
    // Use the magic of magic statics to only calculate this once.
    static QString consoleHostPath = _InboxConsoleHostPath();
    return reinterpret_cast<const wchar_t*>(consoleHostPath.utf16());
}

static bool _HandleIsValid(HANDLE h) noexcept
{
    return (h != INVALID_HANDLE_VALUE) && (h != nullptr);
}



HRESULT _CreatePseudoConsole(const HANDLE hToken,
                             const COORD size,
                             const HANDLE hInput,
                             const HANDLE hOutput,
                             const DWORD dwFlags,
                             _Inout_ PseudoConsole* pPty)
{
    if (pPty == nullptr)
    {
        return E_INVALIDARG;
    }
    if (size.X == 0 || size.Y == 0)
    {
        return E_INVALIDARG;
    }

    unique_handle serverHandle;
    RETURN_IF_NTSTATUS_FAILED(CreateServerHandle(serverHandle.addressof(), TRUE));

    unique_handle signalPipeConhostSide;
    unique_handle signalPipeOurSide;

    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(sa);
    // Mark inheritable for signal handle when creating. It'll have the same value on the other side.
    sa.bInheritHandle = FALSE;
    sa.lpSecurityDescriptor = nullptr;

    RETURN_IF_WIN32_BOOL_FALSE(CreatePipe(signalPipeConhostSide.addressof(), signalPipeOurSide.addressof(), &sa, 0));
    RETURN_IF_WIN32_BOOL_FALSE(SetHandleInformation(signalPipeConhostSide.get(), HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT));

    // GH4061: Ensure that the path to executable in the format is escaped so C:\Program.exe cannot collide with C:\Program Files
    const wchar_t* pwszFormat = L"\"%s\" --headless %s%s%s--width %hu --height %hu --signal 0x%x --server 0x%x";
    // This is plenty of space to hold the formatted string
    wchar_t cmd[MAX_PATH]{};
    const BOOL bInheritCursor = (dwFlags & PSEUDOCONSOLE_INHERIT_CURSOR) == PSEUDOCONSOLE_INHERIT_CURSOR;
    const BOOL bResizeQuirk = (dwFlags & PSEUDOCONSOLE_RESIZE_QUIRK) == PSEUDOCONSOLE_RESIZE_QUIRK;
    const BOOL bWin32InputMode = (dwFlags & PSEUDOCONSOLE_WIN32_INPUT_MODE) == PSEUDOCONSOLE_WIN32_INPUT_MODE;
    swprintf_s(cmd,
               MAX_PATH,
               pwszFormat,
               _ConsoleHostPath(),
               bInheritCursor ? L"--inheritcursor " : L"",
               bWin32InputMode ? L"--win32input " : L"",
               bResizeQuirk ? L"--resizeQuirk " : L"",
               size.X,
               size.Y,
               signalPipeConhostSide.get(),
               serverHandle.get());

    STARTUPINFOEXW siEx{ 0 };
    siEx.StartupInfo.cb = sizeof(STARTUPINFOEXW);
    siEx.StartupInfo.hStdInput = hInput;
    siEx.StartupInfo.hStdOutput = hOutput;
    siEx.StartupInfo.hStdError = hOutput;
    siEx.StartupInfo.dwFlags |= STARTF_USESTDHANDLES;

    // Only pass the handles we actually want the conhost to know about to it:
    const size_t INHERITED_HANDLES_COUNT = 4;
    HANDLE inheritedHandles[INHERITED_HANDLES_COUNT];
    inheritedHandles[0] = serverHandle.get();
    inheritedHandles[1] = hInput;
    inheritedHandles[2] = hOutput;
    inheritedHandles[3] = signalPipeConhostSide.get();

    // Get the size of the attribute list. We need one attribute, the handle list.
    SIZE_T listSize = 0;
    InitializeProcThreadAttributeList(nullptr, 1, 0, &listSize);

    // I have to use a HeapAlloc here because kernelbase can't link new[] or delete[]
    PPROC_THREAD_ATTRIBUTE_LIST attrList = static_cast<PPROC_THREAD_ATTRIBUTE_LIST>(HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, listSize));
    RETURN_IF_NULL_ALLOC(attrList);
    auto attrListDelete = scope_exit([&]() noexcept {
        HeapFree(GetProcessHeap(), 0, attrList);
    });

    siEx.lpAttributeList = attrList;
    RETURN_IF_WIN32_BOOL_FALSE(InitializeProcThreadAttributeList(siEx.lpAttributeList, 1, 0, &listSize));
    // Set cleanup data for ProcThreadAttributeList when successful.
    auto cleanupProcThreadAttribute = scope_exit([&]() noexcept {
        DeleteProcThreadAttributeList(siEx.lpAttributeList);
    });
    RETURN_IF_WIN32_BOOL_FALSE(UpdateProcThreadAttribute(siEx.lpAttributeList,
                                                         0,
                                                         PROC_THREAD_ATTRIBUTE_HANDLE_LIST,
                                                         inheritedHandles,
                                                         (INHERITED_HANDLES_COUNT * sizeof(HANDLE)),
                                                         nullptr,
                                                         nullptr));
    unique_process_information pi;
    { // wow64 disabled filesystem redirection scope
        if (hToken == INVALID_HANDLE_VALUE || hToken == nullptr)
        {
            // Call create process
            RETURN_IF_WIN32_BOOL_FALSE(CreateProcessW(_ConsoleHostPath(),
                                                      cmd,
                                                      nullptr,
                                                      nullptr,
                                                      TRUE,
                                                      EXTENDED_STARTUPINFO_PRESENT,
                                                      nullptr,
                                                      nullptr,
                                                      &siEx.StartupInfo,
                                                      pi.addressof()));
        }
        else
        {
            // Call create process
            RETURN_IF_WIN32_BOOL_FALSE(CreateProcessAsUserW(hToken,
                                                            _ConsoleHostPath(),
                                                            cmd,
                                                            nullptr,
                                                            nullptr,
                                                            TRUE,
                                                            EXTENDED_STARTUPINFO_PRESENT,
                                                            nullptr,
                                                            nullptr,
                                                            &siEx.StartupInfo,
                                                            pi.addressof()));
        }
    }

    // Move the process handle out of the PROCESS_INFORMATION into out Pseudoconsole
    pPty->hConPtyProcess = pi.hProcess;
    pi.hProcess = nullptr;

    RETURN_IF_NTSTATUS_FAILED(CreateClientHandle(&pPty->hPtyReference,
                                                 serverHandle.get(),
                                                 L"\\Reference",
                                                 FALSE));

    pPty->hSignal = signalPipeOurSide.release();

    return S_OK;
}

// Function Description:
// - Resizes the conpty
// Arguments:
// - hSignal: A signal pipe as returned by CreateConPty.
// - size: The new dimensions of the conpty, in characters.
// Return Value:
// - S_OK if the call succeeded, else an appropriate HRESULT for failing to
//      write the resize message to the pty.
HRESULT _ResizePseudoConsole(_In_ const PseudoConsole* const pPty, _In_ const COORD size)
{
    if (pPty == nullptr || size.X < 0 || size.Y < 0)
    {
        return E_INVALIDARG;
    }

    unsigned short signalPacket[3];
    signalPacket[0] = PTY_SIGNAL_RESIZE_WINDOW;
    signalPacket[1] = size.X;
    signalPacket[2] = size.Y;

    const BOOL fSuccess = WriteFile(pPty->hSignal, signalPacket, sizeof(signalPacket), nullptr, nullptr);
    return fSuccess ? S_OK : HRESULT_FROM_WIN32(GetLastError());
}

// Function Description:
// - Clears the conpty
// Arguments:
// - hSignal: A signal pipe as returned by CreateConPty.
// Return Value:
// - S_OK if the call succeeded, else an appropriate HRESULT for failing to
//      write the clear message to the pty.
HRESULT _ClearPseudoConsole(_In_ const PseudoConsole* const pPty)
{
    if (pPty == nullptr)
    {
        return E_INVALIDARG;
    }

    unsigned short signalPacket[1];
    signalPacket[0] = PTY_SIGNAL_CLEAR_WINDOW;

    const BOOL fSuccess = WriteFile(pPty->hSignal, signalPacket, sizeof(signalPacket), nullptr, nullptr);
    return fSuccess ? S_OK : HRESULT_FROM_WIN32(GetLastError());
}

// Function Description:
// - This closes each of the members of a PseudoConsole. It does not free the
//      data associated with the PseudoConsole. This is helpful for testing,
//      where we might stack allocate a PseudoConsole (instead of getting a
//      HPCON via the API).
// Arguments:
// - pPty: A pointer to a PseudoConsole struct.
// Return Value:
// - <none>
void _ClosePseudoConsoleMembers(_In_ PseudoConsole* pPty)
{
    if (pPty != nullptr)
    {
        // See MSFT:19918626
        // First break the signal pipe - this will trigger conhost to tear itself down
        if (_HandleIsValid(pPty->hSignal))
        {
            CloseHandle(pPty->hSignal);
            pPty->hSignal = nullptr;
        }
        // Then, wait on the conhost process before killing it.
        // We do this to make sure the conhost finishes flushing any output it
        //      has yet to send before we hard kill it.
        if (_HandleIsValid(pPty->hConPtyProcess))
        {
            // If the conhost is already dead, then that's fine. Presumably
            //      it's finished flushing it's output already.
            DWORD dwExit = 0;
            // If GetExitCodeProcess failed, it's likely conhost is already dead
            //      If so, skip waiting regardless of whatever error
            //      GetExitCodeProcess returned.
            //      We'll just go straight to killing conhost.
            if (GetExitCodeProcess(pPty->hConPtyProcess, &dwExit) && dwExit == STILL_ACTIVE)
            {
                WaitForSingleObject(pPty->hConPtyProcess, INFINITE);
            }

            TerminateProcess(pPty->hConPtyProcess, 0);
            CloseHandle(pPty->hConPtyProcess);
            pPty->hConPtyProcess = nullptr;
        }
        // Then take care of the reference handle.
        // TODO GH#1810: Closing the reference handle late leaves conhost thinking
        // that we have an outstanding connected client.
        if (_HandleIsValid(pPty->hPtyReference))
        {
            CloseHandle(pPty->hPtyReference);
            pPty->hPtyReference = nullptr;
        }
    }
}

// Function Description:
// - This closes each of the members of a PseudoConsole, and HeapFree's the
//      memory allocated to it. This should be used to cleanup any
//      PseudoConsoles that were created with CreatePseudoConsole.
// Arguments:
// - pPty: A pointer to a PseudoConsole struct.
// Return Value:
// - <none>
VOID _ClosePseudoConsole(_In_ PseudoConsole* pPty)
{
    if (pPty != nullptr)
    {
        _ClosePseudoConsoleMembers(pPty);
        HeapFree(GetProcessHeap(), 0, pPty);
    }
}

extern "C" HRESULT ConptyCreatePseudoConsoleAsUser(_In_ HANDLE hToken,
                                                   _In_ COORD size,
                                                   _In_ HANDLE hInput,
                                                   _In_ HANDLE hOutput,
                                                   _In_ DWORD dwFlags,
                                                   _Out_ HPCON* phPC)
{
    if (phPC == nullptr)
    {
        return E_INVALIDARG;
    }
    *phPC = nullptr;
    if ((!_HandleIsValid(hInput)) && (!_HandleIsValid(hOutput)))
    {
        return E_INVALIDARG;
    }

    PseudoConsole* pPty = (PseudoConsole*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PseudoConsole));
    RETURN_IF_NULL_ALLOC(pPty);
    auto cleanupPty = scope_exit([&]() noexcept {
        _ClosePseudoConsole(pPty);
    });

    unique_handle duplicatedInput;
    unique_handle duplicatedOutput;
    RETURN_IF_WIN32_BOOL_FALSE(DuplicateHandle(GetCurrentProcess(), hInput, GetCurrentProcess(), duplicatedInput.addressof(), 0, TRUE, DUPLICATE_SAME_ACCESS));
    RETURN_IF_WIN32_BOOL_FALSE(DuplicateHandle(GetCurrentProcess(), hOutput, GetCurrentProcess(), duplicatedOutput.addressof(), 0, TRUE, DUPLICATE_SAME_ACCESS));

    RETURN_IF_FAILED(_CreatePseudoConsole(hToken, size, duplicatedInput.get(), duplicatedOutput.get(), dwFlags, pPty));

    *phPC = (HPCON)pPty;
    cleanupPty.release();

    return S_OK;
}

// These functions are defined in the console l1 apiset, which is generated from
//      the consoleapi.apx file in minkernel\apiset\libs\Console.

// Function Description:
// Creates a "Pseudo-console" (conpty) with dimensions (in characters)
//      provided by the `size` parameter. The caller should provide two handles:
// - `hInput` is used for writing input to the pty, encoded as UTF-8 and VT sequences.
// - `hOutput` is used for reading the output of the pty, encoded as UTF-8 and VT sequences.
// Once the call completes, `phPty` will receive a token value to identify this
//      conpty object. This value should be used in conjunction with the other
//      Pseudoconsole API's.
// `dwFlags` is used to specify optional behavior to the created pseudoconsole.
// The flags can be combinations of the following values:
//  INHERIT_CURSOR: This will cause the created conpty to attempt to inherit the
//      cursor position of the parent terminal application. This can be useful
//      for applications like `ssh`, where ssh (currently running in a terminal)
//      might want to create a pseudoterminal session for an child application
//      and the child inherit the cursor position of ssh.
//      The created conpty will immediately emit a "Device Status Request" VT
//      sequence to hOutput, that should be replied to on hInput in the format
//      "\x1b[<r>;<c>R", where `<r>` is the row and `<c>` is the column of the
//      cursor position.
//      This requires a cooperating terminal application - if a caller does not
//      reply to this message, the conpty will not process any input until it
//      does. Most *nix terminals and the Windows Console (after Windows 10
//      Anniversary Update) will be able to handle such a message.

extern "C" HRESULT WINAPI ConptyCreatePseudoConsole(_In_ COORD size,
                                                    _In_ HANDLE hInput,
                                                    _In_ HANDLE hOutput,
                                                    _In_ DWORD dwFlags,
                                                    _Out_ HPCON* phPC)
{
    return ConptyCreatePseudoConsoleAsUser(INVALID_HANDLE_VALUE, size, hInput, hOutput, dwFlags, phPC);
}

// Function Description:
// Resizes the given conpty to the specified size, in characters.
extern "C" HRESULT WINAPI ConptyResizePseudoConsole(_In_ HPCON hPC, _In_ COORD size)
{
    const PseudoConsole* const pPty = (PseudoConsole*)hPC;
    HRESULT hr = pPty == nullptr ? E_INVALIDARG : S_OK;
    if (SUCCEEDED(hr))
    {
        hr = _ResizePseudoConsole(pPty, size);
    }
    return hr;
}

// Function Description:
// - Clear the contents of the conpty buffer, leaving the cursor row at the top
//   of the viewport.
// - This is used exclusively by ConPTY to support GH#1193, GH#1882. This allows
//   a terminal to clear the contents of the ConPTY buffer, which is important
//   if the user would like to be able to clear the terminal-side buffer.
extern "C" HRESULT WINAPI ConptyClearPseudoConsole(_In_ HPCON hPC)
{
    const PseudoConsole* const pPty = (PseudoConsole*)hPC;
    HRESULT hr = pPty == nullptr ? E_INVALIDARG : S_OK;
    if (SUCCEEDED(hr))
    {
        hr = _ClearPseudoConsole(pPty);
    }
    return hr;
}

// Function Description:
// Closes the conpty and all associated state.
// Client applications attached to the conpty will also behave as though the
//      console window they were running in was closed.
// This can fail if the conhost hosting the pseudoconsole failed to be
//      terminated, or if the pseudoconsole was already terminated.
extern "C" VOID WINAPI ConptyClosePseudoConsole(_In_ HPCON hPC)
{
    PseudoConsole* const pPty = (PseudoConsole*)hPC;
    if (pPty != nullptr)
    {
        _ClosePseudoConsole(pPty);
    }
}


//ConPTY is available only on Windows 10 released after 1903 (19H1) Windows release
class WindowsContext
{
private:
    WindowsContext() {}

public:
    typedef HRESULT (*CreatePseudoConsolePtr)(
            COORD size,         // ConPty Dimensions
            HANDLE hInput,      // ConPty Input
            HANDLE hOutput,	    // ConPty Output
            DWORD dwFlags,      // ConPty Flags
            HPCON* phPC);       // ConPty Reference

    typedef HRESULT (*ResizePseudoConsolePtr)(HPCON hPC, COORD size);

    typedef VOID (*ClosePseudoConsolePtr)(HPCON hPC);

    static WindowsContext &instance() 
    {
        static WindowsContext ctx;
        return ctx;
    }

    bool init()
    {
        createPseudoConsole = (CreatePseudoConsolePtr)ConptyCreatePseudoConsole;
        resizePseudoConsole = (ResizePseudoConsolePtr)ConptyResizePseudoConsole;
        closePseudoConsole = (ClosePseudoConsolePtr)ConptyClosePseudoConsole;

        return true;
    }

    QString lastError()
    {
        return m_lastError;
    }

public:
    //vars
    CreatePseudoConsolePtr createPseudoConsole{nullptr};
    ResizePseudoConsolePtr resizePseudoConsole{nullptr};
    ClosePseudoConsolePtr closePseudoConsole{nullptr};

private:
    QString m_lastError;
};

static bool checkConHostHasResizeQuirkOption()
{
  static bool hasResizeQuirk = std::invoke([](){
    QFile f(_InboxConsoleHostPath());
    if (!f.open(QIODevice::ReadOnly)) {
      qWarning() << "couldn't open conhost.exe, assuming no resizeQuirk.";
      return false;
    }
    // Conhost.exe should be around 1 MB
    if (f.size() > 5 * 1024 * 1024) {
      qWarning() << "conhost.exe is > 5MB, assuming no resizeQuirk.";
      return false;
    }
    QByteArray content = f.readAll();
    QString searchString("--resizeQuirk");
    QByteArrayView v((const char*)searchString.data(), searchString.length()*2);
    bool result = content.contains(v);
    if (!result)
      qDebug() << "No resizeQuirk option found in conhost.";
    return result;
  });

  return hasResizeQuirk;
}

HRESULT ConPtyProcess::createPseudoConsoleAndPipes(HPCON* phPC, HANDLE* phPipeIn, HANDLE* phPipeOut, qint16 cols, qint16 rows)
{
    HRESULT hr{ E_UNEXPECTED };
    HANDLE hPipePTYIn{ INVALID_HANDLE_VALUE };
    HANDLE hPipePTYOut{ INVALID_HANDLE_VALUE };

    // Create the pipes to which the ConPTY will connect
    if (CreatePipe(&hPipePTYIn, phPipeOut, NULL, 0) &&
            CreatePipe(phPipeIn, &hPipePTYOut, NULL, 0))
    {
        // Create the Pseudo Console of the required size, attached to the PTY-end of the pipes
        hr = WindowsContext::instance().createPseudoConsole({cols, rows}, hPipePTYIn, hPipePTYOut, checkConHostHasResizeQuirkOption() ? PSEUDOCONSOLE_RESIZE_QUIRK : 0, phPC);

        // Note: We can close the handles to the PTY-end of the pipes here
        // because the handles are dup'ed into the ConHost and will be released
        // when the ConPTY is destroyed.
        if (INVALID_HANDLE_VALUE != hPipePTYOut) CloseHandle(hPipePTYOut);
        if (INVALID_HANDLE_VALUE != hPipePTYIn) CloseHandle(hPipePTYIn);
    }

    return hr;
}

// Initializes the specified startup info struct with the required properties and
// updates its thread attribute list with the specified ConPTY handle
HRESULT ConPtyProcess::initializeStartupInfoAttachedToPseudoConsole(STARTUPINFOEX* pStartupInfo, HPCON hPC)
{
    HRESULT hr{ E_UNEXPECTED };

    if (pStartupInfo)
    {
        SIZE_T attrListSize{};

        pStartupInfo->StartupInfo.hStdInput = m_hPipeIn;
        pStartupInfo->StartupInfo.hStdError = m_hPipeOut;
        pStartupInfo->StartupInfo.hStdOutput = m_hPipeOut;
        pStartupInfo->StartupInfo.dwFlags |= STARTF_USESTDHANDLES;

        pStartupInfo->StartupInfo.cb = sizeof(STARTUPINFOEX);

        // Get the size of the thread attribute list.
        InitializeProcThreadAttributeList(NULL, 1, 0, &attrListSize);

        // Allocate a thread attribute list of the correct size
        pStartupInfo->lpAttributeList = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(
            HeapAlloc(GetProcessHeap(), 0, attrListSize));

        // Initialize thread attribute list
        if (pStartupInfo->lpAttributeList
                && InitializeProcThreadAttributeList(pStartupInfo->lpAttributeList, 1, 0, &attrListSize))
        {
            // Set Pseudo Console attribute
            hr = UpdateProcThreadAttribute(
                        pStartupInfo->lpAttributeList,
                        0,
                        PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE,
                        hPC,
                        sizeof(HPCON),
                        NULL,
                        NULL)
                    ? S_OK
                    : HRESULT_FROM_WIN32(GetLastError());
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }
    return hr;
}

Q_DECLARE_METATYPE(HANDLE)

ConPtyProcess::ConPtyProcess()
    : IPtyProcess()
    , m_ptyHandler { INVALID_HANDLE_VALUE }
    , m_hPipeIn { INVALID_HANDLE_VALUE }
    , m_hPipeOut { INVALID_HANDLE_VALUE }
    , m_readThread(nullptr)
{
   qRegisterMetaType<HANDLE>("HANDLE");
}

ConPtyProcess::~ConPtyProcess()
{
    kill();
}

bool ConPtyProcess::startProcess(const QString &executable,
                                 const QStringList &arguments,
                                 const QString &workingDir,
                                 QStringList environment,
                                 qint16 cols,
                                 qint16 rows)
{
    if (!isAvailable()) {
        m_lastError = WindowsContext::instance().lastError();
        return false;
    }

    //already running
    if (m_ptyHandler != INVALID_HANDLE_VALUE)
        return false;

    QFileInfo fi(executable);
    if (fi.isRelative() || !QFile::exists(executable)) {
        //todo add auto-find executable in PATH env var
        m_lastError = QString("ConPty Error: shell file path '%1' must be absolute").arg(executable);
        return false;
    }

    m_shellPath = executable;
    m_shellPath.replace('/', '\\');
    m_size = QPair<qint16, qint16>(cols, rows);

    //env
    const QString env = environment.join(QChar(QChar::Null)) + QChar(QChar::Null);
    LPVOID envPtr = env.isEmpty() ? nullptr : (LPVOID) env.utf16();

    LPCWSTR workingDirPtr = workingDir.isEmpty() ? nullptr : (LPCWSTR) workingDir.utf16();

    QStringList exeAndArgs = arguments;
    exeAndArgs.prepend(m_shellPath);
    std::wstring cmdArg{(LPCWSTR) (exeAndArgs.join(QLatin1String(" ")).utf16())};

    HRESULT hr{E_UNEXPECTED};

    //  Create the Pseudo Console and pipes to it
    hr = createPseudoConsoleAndPipes(&m_ptyHandler, &m_hPipeIn, &m_hPipeOut, cols, rows);

    if (S_OK != hr) {
        m_lastError = QString("ConPty Error: CreatePseudoConsoleAndPipes fail");
        return false;
    }

    // Initialize the necessary startup info struct
    if (S_OK != initializeStartupInfoAttachedToPseudoConsole(&m_shellStartupInfo, m_ptyHandler)) {
        m_lastError = QString("ConPty Error: InitializeStartupInfoAttachedToPseudoConsole fail");
        return false;
    }

    hr = CreateProcessW(nullptr,       // No module name - use Command Line
                        cmdArg.data(), // Command Line
                        nullptr,       // Process handle not inheritable
                        nullptr,       // Thread handle not inheritable
                        FALSE,         // Inherit handles
                        EXTENDED_STARTUPINFO_PRESENT | CREATE_UNICODE_ENVIRONMENT, // Creation flags
                        envPtr,                          // Environment block
                        workingDirPtr,                   // Use parent's starting directory
                        &m_shellStartupInfo.StartupInfo, // Pointer to STARTUPINFO
                        &m_shellProcessInformation)      // Pointer to PROCESS_INFORMATION
             ? S_OK
             : GetLastError();

    if (S_OK != hr) {
        m_lastError = QString("ConPty Error: Cannot create process -> %1").arg(hr);
        return false;
    }

    m_pid = m_shellProcessInformation.dwProcessId;

    // Notify when the shell process has been terminated
    m_shellCloseWaitNotifier = new QWinEventNotifier(m_shellProcessInformation.hProcess, notifier());
    QObject::connect(m_shellCloseWaitNotifier,
                     &QWinEventNotifier::activated,
                     notifier(),
                     [this](HANDLE hEvent) {
                         DWORD exitCode = 0;
                         GetExitCodeProcess(hEvent, &exitCode);
                         m_exitCode = exitCode;
                         // Do not respawn if the object is about to be destructed
                         if (!m_aboutToDestruct)
                             emit notifier()->aboutToClose();
                         m_shellCloseWaitNotifier->setEnabled(false);
                     }, Qt::QueuedConnection);

    //this code runned in separate thread
    m_readThread = QThread::create([this]() {
        //buffers
        const DWORD BUFF_SIZE{1024};
        char szBuffer[BUFF_SIZE]{};

        forever {
            DWORD dwBytesRead{};

            // Read from the pipe
            BOOL result = ReadFile(m_hPipeIn, szBuffer, BUFF_SIZE, &dwBytesRead, NULL);

            const bool needMoreData = !result && GetLastError() == ERROR_MORE_DATA;
            if (result || needMoreData) {
                QMutexLocker locker(&m_bufferMutex);
                m_buffer.m_readBuffer.append(szBuffer, dwBytesRead);
                m_buffer.emitReadyRead();
            }

            const bool brokenPipe = !result && GetLastError() == ERROR_BROKEN_PIPE;
            if (QThread::currentThread()->isInterruptionRequested() || brokenPipe)
                break;
        }

        CancelIoEx(m_hPipeIn, nullptr);
    });

    //start read thread
    m_readThread->start();

    return true;
}

bool ConPtyProcess::resize(qint16 cols, qint16 rows)
{
    if (m_ptyHandler == nullptr)
    {
        return false;
    }

    bool res = SUCCEEDED(WindowsContext::instance().resizePseudoConsole(m_ptyHandler, {cols, rows}));

    if (res)
    {
        m_size = QPair<qint16, qint16>(cols, rows);
    }

    return res;

    return true;
}

bool ConPtyProcess::kill()
{
    bool exitCode = false;

    if (m_ptyHandler != INVALID_HANDLE_VALUE) {
        m_aboutToDestruct = true;

        // Close ConPTY - this will terminate client process if running
        WindowsContext::instance().closePseudoConsole(m_ptyHandler);

        // Clean-up the pipes
        if (INVALID_HANDLE_VALUE != m_hPipeOut)
            CloseHandle(m_hPipeOut);
        if (INVALID_HANDLE_VALUE != m_hPipeIn)
            CloseHandle(m_hPipeIn);

        if (m_readThread) {
            m_readThread->requestInterruption();
            if (!m_readThread->wait(1000))
                m_readThread->terminate();
            m_readThread->deleteLater();
            m_readThread = nullptr;
        }

        delete m_shellCloseWaitNotifier;
        m_shellCloseWaitNotifier = nullptr;

        m_pid = 0;
        m_ptyHandler = INVALID_HANDLE_VALUE;
        m_hPipeIn = INVALID_HANDLE_VALUE;
        m_hPipeOut = INVALID_HANDLE_VALUE;

        CloseHandle(m_shellProcessInformation.hThread);
        CloseHandle(m_shellProcessInformation.hProcess);

        // Cleanup attribute list
        if (m_shellStartupInfo.lpAttributeList) {
            DeleteProcThreadAttributeList(m_shellStartupInfo.lpAttributeList);
            HeapFree(GetProcessHeap(), 0, m_shellStartupInfo.lpAttributeList);
        }

        exitCode = true;
    }

    return exitCode;
}

IPtyProcess::PtyType ConPtyProcess::type()
{
    return PtyType::ConPty;
}

QString ConPtyProcess::dumpDebugInfo()
{
#ifdef PTYQT_DEBUG
    return QString("PID: %1, Type: %2, Cols: %3, Rows: %4")
            .arg(m_pid).arg(type())
            .arg(m_size.first).arg(m_size.second);
#else
    return QString("Nothing...");
#endif
}

QIODevice *ConPtyProcess::notifier()
{
    return &m_buffer;
}

QByteArray ConPtyProcess::readAll()
{
    QByteArray result;
    {
        QMutexLocker locker(&m_bufferMutex);
        result.swap(m_buffer.m_readBuffer);
    }
    return result;
}

qint64 ConPtyProcess::write(const QByteArray &byteArray)
{
    DWORD dwBytesWritten{};
    WriteFile(m_hPipeOut, byteArray.data(), byteArray.size(), &dwBytesWritten, NULL);
    return dwBytesWritten;
}

bool ConPtyProcess::isAvailable()
{
    qint32 buildNumber = QSysInfo::kernelVersion().split(".").last().toInt();
    if (buildNumber < CONPTY_MINIMAL_WINDOWS_VERSION)
        return false;
    return WindowsContext::instance().init();
}

void ConPtyProcess::moveToThread(QThread *targetThread)
{
    //nothing for now...
}
