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

#pragma once

#include <QObject>
#include <utils/filepath.h>

namespace McuSupport {
namespace Internal {

class McuPackageVersionDetector : public QObject
{
    Q_OBJECT
public:
    McuPackageVersionDetector();
    virtual ~McuPackageVersionDetector() = default;
    virtual QString parseVersion(const QString &packagePath) const = 0;
};

// Get version from the output of an executable
class McuPackageExecutableVersionDetector : public McuPackageVersionDetector
{
public:
    McuPackageExecutableVersionDetector(const Utils::FilePath &detectionPath,
                                        const QStringList &detectionArgs,
                                        const QString &detectionRegExp);
    QString parseVersion(const QString &packagePath) const final;

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
    QString parseVersion(const QString &packagePath) const final;

private:
    const QString m_filePattern;
    const QString m_versionElement;
    const QString m_versionAttribute;
    const QString m_versionRegExp;
};

// Get version from the filename of a given file/dir in the package directory
class McuPackageDirectoryVersionDetector : public McuPackageVersionDetector
{
public:
    McuPackageDirectoryVersionDetector(const QString &filePattern,
                                       const QString &versionRegExp,
                                       const bool isFile);
    QString parseVersion(const QString &packagePath) const final;

private:
    const QString m_filePattern;
    const QString m_versionRegExp;
    const bool m_isFile;
};

// Get version from the path of the package itself
class McuPackagePathVersionDetector : public McuPackageVersionDetector
{
public:
    McuPackagePathVersionDetector(const QString &versionRegExp);
    QString parseVersion(const QString &packagePath) const final;

private:
    const QString m_versionRegExp;
};

} // namespace Internal
} // namespace McuSupport
