/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "projectexplorer_export.h"

#include "project.h"
#include "treescanner.h"

#include <QTimer>

namespace ProjectExplorer {

class BuildConfiguration;
class ExtraCompiler;

// --------------------------------------------------------------------
// BuildSystem:
// --------------------------------------------------------------------

class PROJECTEXPLORER_EXPORT BuildSystem : public QObject
{
    Q_OBJECT

public:
    explicit BuildSystem(Project *project);

    BuildSystem(const BuildSystem &other) = delete;

    Project *project() const;

    bool isWaitingForParse() const;

    void requestParse();
    void requestDelayedParse();

protected:
    class ParsingContext
    {
    public:
        ParsingContext() = default;

        ParsingContext(const ParsingContext &other) = delete;
        ParsingContext &operator=(const ParsingContext &other) = delete;
        ParsingContext(ParsingContext &&other) = default;
        ParsingContext &operator=(ParsingContext &&other) = default;

        Project::ParseGuard guard;

        Project *project = nullptr;
        BuildConfiguration *buildConfiguration = nullptr;
        Utils::MacroExpander *expander = nullptr;
        Utils::Environment environment;

    private:
        ParsingContext(Project::ParseGuard &&g,
                       Project *p,
                       BuildConfiguration *bc,
                       Utils::MacroExpander *e,
                       Utils::Environment &env)
            : guard(std::move(g))
            , project(p)
            , buildConfiguration(bc)
            , expander(e)
            , environment(env)
        {}

        friend class BuildSystem;
    };

    virtual bool validateParsingContext(const ParsingContext &ctx)
    {
        Q_UNUSED(ctx)
        return true;
    }

    virtual void parseProject(ParsingContext &&ctx) = 0; // actual code to parse project

private:
    void requestParse(int delay); // request a (delayed!) parser run.
    void triggerParsing();

    QTimer m_delayedParsingTimer;

    Project *m_project;
};

} // namespace ProjectExplorer
