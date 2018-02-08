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

#include "projectpartartefactexception.h"

#include <utils/smallstringvector.h>

#include <compilermacro.h>

QT_FORWARD_DECLARE_CLASS(QJsonDocument)
QT_FORWARD_DECLARE_STRUCT(QJsonParseError)

namespace ClangBackEnd {

class ProjectPartArtefact
{
public:
    ProjectPartArtefact(Utils::SmallStringView compilerArgumentsText,
                        Utils::SmallStringView compilerMacrosText,
                        Utils::SmallStringView includeSearchPaths,
                        int projectPartId);

    static
    Utils::SmallStringVector toStringVector(Utils::SmallStringView jsonText);
    static
    CompilerMacros createCompilerMacrosFromDocument(const QJsonDocument &document);
    static
    CompilerMacros toCompilerMacros(Utils::SmallStringView jsonText);
    static
    QJsonDocument createJsonDocument(Utils::SmallStringView jsonText,
                                     const char *whatError);
    static
    void checkError(const char *whatError, const QJsonParseError &error);
    friend
    bool operator==(const ProjectPartArtefact &first, const ProjectPartArtefact &second);

public:
    Utils::SmallStringVector compilerArguments;
    CompilerMacros compilerMacros;
    Utils::SmallStringVector includeSearchPaths;
    int projectPartId = -1;
};

using ProjectPartArtefacts = std::vector<ProjectPartArtefact>;
} // namespace ClangBackEnd
