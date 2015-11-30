/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
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
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef RUNCONFIGURATION_H
#define RUNCONFIGURATION_H

#include "projectconfiguration.h"
#include "projectexplorer_export.h"
#include "projectexplorerconstants.h"

#include <utils/outputformat.h>
#include <utils/qtcassert.h>
#include <utils/icon.h>

#include <QPointer>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QFormLayout;
QT_END_NAMESPACE

namespace Utils { class OutputFormatter; }

namespace ProjectExplorer {
class Abi;
class BuildConfiguration;
class IRunConfigurationAspect;
class RunConfiguration;
class RunConfigWidget;
class RunControl;
class Target;

// FIXME: This should also contain a handle to an remote device if used.
class PROJECTEXPLORER_EXPORT ProcessHandle
{
public:
    explicit ProcessHandle(quint64 pid = 0);

    bool isValid() const;
    void setPid(quint64 pid);
    quint64 pid() const;
    QString toString() const;

    bool equals(const ProcessHandle &) const;

private:
    quint64 m_pid;
};

inline bool operator==(const ProcessHandle &p1, const ProcessHandle &p2) { return p1.equals(p2); }
inline bool operator!=(const ProcessHandle &p1, const ProcessHandle &p2) { return !p1.equals(p2); }

/**
 * An interface for a hunk of global or per-project
 * configuration data.
 *
 */

class PROJECTEXPLORER_EXPORT ISettingsAspect : public QObject
{
    Q_OBJECT

public:
    ISettingsAspect() {}

    /// Create a configuration widget for this settings aspect.
    virtual QWidget *createConfigWidget(QWidget *parent) = 0;
    /// "Virtual default constructor"
    virtual ISettingsAspect *create() const = 0;
    /// "Virtual copy constructor"
    ISettingsAspect *clone() const;

protected:
    ///
    friend class IRunConfigurationAspect;
    /// Converts current object into map for storage.
    virtual void toMap(QVariantMap &map) const = 0;
    /// Read object state from @p map.
    virtual void fromMap(const QVariantMap &map) = 0;
};


/**
 * An interface to facilitate switching between hunks of
 * global and per-project configuration data.
 *
 */

class PROJECTEXPLORER_EXPORT IRunConfigurationAspect : public QObject
{
    Q_OBJECT

public:
    explicit IRunConfigurationAspect(RunConfiguration *runConfig);
    ~IRunConfigurationAspect();

    virtual IRunConfigurationAspect *create(RunConfiguration *runConfig) const = 0;
    virtual IRunConfigurationAspect *clone(RunConfiguration *runConfig) const;
    virtual RunConfigWidget *createConfigurationWidget();

    void setId(Core::Id id) { m_id = id; }
    void setDisplayName(const QString &displayName) { m_displayName = displayName; }
    void setProjectSettings(ISettingsAspect *settings);
    void setGlobalSettings(ISettingsAspect *settings);

    QString displayName() const { return m_displayName; }
    Core::Id id() const { return m_id; }
    bool isUsingGlobalSettings() const { return m_useGlobalSettings; }
    void setUsingGlobalSettings(bool value);
    void resetProjectToGlobalSettings();

    ISettingsAspect *projectSettings() const { return m_projectSettings; }
    ISettingsAspect *globalSettings() const { return m_globalSettings; }
    ISettingsAspect *currentSettings() const;
    RunConfiguration *runConfiguration() const { return m_runConfiguration; }

protected:
    friend class RunConfiguration;
    virtual void fromMap(const QVariantMap &map);
    virtual void toMap(QVariantMap &data) const;

private:
    Core::Id m_id;
    QString m_displayName;
    bool m_useGlobalSettings;
    RunConfiguration *m_runConfiguration;
    ISettingsAspect *m_projectSettings; // Owned if present.
    ISettingsAspect *m_globalSettings;  // Not owned.
};

// Documentation inside.
class PROJECTEXPLORER_EXPORT RunConfiguration : public ProjectConfiguration
{
    Q_OBJECT

public:
    ~RunConfiguration() override;

    virtual bool isEnabled() const;
    virtual QString disabledReason() const;
    virtual QWidget *createConfigurationWidget() = 0;

    virtual bool isConfigured() const;
    // Pop up configuration dialog in case for example the executable is missing.
    enum ConfigurationState { Configured, UnConfigured, Waiting };
    // TODO rename function
    virtual ConfigurationState ensureConfigured(QString *errorMessage = 0);

    Target *target() const;

    virtual Utils::OutputFormatter *createOutputFormatter() const;

    bool fromMap(const QVariantMap &map) override;
    QVariantMap toMap() const override;

    QList<IRunConfigurationAspect *> extraAspects() const;
    IRunConfigurationAspect *extraAspect(Core::Id id) const;

    template <typename T> T *extraAspect() const
    {
        QTC_ASSERT(m_aspectsInitialized, return 0);
        foreach (IRunConfigurationAspect *aspect, m_aspects)
            if (T *result = qobject_cast<T *>(aspect))
                return result;
        return 0;
    }

    virtual Abi abi() const;

    void addExtraAspects();
    void addExtraAspect(IRunConfigurationAspect *aspect);

signals:
    void enabledChanged();
    void requestRunActionsUpdate();
    void configurationFinished();

protected:
    RunConfiguration(Target *parent, Core::Id id);
    RunConfiguration(Target *parent, RunConfiguration *source);

    /// convenience function to get current build configuration.
    BuildConfiguration *activeBuildConfiguration() const;

private:
    void ctor();

    QList<IRunConfigurationAspect *> m_aspects;
    bool m_aspectsInitialized;
};

class PROJECTEXPLORER_EXPORT IRunConfigurationFactory : public QObject
{
    Q_OBJECT

public:
    explicit IRunConfigurationFactory(QObject *parent = 0);
    virtual ~IRunConfigurationFactory();

    enum CreationMode {UserCreate, AutoCreate};
    virtual QList<Core::Id> availableCreationIds(Target *parent, CreationMode mode = UserCreate) const = 0;
    virtual QString displayNameForId(Core::Id id) const = 0;

    virtual bool canCreate(Target *parent, Core::Id id) const = 0;
    RunConfiguration *create(Target *parent, Core::Id id);
    virtual bool canRestore(Target *parent, const QVariantMap &map) const = 0;
    RunConfiguration *restore(Target *parent, const QVariantMap &map);
    virtual bool canClone(Target *parent, RunConfiguration *product) const = 0;
    virtual RunConfiguration *clone(Target *parent, RunConfiguration *product) = 0;

    static IRunConfigurationFactory *find(Target *parent, const QVariantMap &map);
    static IRunConfigurationFactory *find(Target *parent, RunConfiguration *rc);
    static QList<IRunConfigurationFactory *> find(Target *parent);

signals:
    void availableCreationIdsChanged();

private:
    virtual RunConfiguration *doCreate(Target *parent, Core::Id id) = 0;
    virtual RunConfiguration *doRestore(Target *parent, const QVariantMap &map) = 0;
};

class PROJECTEXPLORER_EXPORT IRunControlFactory : public QObject
{
    Q_OBJECT
public:
    explicit IRunControlFactory(QObject *parent = 0);
    virtual ~IRunControlFactory();

    virtual bool canRun(RunConfiguration *runConfiguration, Core::Id mode) const = 0;
    virtual RunControl *create(RunConfiguration *runConfiguration, Core::Id mode, QString *errorMessage) = 0;

    virtual IRunConfigurationAspect *createRunConfigurationAspect(RunConfiguration *rc);
};

class PROJECTEXPLORER_EXPORT RunConfigWidget
    : public QWidget
{
    Q_OBJECT
public:
    RunConfigWidget()
        : QWidget(0)
    {}

    virtual QString displayName() const = 0;

signals:
    void displayNameChanged(const QString &);
};

class PROJECTEXPLORER_EXPORT RunControl : public QObject
{
    Q_OBJECT
public:
    enum StopResult {
        StoppedSynchronously, // Stopped.
        AsynchronousStop     // Stop sequence has been started
    };

    RunControl(RunConfiguration *runConfiguration, Core::Id mode);
    virtual ~RunControl();
    virtual void start() = 0;

    virtual bool promptToStop(bool *optionalPrompt = 0) const;
    virtual StopResult stop() = 0;
    virtual bool isRunning() const = 0;
    virtual QString displayName() const;
    virtual bool supportsReRunning() const { return true; }

    void setIcon(const Utils::Icon &icon);
    Utils::Icon icon() const;

    ProcessHandle applicationProcessHandle() const;
    void setApplicationProcessHandle(const ProcessHandle &handle);
    Abi abi() const;

    RunConfiguration *runConfiguration() const;
    Project *project() const;
    bool sameRunConfiguration(const RunControl *other) const;

    Utils::OutputFormatter *outputFormatter();
    Core::Id runMode() const;

public slots:
    void bringApplicationToForeground(qint64 pid);
    void appendMessage(const QString &msg, Utils::OutputFormat format);

signals:
    void appendMessage(ProjectExplorer::RunControl *runControl,
        const QString &msg, Utils::OutputFormat format);
    void started();
    void finished();
    void applicationProcessHandleChanged();

private slots:
    void bringApplicationToForegroundInternal();

protected:
    bool showPromptToStopDialog(const QString &title, const QString &text,
                                const QString &stopButtonText = QString(),
                                const QString &cancelButtonText = QString(),
                                bool *prompt = 0) const;

private:
    QString m_displayName;
    Core::Id m_runMode;
    Utils::Icon m_icon;
    const QPointer<RunConfiguration> m_runConfiguration;
    QPointer<Project> m_project;
    Utils::OutputFormatter *m_outputFormatter;

    // A handle to the actual application process.
    ProcessHandle m_applicationProcessHandle;

#ifdef Q_OS_MAC
    //these two are used to bring apps in the foreground on Mac
    qint64 m_internalPid;
    int m_foregroundCount;
#endif
};

} // namespace ProjectExplorer

// Allow a RunConfiguration to be stored in a QVariant
Q_DECLARE_METATYPE(ProjectExplorer::RunConfiguration*)
Q_DECLARE_METATYPE(ProjectExplorer::RunControl*)

#endif // RUNCONFIGURATION_H
