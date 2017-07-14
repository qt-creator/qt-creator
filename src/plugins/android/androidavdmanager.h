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

#include "androidconfigurations.h"

#include <memory>

namespace Android {
namespace Internal {

class AndroidToolManager;
class AvdManagerOutputParser;

class AndroidAvdManager
{
public:
    AndroidAvdManager(const AndroidConfig& config = AndroidConfigurations::currentConfig());
    ~AndroidAvdManager();

    void launchAvdManagerUiTool() const;
    QFuture<CreateAvdInfo> createAvd(CreateAvdInfo info) const;
    bool removeAvd(const QString &name) const;
    QFuture<AndroidDeviceInfoList> avdList() const;

    QString startAvd(const QString &name) const;
    bool startAvdAsync(const QString &avdName) const;
    QString findAvd(const QString &avdName) const;
    QString waitForAvd(const QString &avdName,
                       const QFutureInterface<bool> &fi = QFutureInterface<bool>()) const;
    bool isAvdBooted(const QString &device) const;

private:
    bool waitForBooted(const QString &serialNumber, const QFutureInterface<bool> &fi) const;

private:
    const AndroidConfig &m_config;
    std::unique_ptr<AndroidToolManager> m_androidTool;
    std::unique_ptr<AvdManagerOutputParser> m_parser;
};

} // namespace Internal
} // namespace Android
