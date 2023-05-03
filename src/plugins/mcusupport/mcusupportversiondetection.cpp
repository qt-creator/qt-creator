// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mcusupportversiondetection.h"

#include <utils/process.h>

#include <QDir>
#include <QRegularExpression>
#include <QXmlStreamReader>

using namespace Utils;

namespace McuSupport {
namespace Internal {

QString matchRegExp(const QString &text, const QString &regExp)
{
    const QRegularExpression regularExpression(regExp);
    const QRegularExpressionMatch match = regularExpression.match(text);
    if (match.hasMatch())
        return match.captured(regularExpression.captureCount());
    return QString();
}

McuPackageExecutableVersionDetector::McuPackageExecutableVersionDetector(
    const FilePath &detectionPath, const QStringList &detectionArgs, const QString &detectionRegExp)
    : McuPackageVersionDetector()
    , m_detectionPath(detectionPath)
    , m_detectionArgs(detectionArgs)
    , m_detectionRegExp(detectionRegExp)
{}

QString McuPackageExecutableVersionDetector::parseVersion(const FilePath &packagePath) const
{
    if (m_detectionPath.isEmpty() || m_detectionRegExp.isEmpty())
        return {};

    const FilePath binaryPath = packagePath / m_detectionPath.path();
    if (!binaryPath.exists())
        return {};

    const int timeout = 3000; // usually runs below 1s, but we want to be on the safe side
    Process process;
    process.setCommand({binaryPath, m_detectionArgs});
    process.start();
    if (!process.waitForFinished(timeout) || process.result() != ProcessResult::FinishedWithSuccess)
        return {};

    return matchRegExp(process.allOutput(), m_detectionRegExp);
}

McuPackageXmlVersionDetector::McuPackageXmlVersionDetector(const QString &filePattern,
                                                           const QString &versionElement,
                                                           const QString &versionAttribute,
                                                           const QString &versionRegExp)
    : m_filePattern(filePattern)
    , m_versionElement(versionElement)
    , m_versionAttribute(versionAttribute)
    , m_versionRegExp(versionRegExp)
{}

QString McuPackageXmlVersionDetector::parseVersion(const FilePath &packagePath) const
{
    const auto files = QDir(packagePath.toString(), m_filePattern).entryInfoList();
    for (const auto &xmlFile : files) {
        QFile sdkXmlFile = QFile(xmlFile.absoluteFilePath());
        sdkXmlFile.open(QFile::OpenModeFlag::ReadOnly);
        QXmlStreamReader xmlReader(&sdkXmlFile);
        while (xmlReader.readNext()) {
            if (xmlReader.name() == m_versionElement) {
                const QString versionString
                    = xmlReader.attributes().value(m_versionAttribute).toString();
                const QString matched = matchRegExp(versionString, m_versionRegExp);
                return !matched.isEmpty() ? matched : versionString;
            }
        }
    }

    return QString();
}

McuPackageDirectoryEntriesVersionDetector::McuPackageDirectoryEntriesVersionDetector(
    const QString &filePattern, const QString &versionRegExp)
    : m_filePattern(filePattern)
    , m_versionRegExp(versionRegExp)
{}

QString McuPackageDirectoryEntriesVersionDetector::parseVersion(const FilePath &packagePath) const
{
    const auto files = QDir(packagePath.toString(), m_filePattern).entryInfoList();
    for (const auto &entry : files) {
        const QString matched = matchRegExp(entry.fileName(), m_versionRegExp);
        if (!matched.isEmpty())
            return matched;
    }
    return QString();
}

McuPackagePathVersionDetector::McuPackagePathVersionDetector(const QString &versionRegExp)
    : m_versionRegExp(versionRegExp)
{}

QString McuPackagePathVersionDetector::parseVersion(const FilePath &packagePath) const
{
    if (!packagePath.exists())
        return {};
    return matchRegExp(packagePath.toString(), m_versionRegExp);
}

} // namespace Internal
} // namespace McuSupport
