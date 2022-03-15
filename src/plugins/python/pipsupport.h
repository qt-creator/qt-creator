/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include <utils/filepath.h>
#include <utils/qtcprocess.h>

#include <QFutureWatcher>

namespace Python {
namespace Internal {

class PipPackage
{
public:
    explicit PipPackage(const QString &packageName = {},
                        const QString &displayName = {},
                        const QString &version = {})
        : packageName(packageName)
        , displayName(displayName.isEmpty() ? packageName : displayName)
        , version(version)
    {}
    QString packageName;
    QString displayName;
    QString version;
};

class PipInstallTask : public QObject
{
    Q_OBJECT
public:
    explicit PipInstallTask(const Utils::FilePath &python);
    void setPackage(const PipPackage &package);
    void run();

signals:
    void finished(bool success);

private:
    void cancel();
    void installFinished();
    void handleOutput();
    void hanleError();

    const Utils::FilePath m_python;
    PipPackage m_package;
    Utils::QtcProcess m_process;
    QFutureInterface<void> m_future;
    QFutureWatcher<void> m_watcher;
    QTimer m_killTimer;
};

} // namespace Internal
} // namespace Python
