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

#ifndef RUNCONFIGURATION_H
#define RUNCONFIGURATION_H

#include "projectconfiguration.h"
#include "projectexplorer_export.h"
#include "projectexplorerconstants.h"

#include <utils/outputformat.h>

#include <QMetaType>
#include <QPointer>
#include <QWidget>

QT_FORWARD_DECLARE_CLASS(QIcon)

namespace Utils { class OutputFormatter; }

namespace ProjectExplorer {
class Abi;
class BuildConfiguration;
class DebuggerRunConfigurationAspect;
class RunConfiguration;
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

class PROJECTEXPLORER_EXPORT IRunConfigurationAspect
{
public:
    virtual ~IRunConfigurationAspect() {}
    virtual QVariantMap toMap() const = 0;
    virtual QString displayName() const = 0;
protected:
    friend class RunConfiguration;
    virtual void fromMap(const QVariantMap &map) = 0;
};

class PROJECTEXPLORER_EXPORT DebuggerRunConfigurationAspect
    : public QObject, public ProjectExplorer::IRunConfigurationAspect
{
    Q_OBJECT

public:
    DebuggerRunConfigurationAspect(RunConfiguration *runConfiguration);
    DebuggerRunConfigurationAspect(RunConfiguration *runConfiguration, DebuggerRunConfigurationAspect *other);

    enum QmlDebuggerStatus {
        DisableQmlDebugger = 0,
        EnableQmlDebugger,
        AutoEnableQmlDebugger
    };

    QVariantMap toMap() const;
    void fromMap(const QVariantMap &map);

    QString displayName() const;

    bool useCppDebugger() const;
    void setUseCppDebugger(bool value);
    bool useQmlDebugger() const;
    void setUseQmlDebugger(bool value);
    uint qmlDebugServerPort() const;
    void setQmllDebugServerPort(uint port);
    bool useMultiProcess() const;
    void setUseMultiProcess(bool on);
    void suppressDisplay();
    void suppressQmlDebuggingOptions();
    void suppressCppDebuggingOptions();
    void suppressQmlDebuggingSpinbox();
    bool isDisplaySuppressed() const;
    bool areQmlDebuggingOptionsSuppressed() const;
    bool areCppDebuggingOptionsSuppressed() const;
    bool isQmlDebuggingSpinboxSuppressed() const;
    RunConfiguration *runConfiguration();

signals:
    void debuggersChanged();

public:
    RunConfiguration *m_runConfiguration;
    bool m_useCppDebugger;
    QmlDebuggerStatus m_useQmlDebugger;
    uint m_qmlDebugServerPort;
    bool m_useMultiProcess;

    bool m_suppressDisplay;
    bool m_suppressQmlDebuggingOptions;
    bool m_suppressCppDebuggingOptions;
    bool m_suppressQmlDebuggingSpinbox;
};


// Documentation inside.
class PROJECTEXPLORER_EXPORT RunConfiguration : public ProjectConfiguration
{
    Q_OBJECT

public:
    ~RunConfiguration();

    virtual bool isEnabled() const;
    virtual QString disabledReason() const;
    virtual QWidget *createConfigurationWidget() = 0;
    virtual bool isConfigured() const;
    // Pop up configuration dialog in case for example the executable is missing.
    virtual bool ensureConfigured(QString *errorMessage = 0);

    Target *target() const;

    virtual Utils::OutputFormatter *createOutputFormatter() const;

    bool fromMap(const QVariantMap &map);
    QVariantMap toMap() const;

    DebuggerRunConfigurationAspect *debuggerAspect() const { return m_debuggerAspect; }

    QList<IRunConfigurationAspect *> extraAspects() const;
    template <typename T> T *extraAspect() const
    {
        IRunConfigurationAspect *typeCheck = static_cast<T *>(0);
        Q_UNUSED(typeCheck);
        T *result = 0;
        foreach (IRunConfigurationAspect *a, m_aspects) {
            if ((result = dynamic_cast<T *>(a)) != 0)
                break;
        }
        return result;
    }

    virtual ProjectExplorer::Abi abi() const;

signals:
    void enabledChanged();

protected:
    RunConfiguration(Target *parent, const Core::Id id);
    RunConfiguration(Target *parent, RunConfiguration *source);

    /// convenience method to get current build configuration.
    BuildConfiguration *activeBuildConfiguration() const;

private:
    void addExtraAspects();

    QList<IRunConfigurationAspect *> m_aspects;
    DebuggerRunConfigurationAspect *m_debuggerAspect;
};

class PROJECTEXPLORER_EXPORT IRunConfigurationFactory : public QObject
{
    Q_OBJECT

public:
    explicit IRunConfigurationFactory(QObject *parent = 0);
    virtual ~IRunConfigurationFactory();

    virtual QList<Core::Id> availableCreationIds(Target *parent) const = 0;
    virtual QString displayNameForId(const Core::Id id) const = 0;

    virtual bool canCreate(Target *parent, const Core::Id id) const = 0;
    virtual RunConfiguration *create(Target *parent, const Core::Id id) = 0;
    virtual bool canRestore(Target *parent, const QVariantMap &map) const = 0;
    virtual RunConfiguration *restore(Target *parent, const QVariantMap &map) = 0;
    virtual bool canClone(Target *parent, RunConfiguration *product) const = 0;
    virtual RunConfiguration *clone(Target *parent, RunConfiguration *product) = 0;

    static IRunConfigurationFactory *find(Target *parent, const QVariantMap &map);
    static IRunConfigurationFactory *find(Target *parent, RunConfiguration *rc);
    static QList<IRunConfigurationFactory *> find(Target *parent);

signals:
    void availableCreationIdsChanged();
};

class RunConfigWidget;

class PROJECTEXPLORER_EXPORT IRunControlFactory : public QObject
{
    Q_OBJECT
public:
    explicit IRunControlFactory(QObject *parent = 0);
    virtual ~IRunControlFactory();

    virtual bool canRun(RunConfiguration *runConfiguration, RunMode mode) const = 0;
    virtual RunControl *create(RunConfiguration *runConfiguration, RunMode mode, QString *errorMessage) = 0;

    virtual QString displayName() const = 0;

    virtual IRunConfigurationAspect *createRunConfigurationAspect();
    virtual IRunConfigurationAspect *cloneRunConfigurationAspect(IRunConfigurationAspect *);
    virtual RunConfigWidget *createConfigurationWidget(RunConfiguration *runConfiguration);
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

    RunControl(RunConfiguration *runConfiguration, RunMode mode);
    virtual ~RunControl();
    virtual void start() = 0;

    virtual bool promptToStop(bool *optionalPrompt = 0) const;
    virtual StopResult stop() = 0;
    virtual bool isRunning() const = 0;
    virtual QString displayName() const;
    virtual QIcon icon() const = 0;

    ProcessHandle applicationProcessHandle() const;
    void setApplicationProcessHandle(const ProcessHandle &handle);
    Abi abi() const;

    RunConfiguration *runConfiguration() const;
    bool sameRunConfiguration(const RunControl *other) const;

    Utils::OutputFormatter *outputFormatter();
    RunMode runMode() const;

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
    RunMode m_runMode;
    const QPointer<RunConfiguration> m_runConfiguration;
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
