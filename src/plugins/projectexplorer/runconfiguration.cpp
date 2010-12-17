/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "runconfiguration.h"

#include "outputformatter.h"
#include "project.h"
#include "target.h"
#include "buildconfiguration.h"
#include "projectexplorerconstants.h"

#include <extensionsystem/pluginmanager.h>
#include <coreplugin/icore.h>
#include <utils/qtcassert.h>

#include <QtCore/QTimer>
#include <QtGui/QMainWindow>
#include <QtGui/QMessageBox>
#include <QtGui/QAbstractButton>

#ifdef Q_OS_MAC
#include <Carbon/Carbon.h>
#endif

using namespace ProjectExplorer;

namespace {
// Function objects:

const char * const USE_CPP_DEBUGGER_KEY("RunConfiguration.UseCppDebugger");
const char * const USE_QML_DEBUGGER_KEY("RunConfiguration.UseQmlDebugger");
const char * const QML_DEBUG_SERVER_PORT_KEY("RunConfiguration.QmlDebugServerPort");

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
    ProjectConfiguration(target, id),
    m_useCppDebugger(true),
    m_useQmlDebugger(false),
    m_qmlDebugServerPort(Constants::QML_DEFAULT_DEBUG_SERVER_PORT)
{
    Q_ASSERT(target);
}

RunConfiguration::RunConfiguration(Target *target, RunConfiguration *source) :
    ProjectConfiguration(target, source),
    m_useCppDebugger(source->useCppDebugger()),
    m_useQmlDebugger(source->useQmlDebugger())
{
    Q_ASSERT(target);
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
    return static_cast<Target *>(parent());
}

void RunConfiguration::setUseQmlDebugger(bool value)
{
    m_useQmlDebugger = value;
    emit debuggersChanged();
}

void RunConfiguration::setUseCppDebugger(bool value)
{
    m_useCppDebugger = value;
    emit debuggersChanged();
}

bool RunConfiguration::useCppDebugger() const
{
    return m_useCppDebugger;
}

bool RunConfiguration::useQmlDebugger() const
{
    return m_useQmlDebugger;
}

uint RunConfiguration::qmlDebugServerPort() const
{
    return m_qmlDebugServerPort;
}

void RunConfiguration::setQmlDebugServerPort(uint port)
{
    m_qmlDebugServerPort = port;
    emit qmlDebugServerPortChanged(port);
}

QVariantMap RunConfiguration::toMap() const
{
    QVariantMap map(ProjectConfiguration::toMap());
    map.insert(QLatin1String(USE_CPP_DEBUGGER_KEY), m_useCppDebugger);
    map.insert(QLatin1String(USE_QML_DEBUGGER_KEY), m_useQmlDebugger);
    map.insert(QLatin1String(QML_DEBUG_SERVER_PORT_KEY), m_qmlDebugServerPort);
    return map;
}

bool RunConfiguration::fromMap(const QVariantMap &map)
{
    m_useCppDebugger = map.value(QLatin1String(USE_CPP_DEBUGGER_KEY), true).toBool();
    m_useQmlDebugger = map.value(QLatin1String(USE_QML_DEBUGGER_KEY), false).toBool();
    m_qmlDebugServerPort = map.value(QLatin1String(QML_DEBUG_SERVER_PORT_KEY), Constants::QML_DEFAULT_DEBUG_SERVER_PORT).toUInt();

    return ProjectConfiguration::fromMap(map);
}

ProjectExplorer::OutputFormatter *RunConfiguration::createOutputFormatter() const
{
    return new OutputFormatter();
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

RunControl::RunControl(RunConfiguration *runConfiguration, QString mode)
    : m_runMode(mode), m_runConfiguration(runConfiguration), m_outputFormatter(0)
{
    if (runConfiguration) {
        m_displayName  = runConfiguration->displayName();
        m_outputFormatter = runConfiguration->createOutputFormatter();
    }
    // We need to ensure that there's always a OutputFormatter
    if (!m_outputFormatter)
        m_outputFormatter = new OutputFormatter();
}

RunControl::~RunControl()
{
    delete m_outputFormatter;
}

OutputFormatter *RunControl::outputFormatter()
{
    return m_outputFormatter;
}

QString RunControl::runMode() const
{
    return m_runMode;
}

QString RunControl::displayName() const
{
    return m_displayName;
}

bool RunControl::aboutToStop() const
{
    QTC_ASSERT(isRunning(), return true;)

    QMessageBox messageBox(QMessageBox::Warning,
                           tr("Application Still Running"),
                           tr("%1 is still running.").arg(displayName()),
                           QMessageBox::Cancel | QMessageBox::Yes,
                           Core::ICore::instance()->mainWindow());
    messageBox.setInformativeText(tr("Force it to quit?"));
    messageBox.setDefaultButton(QMessageBox::Yes);
    messageBox.button(QMessageBox::Yes)->setText(tr("Force Quit"));
    messageBox.button(QMessageBox::Cancel)->setText(tr("Keep Running"));
    return messageBox.exec() == QMessageBox::Yes;
}

bool RunControl::sameRunConfiguration(const RunControl *other) const
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
