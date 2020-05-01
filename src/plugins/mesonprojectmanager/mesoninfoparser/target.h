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
#include <utils/fileutils.h>
#include <utils/optional.h>
#include <QString>
#include <QVariant>

namespace MesonProjectManager {
namespace Internal {

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
            , sources{std::move(sources)}
            , generatedSources{std::move(generatedSources)}
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

    static inline QString fullName(const Target &target)
    {
        // TODO, this is bad, might be moved in a place where src dir is known
        if (target.fileName.first().startsWith("/")) {
            auto fname = target.fileName.first().split('/');
            auto definedIn = target.definedIn.split('/');
            definedIn.pop_back();
            int i = std::min(definedIn.length(), fname.length()) - 1;
            for (; i >= 0 && fname[i] == definedIn[i]; --i)
                ;
            return fname.mid(i + 1).join("/");
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
        , definedIn{std::move(definedIn)}
        , fileName{std::move(fileName)}
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
