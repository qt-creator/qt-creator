// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0
#pragma once

#include <QtGlobal>

#ifdef ENABLE_QT_BREAKPAD
#include <qtsystemexceptionhandler.h>
#endif

#if defined(ENABLE_CRASHPAD) && defined(Q_OS_WIN)
#define NOMINMAX
#include "client/crash_report_database.h"
#include "client/crashpad_client.h"
#include "client/settings.h"
#endif

namespace {
#if defined(ENABLE_CRASHPAD) && defined(Q_OS_WIN)
    bool startCrashpad()
    {
        using namespace crashpad;

        // Cache directory that will store crashpad information and minidumps
        base::FilePath database(L"crashpad_reports");
        base::FilePath handler(L"crashpad_handler.exe");

        // URL used to submit minidumps to
        std::string url(CRASHPAD_BACKEND_URL);

        // Optional annotations passed via --annotations to the handler
        std::map<std::string, std::string> annotations;
        annotations["qt-version"] = QT_VERSION_STR;

        // Optional arguments to pass to the handler
        std::vector<std::string> arguments;
        arguments.push_back("--no-rate-limit");

        CrashpadClient *client = new CrashpadClient();
        bool success = client->StartHandler(handler,
                                            database,
                                            database,
                                            url,
                                            annotations,
                                            arguments,
                                            /* restartable */ true,
                                            /* asynchronous_start */ true);
        // TODO: research using this method, should avoid creating a separate CrashpadClient for the
        // puppet (needed only on windows according to docs).
        //    client->SetHandlerIPCPipe(L"\\\\.\\pipe\\qml2puppet");

        return success;
    }

#ifdef ENABLE_QT_BREAKPAD
    const QString libexecPath = QCoreApplication::applicationDirPath() + '/'
                                + RELATIVE_LIBEXEC_PATH;
    QtSystemExceptionHandler systemExceptionHandler(libexecPath);
#endif //#ifdef ENABLE_QT_BREAKPAD
#else //#if defined(ENABLE_CRASHPAD) && defined(Q_OS_WIN)
    bool startCrashpad()
    {
        return false;
    }
#endif //#if defined(ENABLE_CRASHPAD) && defined(Q_OS_WIN)
}
