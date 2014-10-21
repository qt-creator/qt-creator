/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "localapplicationrunconfiguration.h"

#include "buildconfiguration.h"

#include <utils/macroexpander.h>

#include <projectexplorer/target.h>
#include <projectexplorer/project.h>

#include <QDir>

namespace ProjectExplorer {
namespace Internal {

class FallBackMacroExpander : public Utils::MacroExpander
{
public:
    explicit FallBackMacroExpander(const Target *target) : m_target(target) {}
    virtual bool resolveMacro(const QString &name, QString *ret) const;
private:
    const Target *m_target;
};

bool FallBackMacroExpander::resolveMacro(const QString &name, QString *ret) const
{
    if (name == QLatin1String("sourceDir")) {
        *ret = m_target->project()->projectDirectory().toUserOutput();
        return true;
    }
    return false;
}
} // namespace Internal

/// LocalApplicationRunConfiguration

LocalApplicationRunConfiguration::LocalApplicationRunConfiguration(Target *target, Core::Id id) :
    RunConfiguration(target, id), m_macroExpander(0)
{ }

LocalApplicationRunConfiguration::LocalApplicationRunConfiguration(Target *target, LocalApplicationRunConfiguration *rc) :
    RunConfiguration(target, rc), m_macroExpander(0)
{ }

LocalApplicationRunConfiguration::~LocalApplicationRunConfiguration()
{
    delete m_macroExpander;
}

void LocalApplicationRunConfiguration::addToBaseEnvironment(Utils::Environment &env) const
{
    Q_UNUSED(env);
}

Utils::MacroExpander *LocalApplicationRunConfiguration::macroExpander() const
{
    if (BuildConfiguration *bc = activeBuildConfiguration())
        return bc->macroExpander();
    if (!m_macroExpander)
        m_macroExpander = new Internal::FallBackMacroExpander(target());
    return m_macroExpander;
}

} // namespace ProjectExplorer
