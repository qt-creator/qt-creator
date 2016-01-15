/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    int pid = -1;
    const size_t maxPipeNameSize = 256;
    wchar_t pipeName[maxPipeNameSize] = { 0 };

    for (int i = 0; i < argc - 1; ++i) {
        if (!strcmp(argv[i], "-t")) {
            ++i;
            if (swprintf(pipeName, maxPipeNameSize, L"%hs", argv[i]) < 0)
                return 0; // Pipe name too long
        } else if (!strcmp(argv[i], "-p")) {
            ++i;

            // check if -p is followed by a number
            const char *pidString = argv[i];
            char *end;
            pid = strtoul(pidString, &end, 0);
            if (*end != 0)
                return 0;
        }
    }

    if (pid < 0)
        return 0;

    if (*pipeName == 0)
        swprintf(pipeName, maxPipeNameSize, L"\\\\.\\pipe\\QtCreatorWinRtDebugPIDPipe");
    HANDLE pipe;
    while (true) {
        pipe = CreateFile(pipeName, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
        if (pipe != INVALID_HANDLE_VALUE)
            break;
        if ((GetLastError() != ERROR_PIPE_BUSY) || (!WaitNamedPipe(pipeName, 10000)))
            return 0;
    }

    const size_t msgBufferSize = 15;
    char pidMessageBuffer[msgBufferSize];
    int length = sprintf_s(pidMessageBuffer, msgBufferSize, "PID:%d", pid);
    if (length >= 0)
        WriteFile(pipe, pidMessageBuffer, length, NULL, NULL);

    return 0;
}
