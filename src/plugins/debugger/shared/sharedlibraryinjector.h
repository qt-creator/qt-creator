/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Marius Storm-Olsen
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

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
