/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

//
// WinANSI - An ANSI implementation for the modern Windows
//
// Copyright (C) 2008- Marius Storm-Olsen <mstormo@gmail.com>
//
// Based on work by Robert Kuster in article
//   http://software.rkuster.com/articles/winspy.htm
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// -------------------------------------------------------------------------------------------------

#ifndef SHAREDLIBRARYINJECTOR_H
#define SHAREDLIBRARYINJECTOR_H

#include <windows.h>
#include <QtCore/QString>

namespace Debugger {
namespace Internal {

/* SharedLibraryInjector: Injects a DLL into a remote process.
 * Escalates the calling process rights. */
class SharedLibraryInjector {
    Q_DISABLE_COPY(SharedLibraryInjector)
public:

    explicit SharedLibraryInjector(unsigned long remotePid, unsigned long remoteThreadId = 0);
    ~SharedLibraryInjector();

    void setModulePath(const QString &modulePath);
    bool hasLoaded(const QString &modulePath = QString());

    // Remote injection, to be used for running processes
    bool remoteInject(const QString &modulePath, bool waitForThread, QString *errorMessage);

    // Stub injection, to be used before execution starts
    bool stubInject(const QString &modulePath, unsigned long entryPoint, QString *errorMessage);

    bool unload(const QString &modulePath /*  = QString()*/, QString *errorMessage);
    bool unload(HMODULE hFreeModule, QString *errorMessage);

    void setPid(unsigned long pid);
    void setThreadId(unsigned long tid);

    static QString findModule(const QString &moduleName);
    static unsigned long getModuleEntryPoint(const QString &moduleName);

private:
    bool escalatePrivileges(QString *errorMessage);
    bool doRemoteInjection(unsigned long pid, HMODULE hFreeModule,
                           const QString &modulePath, bool waitForThread,
                           QString *errorMessage);
    bool doStubInjection(unsigned long pid, const QString &modulePath,
                         unsigned long entryPoint, QString *errorMessage);

    HMODULE findModuleHandle(const QString &modulePath, QString *errorMessage);

    unsigned long m_processId;
    unsigned long m_threadId;
    QString m_modulePath;
    bool m_hasEscalatedPrivileges;
};

} // namespace Internal
} // namespace Debugger

#endif // SHAREDLIBRARYINJECTOR_H
