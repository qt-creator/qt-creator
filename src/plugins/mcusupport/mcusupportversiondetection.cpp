/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "mcusupportversiondetection.h"

#include <utils/fileutils.h>
#include <QDir>
#include <QRegularExpression>
#include <QProcess>

namespace McuSupport {
namespace Internal {

McuPackageVersionDetector::McuPackageVersionDetector()
{
}

McuPackageExecutableVersionDetector::McuPackageExecutableVersionDetector(
        const QString &detectionPath,
        const QStringList &detectionArgs,
        const QString &detectionRegExp)
    : McuPackageVersionDetector()
    , m_detectionPath(detectionPath)
    , m_detectionArgs(detectionArgs)
    , m_detectionRegExp(detectionRegExp)
{
}

QString McuPackageExecutableVersionDetector::parseVersion(const QString &packagePath) const
{
    if (m_detectionPath.isEmpty() || m_detectionRegExp.isEmpty())
        return QString();

    QString binaryPath = QDir::toNativeSeparators(packagePath + "/" + m_detectionPath);
    if (!Utils::FilePath::fromString(binaryPath).exists())
        return QString();

    const QRegularExpression regExp(m_detectionRegExp);

    const int execTimeout = 3000; // usually runs below 1s, but we want to be on the safe side
    QProcess binaryProcess;
    binaryProcess.start(binaryPath, m_detectionArgs);
    if (!binaryProcess.waitForStarted())
        return QString();
    binaryProcess.waitForFinished(execTimeout);
    if (binaryProcess.exitCode() == QProcess::ExitStatus::NormalExit) {
        const QString processOutput = QString::fromUtf8(
                    binaryProcess.readAllStandardOutput().append(
                        binaryProcess.readAllStandardError()));
        const QRegularExpressionMatch match = regExp.match(processOutput);
        if (match.hasMatch())
            return match.captured(regExp.captureCount());
    }

    // Fail gracefully: return empty string if execution failed or regexp did not match
    return QString();
}

McuPackageXmlVersionDetector::McuPackageXmlVersionDetector(const QString &filePattern,
                                                           const QString &versionElement,
                                                           const QString &versionAttribute,
                                                           const QString &versionRegExp)
    : m_filePattern(filePattern),
      m_versionElement(versionElement),
      m_versionAttribute(versionAttribute),
      m_versionRegExp(versionRegExp)
{
}

QString McuPackageXmlVersionDetector::parseVersion(const QString &packagePath) const
{
    const QRegularExpression regExp(m_versionRegExp);
    const auto files = QDir(packagePath, m_filePattern).entryInfoList();
    for (const auto &xmlFile: files) {
        QFile sdkXmlFile = QFile(xmlFile.absoluteFilePath());
        sdkXmlFile.open(QFile::OpenModeFlag::ReadOnly);
        QXmlStreamReader xmlReader(&sdkXmlFile);
        while (xmlReader.readNext()) {
            if (xmlReader.name() == m_versionElement) {
                const QString versionString = xmlReader.attributes().value(m_versionAttribute).toString();
                const QRegularExpressionMatch match = regExp.match(versionString);
                return match.hasMatch() ? match.captured(regExp.captureCount()) : versionString;
            }
        }
    }

    return QString();
}

McuPackageDirectoryVersionDetector::McuPackageDirectoryVersionDetector(const QString &filePattern,
                                                                       const QString &versionRegExp,
                                                                       const bool isFile)
    : m_filePattern(filePattern),
      m_versionRegExp(versionRegExp),
      m_isFile(isFile)
{
}

QString McuPackageDirectoryVersionDetector::parseVersion(const QString &packagePath) const
{
    const auto files = QDir(packagePath, m_filePattern)
            .entryInfoList(m_isFile ? QDir::Filter::Files : QDir::Filter::Dirs);
    for (const auto &entry: files) {
        const QRegularExpression regExp(m_versionRegExp);
        const QRegularExpressionMatch match = regExp.match(entry.fileName());
        if (match.hasMatch())
            return match.captured(regExp.captureCount());
    }
    return QString();
}

} // Internal
} // McuSupport
