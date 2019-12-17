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

#include "fileutils.h"
#include "hostosinfo.h"
#include "namevaluedictionary.h"
#include "namevalueitem.h"
#include "optional.h"

#include <QMap>
#include <QStringList>

#include <functional>

QT_FORWARD_DECLARE_CLASS(QDebug)
QT_FORWARD_DECLARE_CLASS(QProcessEnvironment)

namespace Utils {

class QTCREATOR_UTILS_EXPORT Environment final : public NameValueDictionary
{
public:
    using NameValueDictionary::NameValueDictionary;

    static Environment systemEnvironment();
    static void setupEnglishOutput(Environment *environment);
    static void setupEnglishOutput(QProcessEnvironment *environment);
    static void setupEnglishOutput(QStringList *environment);

    QProcessEnvironment toProcessEnvironment() const;

    void appendOrSet(const QString &key, const QString &value, const QString &sep = QString());
    void prependOrSet(const QString &key, const QString &value, const QString &sep = QString());

    void appendOrSetPath(const QString &value);
    void prependOrSetPath(const QString &value);

    void prependOrSetLibrarySearchPath(const QString &value);
    void prependOrSetLibrarySearchPaths(const QStringList &values);

    using PathFilter = std::function<bool(const FilePath &)>;
    FilePath searchInPath(const QString &executable,
                          const FilePaths &additionalDirs = FilePaths(),
                          const PathFilter &func = PathFilter()) const;
    FilePaths findAllInPath(const QString &executable,
                               const FilePaths &additionalDirs = FilePaths(),
                               const PathFilter &func = PathFilter()) const;

    FilePaths path() const;
    FilePaths pathListValue(const QString &varName) const;
    QStringList appendExeExtensions(const QString &executable) const;

    bool isSameExecutable(const QString &exe1, const QString &exe2) const;

    QString expandedValueForKey(const QString &key) const;
    QString expandVariables(const QString &input) const;
    FilePath expandVariables(const FilePath &input) const;
    QStringList expandVariables(const QStringList &input) const;

    static void modifySystemEnvironment(const EnvironmentItems &list); // use with care!!!

private:
    FilePath searchInDirectory(const QStringList &execs, const FilePath &directory,
                               QSet<FilePath> &alreadyChecked) const;
};

class QTCREATOR_UTILS_EXPORT EnvironmentProvider
{
public:
    QByteArray id;
    QString displayName;
    std::function<Environment()> environment;

    static void addProvider(EnvironmentProvider &&provider);
    static const QVector<EnvironmentProvider> providers();
    static optional<EnvironmentProvider> provider(const QByteArray &id);
};

} // namespace Utils
