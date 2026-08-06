// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

// Runs etwcapture.exe with administrator rights and waits for it. The embedded
// manifest asks for those rights, so Qt Creator gets the consent prompt by
// starting this through ShellExecuteEx. Being elevated by the service, it does
// not inherit Qt Creator's environment, which is why it has to compose the
// PATH etwcapture.exe is loaded with.
//
// Usage: etwcapture-launcher.exe --library-path <dir> <etwcapture arguments>
//
// Everything from the third argument on is passed to etwcapture.exe as it was
// written, without being split and quoted a second time.

#include <string>

#include <windows.h>
#include <shellapi.h>

namespace {

class ScopedHandle
{
public:
    explicit ScopedHandle(HANDLE handle) : m_handle(handle) {}
    ~ScopedHandle()
    {
        if (isValid())
            CloseHandle(m_handle);
    }

    ScopedHandle(const ScopedHandle &) = delete;
    ScopedHandle &operator=(const ScopedHandle &) = delete;

    bool isValid() const { return m_handle != INVALID_HANDLE_VALUE && m_handle != nullptr; }
    operator HANDLE() const { return m_handle; }

private:
    HANDLE m_handle;
};

// The exit code for a failed Win32 call, which Qt Creator reports back. Never
// zero, which would read as a successful recording.
int lastErrorExitCode()
{
    const DWORD error = GetLastError();
    return static_cast<int>(error == ERROR_SUCCESS ? ERROR_INVALID_FUNCTION : error);
}

// The directory this executable is in, with a trailing separator.
std::wstring executableDirectory()
{
    std::wstring path(MAX_PATH, L'\0');
    DWORD length = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
    while (length == path.size()) {
        path.resize(path.size() * 2, L'\0');
        length = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
    }
    if (length == 0)
        return {};
    path.resize(length);

    const size_t separator = path.find_last_of(L'\\');
    return separator == std::wstring::npos ? std::wstring() : path.substr(0, separator + 1);
}

// The two directories next to this executable come first, so that nothing the
// caller passes can take precedence over them. Only the Qt library directory
// is taken from the caller, because a Qt Creator that is not installed next to
// Qt cannot be found from here.
bool setChildPath(const std::wstring &directory, const std::wstring &libraryPath)
{
    std::wstring path = directory + L';' + directory + ETWCAPTURE_PLUGIN_PATH;
    if (!libraryPath.empty())
        path += L';' + libraryPath;

    const DWORD size = GetEnvironmentVariableW(L"PATH", nullptr, 0);
    if (size > 1) {
        std::wstring inherited(size, L'\0');
        const DWORD length = GetEnvironmentVariableW(L"PATH", inherited.data(), size);
        if (length == 0 || length >= size)
            return false;
        inherited.resize(length);
        path += L';' + inherited;
    }
    return SetEnvironmentVariableW(L"PATH", path.c_str()) != FALSE;
}

// Advances past one argument of a command line and the whitespace behind it,
// the way CommandLineToArgvW ends one: on unquoted whitespace, with a run of
// backslashes escaping a quote that follows it. The program name is quoted or
// not, and knows no escapes.
const wchar_t *skipArgument(const wchar_t *argument, bool isProgramName)
{
    bool quoted = false;
    while (*argument != L'\0' && (quoted || (*argument != L' ' && *argument != L'\t'))) {
        if (*argument == L'"') {
            quoted = !quoted;
            ++argument;
        } else if (*argument == L'\\' && !isProgramName) {
            size_t backslashes = 0;
            for (; *argument == L'\\'; ++argument)
                ++backslashes;
            if (*argument == L'"' && backslashes % 2 == 1)
                ++argument;
        } else {
            ++argument;
        }
    }
    for (; *argument == L' ' || *argument == L'\t'; ++argument) {}
    return argument;
}

} // namespace

int WINAPI wWinMain(HINSTANCE, HINSTANCE, wchar_t *, int)
{
    int argumentCount = 0;
    wchar_t **arguments = CommandLineToArgvW(GetCommandLineW(), &argumentCount);
    if (!arguments || argumentCount < 3 || std::wstring(arguments[1]) != L"--library-path")
        return ERROR_INVALID_PARAMETER;
    const std::wstring libraryPath = arguments[2];
    LocalFree(arguments);

    const std::wstring directory = executableDirectory();
    if (directory.empty())
        return lastErrorExitCode();
    if (!setChildPath(directory, libraryPath))
        return lastErrorExitCode();

    const wchar_t *forwarded = GetCommandLineW();
    for (int i = 0; i < 3; ++i)
        forwarded = skipArgument(forwarded, i == 0);

    std::wstring commandLine = L'"' + directory + L"etwcapture.exe\" ";
    commandLine += forwarded;

    // Ending this process must not leave an elevated capture behind, so
    // etwcapture.exe is started suspended, put into a job that kills its
    // members once the last handle to it is gone, and only then resumed.
    ScopedHandle job{CreateJobObjectW(nullptr, nullptr)};
    if (!job.isValid())
        return lastErrorExitCode();
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits {};
    limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if (!SetInformationJobObject(job, JobObjectExtendedLimitInformation, &limits, sizeof(limits)))
        return lastErrorExitCode();

    STARTUPINFOW startupInfo {sizeof(startupInfo)};
    startupInfo.dwFlags = STARTF_USESHOWWINDOW;
    startupInfo.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION processInfo {};
    if (!CreateProcessW(nullptr, commandLine.data(), nullptr, nullptr, FALSE,
                        CREATE_NO_WINDOW | CREATE_SUSPENDED, nullptr, directory.c_str(),
                        &startupInfo, &processInfo)) {
        return lastErrorExitCode();
    }

    const ScopedHandle process{processInfo.hProcess};
    const ScopedHandle thread{processInfo.hThread};
    if (!AssignProcessToJobObject(job, process)
        || ResumeThread(thread) == static_cast<DWORD>(-1)) {
        const int exitCode = lastErrorExitCode();
        TerminateProcess(process, 1);
        return exitCode;
    }

    if (WaitForSingleObject(process, INFINITE) != WAIT_OBJECT_0)
        return lastErrorExitCode();

    DWORD exitCode = 0;
    if (!GetExitCodeProcess(process, &exitCode))
        return lastErrorExitCode();
    return static_cast<int>(exitCode);
}
