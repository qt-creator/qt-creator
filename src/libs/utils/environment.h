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

#include "utils_global.h"

#include "environmentfwd.h"
#include "filepath.h"
#include "namevaluedictionary.h"
#include "optional.h"

#include <functional>

QT_FORWARD_DECLARE_CLASS(QProcessEnvironment)

namespace Utils {

class QTCREATOR_UTILS_EXPORT Environment final : public NameValueDictionary
{
public:
    using NameValueDictionary::NameValueDictionary;

    static Environment systemEnvironment();

    QProcessEnvironment toProcessEnvironment() const;

    void appendOrSet(const QString &key, const QString &value, const QString &sep = QString());
    void prependOrSet(const QString &key, const QString &value, const QString &sep = QString());

    void appendOrSetPath(const FilePath &value);
    void prependOrSetPath(const FilePath &value);

    void prependOrSetLibrarySearchPath(const FilePath &value);
    void prependOrSetLibrarySearchPaths(const FilePaths &values);

    void setupEnglishOutput();

    using PathFilter = std::function<bool(const FilePath &)>;
    FilePath searchInPath(const QString &executable,
                          const FilePaths &additionalDirs = FilePaths(),
                          const PathFilter &func = PathFilter()) const;
    FilePath searchInDirectories(const QString &executable,
                                 const FilePaths &dirs) const;
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
    static void setSystemEnvironment(const Environment &environment);  // don't use at all!!!
};

class QTCREATOR_UTILS_EXPORT EnvironmentChange final
{
public:
    using Item = std::function<void(Environment &)>;

    EnvironmentChange() = default;

    static EnvironmentChange fromFixedEnvironment(const Environment &fixedEnv);

    void applyToEnvironment(Environment &) const;

    void addSetValue(const QString &key, const QString &value);
    void addUnsetValue(const QString &key);
    void addPrependToPath(const FilePaths &values);
    void addAppendToPath(const FilePaths &values);
    void addModify(const NameValueItems &items);
    void addChange(const Item &item) { m_changeItems.append(item); }

private:
    QList<Item> m_changeItems;
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

Q_DECLARE_METATYPE(Utils::Environment)
