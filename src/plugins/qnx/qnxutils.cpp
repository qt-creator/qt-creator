/**************************************************************************
**
** Copyright (C) 2011 - 2013 Research In Motion
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

#include "qnxutils.h"
#include "qnxabstractqtversion.h"

#include <utils/hostosinfo.h>

#include <QDir>

using namespace Qnx;
using namespace Qnx::Internal;

QString QnxUtils::addQuotes(const QString &string)
{
    return QLatin1Char('"') + string + QLatin1Char('"');
}

Qnx::QnxArchitecture QnxUtils::cpudirToArch(const QString &cpuDir)
{
    if (cpuDir == QLatin1String("x86"))
        return Qnx::X86;
    else if (cpuDir == QLatin1String("armle-v7"))
        return Qnx::ArmLeV7;
    else
        return Qnx::UnknownArch;
}

QStringList QnxUtils::searchPaths(QnxAbstractQtVersion *qtVersion)
{
    const QDir pluginDir(qtVersion->versionInfo().value(QLatin1String("QT_INSTALL_PLUGINS")));
    const QStringList pluginSubDirs = pluginDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    QStringList searchPaths;

    Q_FOREACH (const QString &dir, pluginSubDirs) {
        searchPaths << qtVersion->versionInfo().value(QLatin1String("QT_INSTALL_PLUGINS"))
                       + QLatin1Char('/') + dir;
    }

    searchPaths << qtVersion->versionInfo().value(QLatin1String("QT_INSTALL_LIBS"));
    searchPaths << qtVersion->qnxTarget() + QLatin1Char('/') + qtVersion->archString().toLower()
                   + QLatin1String("/lib");
    searchPaths << qtVersion->qnxTarget() + QLatin1Char('/') + qtVersion->archString().toLower()
                   + QLatin1String("/usr/lib");

    return searchPaths;
}

QMultiMap<QString, QString> QnxUtils::parseEnvironmentFile(const QString &fileName)
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

        int equalIndex = line.indexOf(QLatin1Char('='));
        QString var = line.left(equalIndex);
        //Remove set in front
        if (var.startsWith(QLatin1String("set ")))
            var = var.right(var.size() - 4);

        QString value = line.mid(equalIndex + 1);

        if (Utils::HostOsInfo::isWindowsHost()) {
            QRegExp systemVarRegExp(QLatin1String("IF NOT DEFINED ([\\w\\d]+)\\s+set ([\\w\\d]+)=([\\w\\d]+)"));
            if (line.contains(systemVarRegExp)) {
                var = systemVarRegExp.cap(2);
                Utils::Environment sysEnv = Utils::Environment::systemEnvironment();
                QString sysVar = systemVarRegExp.cap(1);
                if (sysEnv.hasKey(sysVar))
                    value = sysEnv.value(sysVar);
                else
                    value = systemVarRegExp.cap(3);
            }
        }
        else if (Utils::HostOsInfo::isAnyUnixHost()) {
            QRegExp systemVarRegExp(QLatin1String("\\$\\{([\\w\\d]+):=([\\w\\d]+)\\}")); // to match e.g. "${QNX_HOST_VERSION:=10_0_9_52}"
            if (value.contains(systemVarRegExp)) {
                Utils::Environment sysEnv = Utils::Environment::systemEnvironment();
                QString sysVar = systemVarRegExp.cap(1);
                if (sysEnv.hasKey(sysVar))
                    value = sysEnv.value(sysVar);
                else
                    value = systemVarRegExp.cap(2);
            }
        }

        if (value.startsWith(QLatin1Char('"')))
            value = value.mid(1);
        if (value.endsWith(QLatin1Char('"')))
            value = value.left(value.size() - 1);

        fileContent[var] = value;
    }
    file.close();

    QMapIterator<QString, QString> it(fileContent);
    while (it.hasNext()) {
        it.next();
        QStringList values;
        if (Utils::HostOsInfo::isWindowsHost())
            values = it.value().split(QLatin1Char(';'));
        else if (Utils::HostOsInfo::isAnyUnixHost())
            values = it.value().split(QLatin1Char(':'));

        QString key = it.key();
        foreach (const QString &value, values) {
            const QString ownKeyAsWindowsVar = QLatin1Char('%') + key + QLatin1Char('%');
            const QString ownKeyAsUnixVar = QLatin1Char('$') + key;
            if (value != ownKeyAsUnixVar && value != ownKeyAsWindowsVar) { // to ignore e.g. PATH=$PATH
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

    if (!result.contains(QLatin1String("CPUVARDIR")))
            result.insert(QLatin1String("CPUVARDIR"), QLatin1String("armle-v7"));

    return result;
}

bool QnxUtils::isValidNdkPath(const QString &ndkPath)
{
    return (QFileInfo(envFilePath(ndkPath)).exists());
}

QString QnxUtils::envFilePath(const QString &ndkPath)
{
    QString envFile;
    if (Utils::HostOsInfo::isWindowsHost())
        envFile = ndkPath + QLatin1String("/bbndk-env.bat");
    else if (Utils::HostOsInfo::isAnyUnixHost())
        envFile = ndkPath + QLatin1String("/bbndk-env.sh");

    return envFile;
}

void QnxUtils::prependQnxMapToEnvironment(const QMultiMap<QString, QString> &qnxMap, Utils::Environment &env)
{
    QMultiMap<QString, QString>::const_iterator it;
    QMultiMap<QString, QString>::const_iterator end(qnxMap.constEnd());
    for (it = qnxMap.constBegin(); it != end; ++it) {
        const QString key = it.key();
        const QString value = it.value();

        if (key == QLatin1String("PATH"))
            env.prependOrSetPath(value);
        else if (key == QLatin1String("LD_LIBRARY_PATH"))
            env.prependOrSetLibrarySearchPath(value);
        else
            env.set(key, value);
    }
}

Utils::FileName QnxUtils::executableWithExtension(const Utils::FileName &fileName)
{
    Utils::FileName result = fileName;
    if (Utils::HostOsInfo::isWindowsHost())
        result.append(QLatin1String(".exe"));
    return result;
}
