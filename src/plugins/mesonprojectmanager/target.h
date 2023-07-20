// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/algorithm.h>
#include <utils/fileutils.h>

#include <QDir>
#include <QVariant>

#include <optional>

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
    const QStringList extraFiles;
    const std::optional<QString> subproject;
    const SourceGroupList sources;

    static inline QString fullName(const Utils::FilePath &buildDir, const Target &target)
    {
        using namespace Utils;
        auto fname = target.fileName.first();
        if (FilePath::fromString(fname).isAbsolutePath()) {
            fname.remove(buildDir.toString());
            if (fname.startsWith('/'))
                fname.remove(0, 1);
            return fname;
        } else {
            return fname;
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
           QStringList &&extraFiles,
           QString &&subproject,
           SourceGroupList &&sources)
        : type{toType(type)}
        , name{std::move(name)}
        , id{std::move(id)}
        , definedIn{QDir::cleanPath(definedIn)}
        , fileName{cleanPath(std::move(fileName))}
        , extraFiles{cleanPath(std::move(extraFiles))}
        , subproject{subproject.isNull() ? std::nullopt
                                         : std::optional<QString>{std::move(subproject)}}
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
