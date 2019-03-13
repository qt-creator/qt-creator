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

#include "projectpartartefact.h"

#include <utils/algorithm.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace ClangBackEnd {



Utils::SmallStringVector ProjectPartArtefact::toStringVector(Utils::SmallStringView jsonText)
{
    if (jsonText.isEmpty())
        return {};

    QJsonDocument document = createJsonDocument(jsonText, "Compiler arguments parsing error");

    return Utils::transform<Utils::SmallStringVector>(document.array(), [] (const QJsonValue &value) {
        return Utils::SmallString{value.toString()};
    });
}

CompilerMacros ProjectPartArtefact::createCompilerMacrosFromDocument(const QJsonDocument &document)
{
    QJsonArray array = document.array();
    CompilerMacros macros;
    macros.reserve(array.size());

    for (const QJsonValueRef entry : array) {
        const QJsonArray entryArray = entry.toArray();
        macros.emplace_back(
            entryArray[0].toString(), entryArray[1].toString(), entryArray[2].toInt());
    }

    std::sort(macros.begin(), macros.end());

    return macros;
}

IncludeSearchPaths ProjectPartArtefact::createIncludeSearchPathsFromDocument(const QJsonDocument &document)
{
    QJsonArray array = document.array();
    IncludeSearchPaths paths;
    paths.reserve(array.size());

    for (const QJsonValueRef entry : array) {
        const QJsonArray entryArray = entry.toArray();
        paths.emplace_back(entryArray[0].toString(), entryArray[1].toInt(), entryArray[2].toInt());
    }

    return paths;
}

CompilerMacros ProjectPartArtefact::toCompilerMacros(Utils::SmallStringView jsonText)
{
    if (jsonText.isEmpty())
        return {};

    QJsonDocument document = createJsonDocument(jsonText, "Compiler macros parsing error");

    return createCompilerMacrosFromDocument(document);
}

QJsonDocument ProjectPartArtefact::createJsonDocument(Utils::SmallStringView jsonText,
                                                      const char *whatError)
{
    QJsonParseError error;
    QJsonDocument document = QJsonDocument::fromJson(QByteArray::fromRawData(jsonText.data(),
                                                                             jsonText.size()),
                                                     &error);
    checkError(whatError, error);

    return document;
}

IncludeSearchPaths ProjectPartArtefact::toIncludeSearchPaths(Utils::SmallStringView jsonText)
{

    if (jsonText.isEmpty())
        return {};

    QJsonDocument document = createJsonDocument(jsonText, "Include search paths parsing error");

    return createIncludeSearchPathsFromDocument(document);
}

void ProjectPartArtefact::checkError(const char *whatError, const QJsonParseError &error)
{
    if (error.error != QJsonParseError::NoError) {
        throw ProjectPartArtefactParseError(whatError,
                                            error.errorString());
    }
}

bool operator==(const ProjectPartArtefact &first, const ProjectPartArtefact &second)
{
    return first.toolChainArguments == second.toolChainArguments
           && first.compilerMacros == second.compilerMacros
           && first.systemIncludeSearchPaths == second.systemIncludeSearchPaths
           && first.projectIncludeSearchPaths == second.projectIncludeSearchPaths;
}

} // namespace ClangBackEnd
