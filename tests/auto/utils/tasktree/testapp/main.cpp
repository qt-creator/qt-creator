// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <app/app_version.h>

#include <QString>
#include <QThread>

#ifdef Q_OS_WIN
#include <crtdbg.h>
#include <cstdlib>
#endif

const char CRASH_OPTION[] = "-crash";
const char RETURN_OPTION[] = "-return";
const char SLEEP_OPTION[] = "-sleep";

int main(int argc, char **argv)
{
#ifdef Q_OS_WIN
    // avoid crash reporter dialog
    _set_error_mode(_OUT_TO_STDERR);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
#endif

    if (argc > 1) {
        const auto arg = QString::fromLocal8Bit(argv[1]);
        if (arg == CRASH_OPTION) {
            qFatal("The application has crashed purposefully!");
            return 1;
        }
        if (arg == RETURN_OPTION) {
            if (argc > 2) {
                const auto retString = QString::fromLocal8Bit(argv[2]);
                bool ok = false;
                const int retVal = retString.toInt(&ok);
                if (ok)
                    return retVal;
                // not an int return value
                return 1;
            }
            // lacking return value
            return 1;
        }
        if (arg == SLEEP_OPTION) {
            if (argc > 2) {
                const auto secondsString = QString::fromLocal8Bit(argv[2]);
                bool ok = false;
                const int secondsVal = secondsString.toInt(&ok);
                if (ok) {
                    QThread::sleep(secondsVal);
                    return 0;
                }
                // not an int return value
                return 1;
            }
            // lacking return value
            return 1;
        }
    }
    // not recognized option
    return 1;
}
