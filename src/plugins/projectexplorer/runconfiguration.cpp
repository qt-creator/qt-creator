/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "runconfiguration.h"

#include "project.h"
#include "target.h"
#include "buildconfiguration.h"

#include <extensionsystem/pluginmanager.h>

#include <QtCore/QTimer>

#ifdef Q_OS_MAC
#include <Carbon/Carbon.h>
#endif

using namespace ProjectExplorer;

namespace {
// Function objects:

class RunConfigurationFactoryMatcher
{
public:
    RunConfigurationFactoryMatcher(Target * target) : m_target(target)
    { }

    virtual bool operator()(IRunConfigurationFactory *) const = 0;

    Target *target() const
    {
        return m_target;
    }

private:
    Target *m_target;
};

class CreateMatcher : public RunConfigurationFactoryMatcher
{
public:
    CreateMatcher(Target *target, const QString &id) :
        RunConfigurationFactoryMatcher(target),
        m_id(id)
    { }
    ~CreateMatcher() { }

    bool operator()(IRunConfigurationFactory *factory) const
    {
        return factory->canCreate(target(), m_id);
    }

private:
    QString m_id;
};

class CloneMatcher : public RunConfigurationFactoryMatcher
{
public:
    CloneMatcher(Target *target, RunConfiguration *source) :
        RunConfigurationFactoryMatcher(target),
        m_source(source)
    { }
    ~CloneMatcher() { }

    bool operator()(IRunConfigurationFactory *factory) const
    {
        return factory->canClone(target(), m_source);
    }

private:
    RunConfiguration *m_source;
};

class RestoreMatcher : public RunConfigurationFactoryMatcher
{
public:
    RestoreMatcher(Target *target, const QVariantMap &map) :
        RunConfigurationFactoryMatcher(target),
        m_map(map)
    { }
    ~RestoreMatcher() { }

    bool operator()(IRunConfigurationFactory *factory) const
    {
        return factory->canRestore(target(), m_map);
    }

private:
    QVariantMap m_map;
};

// Helper methods:

IRunConfigurationFactory * findRunConfigurationFactory(RunConfigurationFactoryMatcher &matcher)
{
    QList<IRunConfigurationFactory *>
            factories(ExtensionSystem::PluginManager::instance()->
                getObjects<IRunConfigurationFactory>());
    foreach (IRunConfigurationFactory *factory, factories) {
        if (matcher(factory))
            return factory;
    }
    return 0;
}

} // namespace

// RunConfiguration
RunConfiguration::RunConfiguration(Target *target, const QString &id) :
    ProjectConfiguration(id),
    m_target(target)
{
    Q_ASSERT(m_target);
}

RunConfiguration::RunConfiguration(Target *target, RunConfiguration *source) :
    ProjectConfiguration(source),
    m_target(target)
{
    Q_ASSERT(m_target);
}

RunConfiguration::~RunConfiguration()
{
}

bool RunConfiguration::isEnabled(BuildConfiguration *bc) const
{
    Q_UNUSED(bc);
    return true;
}

bool RunConfiguration::isEnabled() const
{
    if (target()->project()->hasActiveBuildSettings()
        && !activeBuildConfiguration())
        return false;
    return isEnabled(activeBuildConfiguration());
}

BuildConfiguration *RunConfiguration::activeBuildConfiguration() const
{
    if (!target())
        return 0;
    return target()->activeBuildConfiguration();
}

Target *RunConfiguration::target() const
{
    return m_target;
}

IRunConfigurationFactory::IRunConfigurationFactory(QObject *parent) :
    QObject(parent)
{
}

IRunConfigurationFactory::~IRunConfigurationFactory()
{
}

IRunConfigurationFactory *IRunConfigurationFactory::createFactory(Target *parent, const QString &id)
{
    CreateMatcher matcher(parent, id);
    return findRunConfigurationFactory(matcher);
}

IRunConfigurationFactory *IRunConfigurationFactory::cloneFactory(Target *parent, RunConfiguration *source)
{
    CloneMatcher matcher(parent, source);
    return findRunConfigurationFactory(matcher);
}

IRunConfigurationFactory *IRunConfigurationFactory::restoreFactory(Target *parent, const QVariantMap &map)
{
    RestoreMatcher matcher(parent, map);
    return findRunConfigurationFactory(matcher);
}

IRunControlFactory::IRunControlFactory(QObject *parent)
    : QObject(parent)
{
}

IRunControlFactory::~IRunControlFactory()
{
}

RunControl::RunControl(RunConfiguration *runConfiguration)
    : m_runConfiguration(runConfiguration)
{
    if (runConfiguration)
        m_displayName  = runConfiguration->displayName();
}

RunControl::~RunControl()
{

}

QString RunControl::displayName() const
{
    return m_displayName;
}

bool RunControl::sameRunConfiguration(RunControl *other)
{
    return other->m_runConfiguration.data() == m_runConfiguration.data();
}

void RunControl::bringApplicationToForeground(qint64 pid)
{
#ifdef Q_OS_MAC
    m_internalPid = pid;
    m_foregroundCount = 0;
    bringApplicationToForegroundInternal();
#else
    Q_UNUSED(pid)
#endif
}

void RunControl::bringApplicationToForegroundInternal()
{
#ifdef Q_OS_MAC
    ProcessSerialNumber psn;
    GetProcessForPID(m_internalPid, &psn);
    if (SetFrontProcess(&psn) == procNotFound && m_foregroundCount < 15) {
        // somehow the mac/carbon api says
        // "-600 no eligible process with specified process id"
        // if we call SetFrontProcess too early
        ++m_foregroundCount;
        QTimer::singleShot(200, this, SLOT(bringApplicationToForegroundInternal()));
        return;
    }
#endif
}
