/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "localapplicationrunconfiguration.h"

#include "buildconfiguration.h"
#include "localenvironmentaspect.h"

#include <utils/stringutils.h>
#include <coreplugin/variablemanager.h>
#include <projectexplorer/target.h>
#include <projectexplorer/project.h>

#include <QDir>


namespace ProjectExplorer {


namespace Internal {
class FallBackMacroExpander : public Utils::AbstractQtcMacroExpander {
public:
    explicit FallBackMacroExpander(const Target *target) : m_target(target) {}
    virtual bool resolveMacro(const QString &name, QString *ret);
private:
    const Target *m_target;
};

bool FallBackMacroExpander::resolveMacro(const QString &name, QString *ret)
{
    if (name == QLatin1String("sourceDir")) {
        *ret = QDir::toNativeSeparators(m_target->project()->projectDirectory());
        return true;
    }
    *ret = Core::VariableManager::value(name.toUtf8());
    return !ret->isEmpty();
}
} // namespace Internal

/// LocalApplicationRunConfiguration

LocalApplicationRunConfiguration::LocalApplicationRunConfiguration(Target *target, const Core::Id id) :
    RunConfiguration(target, id), m_macroExpander(0)
{
    ctor();
}

LocalApplicationRunConfiguration::LocalApplicationRunConfiguration(Target *target, LocalApplicationRunConfiguration *rc) :
    RunConfiguration(target, rc), m_macroExpander(0)
{
    ctor();
}

LocalApplicationRunConfiguration::~LocalApplicationRunConfiguration()
{
    delete m_macroExpander;
}

void LocalApplicationRunConfiguration::addToBaseEnvironment(Utils::Environment &env) const
{
    Q_UNUSED(env);
}

Utils::AbstractMacroExpander *LocalApplicationRunConfiguration::macroExpander() const
{
    if (BuildConfiguration *bc = activeBuildConfiguration())
        return bc->macroExpander();
    if (!m_macroExpander)
        m_macroExpander = new Internal::FallBackMacroExpander(target());
    return m_macroExpander;
}

void LocalApplicationRunConfiguration::ctor()
{
    addExtraAspect(new LocalEnvironmentAspect(this));
}

} // namespace ProjectExplorer
