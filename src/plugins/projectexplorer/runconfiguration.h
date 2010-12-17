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

#ifndef RUNCONFIGURATION_H
#define RUNCONFIGURATION_H

#include "projectconfiguration.h"
#include "projectexplorer_export.h"

#include <QtCore/QMetaType>
#include <QtCore/QWeakPointer>

QT_BEGIN_NAMESPACE
class QString;
class QWidget;
QT_END_NAMESPACE

namespace ProjectExplorer {

class Target;

class RunControl;
class BuildConfiguration;
class OutputFormatter;

/**
 * Base class for a run configuration. A run configuration specifies how a
 * target should be run, while the runner (see below) does the actual running.
 *
 * Note that all RunControls and the target hold a shared pointer to the RunConfiguration.
 * That is the lifetime of the RunConfiguration might exceed the life of the target.
 * The user might still have a RunControl running (or output tab of that RunControl open)
 * and yet unloaded the target.
 * Also a RunConfiguration might be already removed from the list of RunConfigurations
 * for a target, but stil be runnable via the output tab.
 */
class PROJECTEXPLORER_EXPORT RunConfiguration : public ProjectConfiguration
{
    Q_OBJECT

public:
    virtual ~RunConfiguration();

    /**
     * Used to find out whether a runconfiguration works with the given
     * buildconfiguration.
     * \note bc may be 0!
     */
    virtual bool isEnabled(BuildConfiguration *bc) const;

    /**
     * Used to find out whether a runconfiguration works with the active
     * buildconfiguration.
     */
    bool isEnabled() const;

    /// Returns the widget used to configure this run configuration. Ownership is transferred to the caller
    virtual QWidget *createConfigurationWidget() = 0;

    Target *target() const;

    virtual ProjectExplorer::OutputFormatter *createOutputFormatter() const;

    void setUseQmlDebugger(bool value);
    void setUseCppDebugger(bool value);
    bool useQmlDebugger() const;
    bool useCppDebugger() const;

    uint qmlDebugServerPort() const;
    void setQmlDebugServerPort(uint port);

    virtual QVariantMap toMap() const;

signals:
    void isEnabledChanged(bool value);
    void debuggersChanged();
    void qmlDebugServerPortChanged(uint port);

protected:
    RunConfiguration(Target *parent, const QString &id);
    RunConfiguration(Target *parent, RunConfiguration *source);

    /// convenience method to get current build configuration.
    BuildConfiguration *activeBuildConfiguration() const;

    virtual bool fromMap(const QVariantMap &map);

private:
    bool m_useCppDebugger;
    bool m_useQmlDebugger;
    uint m_qmlDebugServerPort;
};

/**
 * The run configuration factory is used for restoring run configurations from
 * settings. And used to create new runconfigurations in the "Run Settings" Dialog.
 * For the first case bool canRestore(Target *parent, const QString &id) and
 * RunConfiguration* create(Target *parent, const QString &id) are used.
 * For the second type the functions QStringList availableCreationIds(Target *parent) and
 * QString displayNameForType(const QString&) are used to generate a list of creatable
 * RunConfigurations, and create(..) is used to create it.
 */
class PROJECTEXPLORER_EXPORT IRunConfigurationFactory : public QObject
{
    Q_OBJECT

public:
    explicit IRunConfigurationFactory(QObject *parent = 0);
    virtual ~IRunConfigurationFactory();

    /// used to show the list of possible additons to a target, returns a list of types
    virtual QStringList availableCreationIds(Target *parent) const = 0;

    /// used to translate the types to names to display to the user
    virtual QString displayNameForId(const QString &id) const = 0;

    virtual bool canCreate(Target *parent, const QString &id) const = 0;
    virtual RunConfiguration *create(Target *parent, const QString &id) = 0;
    virtual bool canRestore(Target *parent, const QVariantMap &map) const = 0;
    virtual RunConfiguration *restore(Target *parent, const QVariantMap &map) = 0;
    virtual bool canClone(Target *parent, RunConfiguration *product) const = 0;
    virtual RunConfiguration *clone(Target *parent, RunConfiguration *product) = 0;

    static IRunConfigurationFactory *createFactory(Target *parent, const QString &id);
    static IRunConfigurationFactory *cloneFactory(Target *parent, RunConfiguration *source);
    static IRunConfigurationFactory *restoreFactory(Target *parent, const QVariantMap &map);

signals:
    void availableCreationIdsChanged();
};

class PROJECTEXPLORER_EXPORT IRunControlFactory : public QObject
{
    Q_OBJECT
public:
    explicit IRunControlFactory(QObject *parent = 0);
    virtual ~IRunControlFactory();

    virtual bool canRun(RunConfiguration *runConfiguration, const QString &mode) const = 0;
    virtual RunControl* create(RunConfiguration *runConfiguration, const QString &mode) = 0;

    virtual QString displayName() const = 0;

    /// Returns the widget used to configure this runner. Ownership is transferred to the caller
    virtual QWidget *createConfigurationWidget(RunConfiguration *runConfiguration) = 0;
};

/**
 * Each instance of this class represents one item that is run.
 */
class PROJECTEXPLORER_EXPORT RunControl : public QObject
{
    Q_OBJECT
public:
    enum StopResult {
        StoppedSynchronously, // Stopped.
        AsynchronousStop     // Stop sequence has been started
    };

    explicit RunControl(RunConfiguration *runConfiguration, QString mode);
    virtual ~RunControl();
    virtual void start() = 0;
    virtual bool aboutToStop() const;
    virtual StopResult stop() = 0;
    virtual bool isRunning() const = 0;
    virtual QString displayName() const;

    bool sameRunConfiguration(const RunControl *other) const;

    OutputFormatter *outputFormatter();
    QString runMode() const;

signals:
    void addToOutputWindow(ProjectExplorer::RunControl *, const QString &line, bool onStdErr);
    void addToOutputWindowInline(ProjectExplorer::RunControl *, const QString &line, bool onStdErr);
    void appendMessage(ProjectExplorer::RunControl *, const QString &error, bool isError);
    void started();
    void finished();

public slots:
    void bringApplicationToForeground(qint64 pid);

private slots:
    void bringApplicationToForegroundInternal();

private:
    QString m_displayName;
    QString m_runMode;
    const QWeakPointer<RunConfiguration> m_runConfiguration;
    OutputFormatter *m_outputFormatter;

#ifdef Q_OS_MAC
    //these two are used to bring apps in the foreground on Mac
    qint64 m_internalPid;
    int m_foregroundCount;
#endif
};

} // namespace ProjectExplorer

// Allow a RunConfiguration to be stored in a QVariant
Q_DECLARE_METATYPE(ProjectExplorer::RunConfiguration*)

#endif // RUNCONFIGURATION_H
