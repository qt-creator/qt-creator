/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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
#ifndef IOSPROBE_H
#define IOSPROBE_H
#include <QSettings>
#include <QSharedPointer>
#include <QString>
#include <QStringList>
#include <utils/fileutils.h>

namespace Ios {

typedef QSharedPointer<QSettings> QSettingsPtr;

class Platform
{
public:
    enum PlatformKind {
        BasePlatform = 1 << 0,
        Cxx11Support = 1 << 1
    };

    quint32 platformKind;
    QString name;
    Utils::FileName developerPath;
    Utils::FileName platformPath;
    Utils::FileName sdkPath;
    Utils::FileName defaultToolchainPath;
    Utils::FileName compilerPath;
    QString architecture;
    QStringList backendFlags;
    QSettingsPtr platformInfo;
    QSettingsPtr sdkSettings;
};

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

#endif // IOSPROBE_H
