/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef ABSTRACTPROCESS_H
#define ABSTRACTPROCESS_H

#include <QtCore/QStringList>

namespace ProjectExplorer {
namespace Internal {

class AbstractProcess
{
public:
    AbstractProcess() {}
    virtual ~AbstractProcess() {}

    QString workingDirectory() const { return m_workingDir; }
    void setWorkingDirectory(const QString &dir) { m_workingDir = dir; }

    QStringList environment() const { return m_environment; }
    void setEnvironment(const QStringList &env) { m_environment = env; }

    virtual bool start(const QString &program, const QStringList &args) = 0;
    virtual void stop() = 0;

    virtual bool isRunning() const = 0;
    virtual qint64 applicationPID() const = 0;
    virtual int exitCode() const = 0;

//signals:
    virtual void processError(const QString &error) = 0;

private:
    QString m_workingDir;
    QStringList m_environment;
};

} //namespace Internal
} //namespace Qt4ProjectManager

#endif // ABSTRACTPROCESS_H

