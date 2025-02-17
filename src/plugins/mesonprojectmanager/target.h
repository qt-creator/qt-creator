// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/algorithm.h>
#include <utils/filepath.h>

#include <QDir>
#include <QVariant>

#include <optional>

namespace MesonProjectManager::Internal {

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
    const bool buildByDefault;

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

    static QString typeToString(const Type type)
    {
        switch (type) {
        case Type::executable:
            return QStringLiteral("executable");
        case Type::staticLibrary:
            return QStringLiteral("static library");
        case Type::sharedLibrary:
            return QStringLiteral("shared library");
        case Type::sharedModule:
            return QStringLiteral("shared module");
        case Type::custom:
            return QStringLiteral("custom");
        case Type::run:
            return QStringLiteral("run");
        case Type::jar:
            return QStringLiteral("jar");
        case Type::unknown:
            return QStringLiteral("unknown");
        }
        return QStringLiteral("unknown");
    }

    inline static QString unique_name(const Target &target, const Utils::FilePath &projectDir)
    {
        auto relative_path = Utils::FilePath::fromString(target.definedIn).canonicalPath().relativeChildPath(projectDir.canonicalPath()).parentDir();
        if (target.type == Type::sharedModule)
            return relative_path.pathAppended(Utils::FilePath::fromString(target.fileName[0]).fileName()).toUrlishString();
        return relative_path.pathAppended(target.name).toUrlishString();
    }

    Target(const QString &type,
           QString &&name,
           QString &&id,
           QString &&definedIn,
           QStringList &&fileName,
           QStringList &&extraFiles,
           QString &&subproject,
           SourceGroupList &&sources,
           bool buildByDefault)
        : type{toType(type)}
        , name{std::move(name)}
        , id{std::move(id)}
        , definedIn{QDir::cleanPath(definedIn)}
        , fileName{cleanPath(std::move(fileName))}
        , extraFiles{cleanPath(std::move(extraFiles))}
        , subproject{subproject.isNull() ? std::nullopt
                                         : std::optional<QString>{std::move(subproject)}}
        , sources{std::move(sources)}
        , buildByDefault{buildByDefault}
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

} // namespace MesonProjectManager::Internal
