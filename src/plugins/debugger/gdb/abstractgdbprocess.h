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

#ifndef GDBPROCESSWRAPPER_H
#define GDBPROCESSWRAPPER_H

#include <QtCore/QObject>
#include <QtCore/QProcess>

namespace Debugger {
namespace Internal {

class AbstractGdbProcess : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(AbstractGdbProcess)
public:
    virtual QByteArray readAllStandardOutput() = 0;
    virtual QByteArray readAllStandardError() = 0;

    virtual void start(const QString &cmd, const QStringList &args) = 0;
    virtual bool waitForStarted() = 0;
    virtual qint64 write(const QByteArray &data) = 0;
    virtual void kill() = 0;

    virtual QProcess::ProcessState state() const = 0;
    virtual QString errorString() const = 0;

    virtual QProcessEnvironment processEnvironment() const = 0;
    virtual void setProcessEnvironment(const QProcessEnvironment &env) = 0;
    virtual void setEnvironment(const QStringList &env) = 0;
    virtual void setWorkingDirectory(const QString &dir) = 0;

    virtual ~AbstractGdbProcess() {}

signals:
    void error(QProcess::ProcessError);
    void finished(int exitCode, QProcess::ExitStatus exitStatus);
    void readyReadStandardError();
    void readyReadStandardOutput();

protected:
    explicit AbstractGdbProcess(QObject *parent = 0) : QObject(parent) {}

};

} // namespace Internal
} // namespace Debugger

#endif // GDBPROCESSWRAPPER_H
