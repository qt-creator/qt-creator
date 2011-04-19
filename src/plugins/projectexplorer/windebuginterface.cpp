/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "windebuginterface.h"

namespace ProjectExplorer {
namespace Internal {

WinDebugInterface *WinDebugInterface::m_instance = 0;

WinDebugInterface *WinDebugInterface::instance()
{
    return m_instance;
}


WinDebugInterface::WinDebugInterface(QObject *parent) :
    QThread(parent)
{
    m_instance = this;
    start();
}

void WinDebugInterface::run()
{
    HANDLE bufferReadyEvent = CreateEvent(NULL, FALSE, FALSE, L"DBWIN_BUFFER_READY");
    if (!bufferReadyEvent)
        return;
    HANDLE dataReadyEvent = CreateEvent(NULL, FALSE, FALSE, L"DBWIN_DATA_READY");
    if (!dataReadyEvent)
        return;
    HANDLE sharedFile = CreateFileMapping((HANDLE)-1, NULL, PAGE_READWRITE, 0, 4096, L"DBWIN_BUFFER");
    if (!sharedFile)
        return;
    LPVOID sharedMem = MapViewOfFile(sharedFile, FILE_MAP_READ, 0, 0,  512);
    if (!sharedMem)
        return;

    LPSTR  message;
    LPDWORD processId;

    message = reinterpret_cast<LPSTR>(sharedMem) + sizeof(DWORD);
    processId = reinterpret_cast<LPDWORD>(sharedMem);

    SetEvent(bufferReadyEvent);

    while (true) {
        DWORD ret = WaitForSingleObject(dataReadyEvent, INFINITE);

        if (ret == WAIT_OBJECT_0) {
            emit debugOutput(*processId, QString::fromLocal8Bit(message));
            SetEvent(bufferReadyEvent);
        }
    }
}

} // namespace Internal
} // namespace ProjectExplorer
