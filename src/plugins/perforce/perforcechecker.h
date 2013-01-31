/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef PERFORCECHECKER_H
#define PERFORCECHECKER_H

#include <QObject>
#include <QProcess>

namespace Perforce {
namespace Internal {

// Perforce checker:  Calls perforce asynchronously to do
// a check of the configuration and emits signals with the top level or
// an error message.

class PerforceChecker : public QObject
{
    Q_OBJECT
public:
    explicit PerforceChecker(QObject *parent = 0);
    ~PerforceChecker();

public slots:
    void start(const QString &binary,
               const QStringList &basicArgs = QStringList(),
               int timeoutMS = -1);

    bool isRunning() const;

    bool useOverideCursor() const;
    void setUseOverideCursor(bool v);

signals:
    void succeeded(const QString &repositoryRoot);
    void failed(const QString &errorMessage);

private slots:
    void slotError(QProcess::ProcessError error);
    void slotFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void slotTimeOut();

private:
    void emitFailed(const QString &);
    void emitSucceeded(const QString &);
    void parseOutput(const QString &);
    inline void resetOverrideCursor();

    QProcess m_process;
    QString m_binary;
    int m_timeOutMS;
    bool m_timedOut;
    bool m_useOverideCursor;
    bool m_isOverrideCursor;
};

}
}

#endif // PERFORCECHECKER_H
