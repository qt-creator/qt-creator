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

#pragma once

#include <QProcess>

class FakeProcess : public QObject
{
    Q_OBJECT

public:
    FakeProcess();
    ~FakeProcess();

    void finishUnsuccessfully();
    void finishByCrash();
    void finish();

    void start();
    void setArguments(const QStringList &arguments);
    void setProgram(const QString &program);

    void setProcessChannelMode(QProcess::ProcessChannelMode mode);


    bool isStarted() const;

    const QStringList &arguments() const;
    const QString &applicationPath() const;

signals:
    void finished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    QStringList m_arguments;
    QString m_applicationPath;
    bool m_isFinished = false;
    bool m_isStarted = false;
};
