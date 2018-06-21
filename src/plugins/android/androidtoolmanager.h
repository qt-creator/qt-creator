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

#include "utils/fileutils.h"
#include "androidconfigurations.h"

#include <QStringList>

#include <memory>

namespace Android {
class AndroidConfig;

namespace Internal {

class AndroidToolOutputParser;
/*!
    Wraps the \c android tool's usage. The tool itself is deprecated since SDK tools version 25.3.0.
 */
class AndroidToolManager
{
    Q_DECLARE_TR_FUNCTIONS(AndroidToolManager)

public:
    AndroidToolManager(const AndroidConfig &config);
    ~AndroidToolManager();

    SdkPlatformList availableSdkPlatforms(bool *ok = nullptr) const;
    void launchAvdManager() const;

    QFuture<CreateAvdInfo> createAvd(CreateAvdInfo info) const;
    bool removeAvd(const QString &name) const;
    QFuture<AndroidDeviceInfoList> androidVirtualDevicesFuture() const;

// Helper methods
private:
    static CreateAvdInfo createAvdImpl(CreateAvdInfo info, Utils::FileName androidToolPath,
                                       QProcessEnvironment env);
    static AndroidDeviceInfoList androidVirtualDevices(const Utils::FileName &androidTool,
                                                            const Utils::FileName &sdkLlocationPath,
                                                            const QProcessEnvironment &env);
private:
    const AndroidConfig &m_config;
    std::unique_ptr<AndroidToolOutputParser> m_parser;
};

} // namespace Internal
} // namespace Android
