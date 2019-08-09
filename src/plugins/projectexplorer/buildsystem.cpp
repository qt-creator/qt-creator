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

#include "buildsystem.h"

#include "buildconfiguration.h"
#include "target.h"

#include <utils/qtcassert.h>

using namespace Utils;

namespace ProjectExplorer {

// --------------------------------------------------------------------
// BuildSystem:
// --------------------------------------------------------------------

BuildSystem::BuildSystem(Project *project)
    : m_project(project)
{
    QTC_CHECK(project);

    // Timer:
    m_delayedParsingTimer.setSingleShot(true);

    connect(&m_delayedParsingTimer, &QTimer::timeout, this, &BuildSystem::triggerParsing);
}

Project *BuildSystem::project() const
{
    return m_project;
}

bool BuildSystem::isWaitingForParse() const
{
    return m_delayedParsingTimer.isActive();
}

void BuildSystem::requestParse()
{
    requestParse(0);
}

void BuildSystem::requestDelayedParse()
{
    requestParse(1000);
}

void BuildSystem::requestParse(int delay)
{
    m_delayedParsingTimer.setInterval(delay);
    m_delayedParsingTimer.start();
}

void BuildSystem::triggerParsing()
{
    QTC_CHECK(!project()->isParsing());

    Project *p = project();
    Target *t = p->activeTarget();
    BuildConfiguration *bc = t ? t->activeBuildConfiguration() : nullptr;

    MacroExpander *e = nullptr;
    if (bc)
        e = bc->macroExpander();
    else if (t)
        e = t->macroExpander();
    else
        e = p->macroExpander();

    Utils::Environment env = p->activeParseEnvironment();

    ParsingContext ctx(p->guardParsingRun(), p, bc, e, env);

    if (validateParsingContext(ctx))
        parseProject(std::move(ctx));
}

} // namespace ProjectExplorer
