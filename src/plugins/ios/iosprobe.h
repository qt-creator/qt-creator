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
#include <QSettings>
#include <QSharedPointer>
#include <QString>
#include <QStringList>
#include <utils/fileutils.h>

namespace Ios {

class Platform
{
public:
    enum PlatformKind {
        BasePlatform = 1 << 0,
        Cxx11Support = 1 << 1
    };

    enum CompilerType {
        CLang,
        GCC
    };

    quint32 platformKind;
    CompilerType type;
    QString name;
    Utils::FileName developerPath;
    Utils::FileName platformPath;
    Utils::FileName sdkPath;
    Utils::FileName defaultToolchainPath;
    Utils::FileName cxxCompilerPath;
    Utils::FileName cCompilerPath;
    QString architecture;
    QStringList backendFlags;

    // ignores anything besides name
    bool operator==(const Platform &other) const;
};

uint qHash(const Platform &platform);
QDebug operator<<(QDebug debug, const Platform &platform);

class IosProbe
{
public:
    static QMap<QString, Platform> detectPlatforms(const QString &devPath = QString());
    IosProbe()
    { }

private:
    void addDeveloperPath(const QString &path);
    void detectDeveloperPaths();
    void setupDefaultToolchains(const QString &devPath, const QString &xcodeName);
    void detectFirst();
    QMap<QString, Platform> detectedPlatforms();
private:
    QMap<QString, Platform> m_platforms;
    QStringList m_developerPaths;
};
} // namespace Ios
