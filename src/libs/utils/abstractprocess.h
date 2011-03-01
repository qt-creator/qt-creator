/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef ABSTRACTPROCESS_H
#define ABSTRACTPROCESS_H

#include "utils_global.h"

#include "environment.h"

#include <QtCore/QStringList>

namespace Utils {

class QTCREATOR_UTILS_EXPORT AbstractProcess
{
public:
    AbstractProcess();
    virtual ~AbstractProcess();

    QString workingDirectory() const;
    void setWorkingDirectory(const QString &dir);

    void setEnvironment(const Environment &env);
    Environment environment() const;

    virtual bool start(const QString &program, const QString &args) = 0;
    virtual void stop() = 0;

    virtual bool isRunning() const = 0;
    virtual qint64 applicationPID() const = 0;
    virtual int exitCode() const = 0;

    static QString msgWinCannotRetrieveDebuggingOutput();

//signals:
    virtual void processMessage(const QString &error, bool isError) = 0;

#ifdef Q_OS_WIN
    // Add PATH and SystemRoot environment variables in case they are missing
    static QStringList fixWinEnvironment(const QStringList &env);
    // Quote a Windows command line correctly for the "CreateProcess" API
    static QString createWinCommandline(const QString &program, const QStringList &args);
    static QString createWinCommandline(const QString &program, const QString &args);
    // Create a bytearray suitable to be passed on as environment
    // to the "CreateProcess" API (0-terminated UTF 16 strings).
    static QByteArray createWinEnvironment(const QStringList &env);
#endif

protected:
    QString m_workingDir;
    Environment m_environment;
};

} //namespace Utils

#endif // ABSTRACTPROCESS_H

