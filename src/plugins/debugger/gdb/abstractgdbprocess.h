/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
