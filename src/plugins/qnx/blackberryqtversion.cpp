/**************************************************************************
**
** Copyright (C) 2011 - 2012 Research In Motion
**
** Contact: Research In Motion (blackberry-qt@qnx.com)
** Contact: KDAB (info@kdab.com)
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

#include "blackberryqtversion.h"

#include "qnxconstants.h"

#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <QTextStream>

using namespace Qnx;
using namespace Qnx::Internal;

namespace {
QMultiMap<QString, QString> parseEnvironmentFile(const QString &fileName)
{
    QMultiMap<QString, QString> result;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly))
        return result;

    QTextStream str(&file);
    QMap<QString, QString> fileContent;
    while (!str.atEnd()) {
        QString line = str.readLine();
        if (!line.contains(QLatin1Char('=')))
            continue;

        const QStringList lineContent = line.split(QLatin1Char('='));
        QString var = lineContent.value(0);
        //Remove set in front
        if (var.startsWith(QLatin1String("set ")))
            var = var.right(var.size() - 4);
        QString value = lineContent.value(1).section(QLatin1Char('"'), 0, -1, QString::SectionSkipEmpty);

        fileContent[var] = value;
    }
    file.close();

    QMapIterator<QString, QString> it(fileContent);
    while (it.hasNext()) {
        it.next();
        const QStringList values
                = it.value().split(Utils::HostOsInfo::pathListSeparator());
        QString key = it.key();
        foreach (const QString &value, values) {
            const QString ownKeyAsWindowsVar = QLatin1Char('%') + key + QLatin1Char('%');
            const QString ownKeyAsUnixVar = QLatin1Char('$') + key;
            if (!value.contains(ownKeyAsWindowsVar) && !value.contains(ownKeyAsUnixVar)) {
                QString val = value;
                if (val.contains(QLatin1Char('%')) || val.contains(QLatin1Char('$'))) {
                    QMapIterator<QString, QString> replaceIt(fileContent);
                    while (replaceIt.hasNext()) {
                        replaceIt.next();
                        const QString replaceKey = replaceIt.key();
                        if (replaceKey == key)
                            continue;

                        const QString keyAsWindowsVar = QLatin1Char('%') + replaceKey + QLatin1Char('%');
                        const QString keyAsUnixVar = QLatin1Char('$') + replaceKey;
                        if (val.contains(keyAsWindowsVar))
                            val.replace(keyAsWindowsVar, replaceIt.value());
                        if (val.contains(keyAsUnixVar))
                            val.replace(keyAsUnixVar, replaceIt.value());
                    }
                }
                result.insert(key, val);
            }
        }
    }

    return result;
}
}

BlackBerryQtVersion::BlackBerryQtVersion()
    : QnxAbstractQtVersion()
{
}

BlackBerryQtVersion::BlackBerryQtVersion(QnxArchitecture arch, const Utils::FileName &path, bool isAutoDetected, const QString &autoDetectionSource)
    : QnxAbstractQtVersion(arch, path, isAutoDetected, autoDetectionSource)
{

}

BlackBerryQtVersion::~BlackBerryQtVersion()
{

}

BlackBerryQtVersion *BlackBerryQtVersion::clone() const
{
    return new BlackBerryQtVersion(*this);
}

QString BlackBerryQtVersion::type() const
{
    return QLatin1String(Constants::QNX_BB_QT);
}

QString BlackBerryQtVersion::description() const
{
    return tr("BlackBerry %1", "Qt Version is meant for BlackBerry").arg(archString());
}

QMultiMap<QString, QString> BlackBerryQtVersion::environment() const
{
    QTC_CHECK(!sdkPath().isEmpty());
    if (sdkPath().isEmpty())
        return QMultiMap<QString, QString>();

    QString envFile;
    if (Utils::HostOsInfo::isWindowsHost())
        envFile = sdkPath() + QLatin1String("/bbndk-env.bat");
    else if (Utils::HostOsInfo::isAnyUnixHost())
        envFile = sdkPath() + QLatin1String("/bbndk-env.sh");
    return parseEnvironmentFile(envFile);
}

Core::FeatureSet BlackBerryQtVersion::availableFeatures() const
{
    Core::FeatureSet features = QnxAbstractQtVersion::availableFeatures();
    features |= Core::FeatureSet(Constants::QNX_BB_FEATURE);
    return features;
}

QString BlackBerryQtVersion::platformName() const
{
    return QLatin1String(Constants::QNX_BB_PLATFORM_NAME);
}

QString BlackBerryQtVersion::platformDisplayName() const
{
    return tr("BlackBerry");
}

QString BlackBerryQtVersion::sdkDescription() const
{
    return tr("BlackBerry Native SDK:");
}
