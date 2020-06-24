/****************************************************************************
**
** Copyright (C) 2020 Alexis Jeandet.
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
#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/optional.h>
#include <QDir>
#include <QString>
#include <QVariant>

namespace MesonProjectManager {
namespace Internal {

inline QStringList cleanPath(QStringList &&paths)
{
    return Utils::transform(paths, QDir::cleanPath);
}

struct Target
{
    enum class Type {
        executable,
        run,
        custom,
        sharedLibrary,
        sharedModule,
        staticLibrary,
        jar,
        unknown
    };
    struct SourceGroup
    {
        const QString language;
        const QStringList compiler;
        const QStringList parameters;
        const QStringList sources;
        const QStringList generatedSources;
        SourceGroup(QString &&language,
                    QStringList &&compiler,
                    QStringList &&parameters,
                    QStringList &&sources,
                    QStringList &&generatedSources)
            : language{std::move(language)}
            , compiler{std::move(compiler)}
            , parameters{std::move(parameters)}
            , sources{cleanPath(std::move(sources))}
            , generatedSources{cleanPath(std::move(generatedSources))}
        {}
    };
    using SourceGroupList = std::vector<SourceGroup>;
    const Type type;
    const QString name;
    const QString id;
    const QString definedIn;
    const QStringList fileName;
    const Utils::optional<QString> subproject;
    const SourceGroupList sources;

    static inline QString fullName(const Utils::FilePath &srcDir, const Target &target)
    {
        using namespace Utils;
        if (FileUtils::isAbsolutePath(target.fileName.first())) {
            const auto fname = target.fileName.first().split('/').last();
            QString definedIn = FilePath::fromString(target.definedIn).absolutePath().toString();
            return definedIn.remove(srcDir.toString()) + '/' + fname;
        } else {
            return target.fileName.first();
        }
    }

    static Type toType(const QString &typeStr)
    {
        if (typeStr == "executable")
            return Type::executable;
        if (typeStr == "static library")
            return Type::staticLibrary;
        if (typeStr == "shared library")
            return Type::sharedLibrary;
        if (typeStr == "shared module")
            return Type::sharedModule;
        if (typeStr == "custom")
            return Type::custom;
        if (typeStr == "run")
            return Type::run;
        if (typeStr == "jar")
            return Type::jar;
        return Type::unknown;
    }

    Target(const QString &type,
           QString &&name,
           QString &&id,
           QString &&definedIn,
           QStringList &&fileName,
           QString &&subproject,
           SourceGroupList &&sources)
        : type{toType(type)}
        , name{std::move(name)}
        , id{std::move(id)}
        , definedIn{QDir::cleanPath(definedIn)}
        , fileName{cleanPath(std::move(fileName))}
        , subproject{subproject.isNull() ? Utils::nullopt
                                         : Utils::optional<QString>{std::move(subproject)}}
        , sources{std::move(sources)}
    {}
};

using TargetsList = std::vector<Target>;

template<class function>
void for_each_source_group(const TargetsList &targets, const function &f)
{
    for (const Target &target : targets) {
        for (const Target::SourceGroup &group : target.sources) {
            f(target, group);
        }
    }
}

} // namespace Internal
} // namespace MesonProjectManager
