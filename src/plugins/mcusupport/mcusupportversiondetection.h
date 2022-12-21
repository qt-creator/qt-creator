// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>
#include <QString>

namespace McuSupport::Internal {

class McuPackageVersionDetector
{
public:
    McuPackageVersionDetector() = default;
    virtual ~McuPackageVersionDetector() = default;
    virtual QString parseVersion(const Utils::FilePath &packagePath) const = 0;
};

// Get version from the output of an executable
class McuPackageExecutableVersionDetector : public McuPackageVersionDetector
{
public:
    McuPackageExecutableVersionDetector(const Utils::FilePath &detectionPath,
                                        const QStringList &detectionArgs,
                                        const QString &detectionRegExp);
    QString parseVersion(const Utils::FilePath &packagePath) const final;

private:
    const Utils::FilePath m_detectionPath;
    const QStringList m_detectionArgs;
    const QString m_detectionRegExp;
};

// Get version through parsing an XML file
class McuPackageXmlVersionDetector : public McuPackageVersionDetector
{
public:
    McuPackageXmlVersionDetector(const QString &filePattern,
                                 const QString &elementName,
                                 const QString &versionAttribute,
                                 const QString &versionRegExp);
    QString parseVersion(const Utils::FilePath &packagePath) const final;

private:
    const QString m_filePattern;
    const QString m_versionElement;
    const QString m_versionAttribute;
    const QString m_versionRegExp;
};

// Get version from the filename of a given file/dir in the package directory
class McuPackageDirectoryEntriesVersionDetector : public McuPackageVersionDetector
{
public:
    McuPackageDirectoryEntriesVersionDetector(const QString &filePattern,
                                              const QString &versionRegExp);
    QString parseVersion(const Utils::FilePath &packagePath) const final;

private:
    const QString m_filePattern;
    const QString m_versionRegExp;
};

// Get version from the path of the package itself
class McuPackagePathVersionDetector : public McuPackageVersionDetector
{
public:
    McuPackagePathVersionDetector(const QString &versionRegExp);
    QString parseVersion(const Utils::FilePath &packagePath) const final;

private:
    const QString m_versionRegExp;
};

} // namespace McuSupport::Internal
