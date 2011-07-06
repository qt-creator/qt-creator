/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef RUNCONFIGURATION_H
#define RUNCONFIGURATION_H

#include "abi.h"
#include "projectconfiguration.h"
#include "projectexplorer_export.h"

#include <utils/outputformatter.h>

#include <QtCore/QMetaType>
#include <QtCore/QWeakPointer>
#include <QtGui/QWidget>
#include <QtGui/QIcon>

namespace ProjectExplorer {

class BuildConfiguration;
class IRunConfigurationAspect;
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

// Documentation inside.
class PROJECTEXPLORER_EXPORT RunConfiguration : public ProjectConfiguration
{
    Q_OBJECT

public:
    virtual ~RunConfiguration();

    virtual bool isEnabled() const;
    virtual QString disabledReason() const;
    virtual QWidget *createConfigurationWidget() = 0;

    Target *target() const;

    virtual Utils::OutputFormatter *createOutputFormatter() const;

    void setUseQmlDebugger(bool value);
    void setUseCppDebugger(bool value);
    bool useQmlDebugger() const;
    bool useCppDebugger() const;

    uint qmlDebugServerPort() const;
    void setQmlDebugServerPort(uint port);

    virtual QVariantMap toMap() const;

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
    void addExtraAspects();

    enum QmlDebuggerStatus {
        DisableQmlDebugger = 0,
        EnableQmlDebugger,
        AutoEnableQmlDebugger
    };

    bool m_useCppDebugger;
    mutable QmlDebuggerStatus m_useQmlDebugger;
    uint m_qmlDebugServerPort;
    QList<IRunConfigurationAspect *> m_aspects;
};

class PROJECTEXPLORER_EXPORT IRunConfigurationAspect
{
public:
    virtual ~IRunConfigurationAspect() {}
    virtual QVariantMap toMap() const = 0;
    virtual QString displayName() const = 0;
protected:
    friend class RunConfiguration;
    virtual bool fromMap(const QVariantMap &map) = 0;
};

class PROJECTEXPLORER_EXPORT IRunConfigurationFactory : public QObject
{
    Q_OBJECT

public:
    explicit IRunConfigurationFactory(QObject *parent = 0);
    virtual ~IRunConfigurationFactory();

    virtual QStringList availableCreationIds(Target *parent) const = 0;
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

class RunConfigWidget;

class PROJECTEXPLORER_EXPORT IRunControlFactory : public QObject
{
    Q_OBJECT
public:
    explicit IRunControlFactory(QObject *parent = 0);
    virtual ~IRunControlFactory();

    virtual bool canRun(RunConfiguration *runConfiguration, const QString &mode) const = 0;
    virtual RunControl* create(RunConfiguration *runConfiguration, const QString &mode) = 0;

    virtual QString displayName() const = 0;

    virtual IRunConfigurationAspect *createRunConfigurationAspect();
    virtual RunConfigWidget *createConfigurationWidget(RunConfiguration *runConfiguration) = 0;
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

    explicit RunControl(RunConfiguration *runConfiguration, QString mode);
    virtual ~RunControl();
    virtual void start() = 0;

    virtual bool promptToStop(bool *optionalPrompt = 0) const;
    virtual StopResult stop() = 0;
    virtual bool isRunning() const = 0;
    virtual QString displayName() const;
    virtual QIcon icon() const = 0;

    ProcessHandle applicationProcessHandle() const;
    void setApplicationProcessHandle(const ProcessHandle &handle);

    bool sameRunConfiguration(const RunControl *other) const;

    Utils::OutputFormatter *outputFormatter();
    QString runMode() const;

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
    QString m_runMode;
    const QWeakPointer<RunConfiguration> m_runConfiguration;
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
