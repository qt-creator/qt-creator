/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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
#include <utils/smallstringvector.h>

#include <compilermacro.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace ClangBackEnd {

class ProjectPartArtefact
{
public:
    ProjectPartArtefact(Utils::SmallStringView compilerArgumentsText,
                        Utils::SmallStringView compilerMacrosText,
                        int projectPartId)
        : compilerArguments(toStringVector(compilerArgumentsText)),
          compilerMacros(toCompilerMacros(compilerMacrosText)),
          projectPartId(projectPartId)
    {
    }

    static
    Utils::SmallStringVector toStringVector(Utils::SmallStringView jsonText)
    {
        QJsonDocument document = QJsonDocument::fromJson(QByteArray::fromRawData(jsonText.data(),
                                                                                 jsonText.size()));

        return Utils::transform<Utils::SmallStringVector>(document.array(), [] (const QJsonValue &value) {
            return Utils::SmallString{value.toString()};
        });
    }

    static
    CompilerMacros toCompilerMacros(Utils::SmallStringView jsonText)
    {
        QJsonDocument document = QJsonDocument::fromJson(QByteArray::fromRawData(jsonText.data(),
                                                                                 jsonText.size()));
        QJsonObject object = document.object();
        CompilerMacros macros;
        macros.reserve(object.size());

        for (auto current = object.constBegin(); current != object.constEnd(); ++current)
            macros.emplace_back(current.key(), current.value().toString());

        return macros;
    }

    friend
    bool operator==(const ProjectPartArtefact &first, const ProjectPartArtefact &second)
    {
        return first.compilerArguments == second.compilerArguments
            && first.compilerMacros == second.compilerMacros;
    }

public:
    Utils::SmallStringVector compilerArguments;
    CompilerMacros compilerMacros;
    int projectPartId = -1;
};

using ProjectPartArtefacts = std::vector<ProjectPartArtefact>;
}
