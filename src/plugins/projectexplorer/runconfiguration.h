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

#ifndef RUNCONFIGURATION_H
#define RUNCONFIGURATION_H

#include "projectexplorer_export.h"

#include <QtCore/QObject>
#include <QtCore/QMetaType>
#include <QtCore/QPointer>
#include <QtCore/QWeakPointer>

QT_BEGIN_NAMESPACE
class QString;
class QWidget;
QT_END_NAMESPACE

namespace ProjectExplorer {

class Project;
class PersistentSettingsReader;
class PersistentSettingsWriter;

class RunControl;
class BuildConfiguration;

/* Base class for a run configuration. A run configuration specifies how a
 * project should be run, while the runner (see below) does the actual running.
 *
 * Note that all RunControls and the project hold a shared pointer to the RunConfiguration.
 * That is the lifetime of the RunConfiguration might exceed the life of the project.
 * The user might still have a RunControl running (or output tab of that RunControl open)
 * and yet unloaded the project. 
 * Also a RunConfiguration might be already removed from the list of RunConfigurations 
 * for a project, but stil be runnable via the output tab.
 
*/
class PROJECTEXPLORER_EXPORT RunConfiguration : public QObject
{
    Q_OBJECT
public:
    explicit RunConfiguration(Project *project);
    virtual ~RunConfiguration();
    Project *project() const;

    // The type of this RunConfiguration, e.g. "ProjectExplorer.LocalApplicationRunConfiguration"
    virtual QString type() const = 0;
    // Name shown to the user
    QString name() const;
    void setName(const QString &name);

    virtual bool isEnabled(BuildConfiguration *) const { return true; }
    bool isEnabled() const;

    // Returns the widget used to configure this run configuration. Ownership is transferred to the caller
    // rename to createConfigurationWidget
    virtual QWidget *configurationWidget() = 0;

    virtual void save(PersistentSettingsWriter &writer) const;
    virtual void restore(const PersistentSettingsReader &reader);
signals:
    void nameChanged();
private:
    QPointer<Project> m_project;
    QString m_name;
};

/* The run configuration factory is used for restoring run configurations from
 * settings. And used to create new runconfigurations in the "Run Settings" Dialog.
 * For the first case bool canRestore(const QString &type) and
 * RunConfiguration* create(Project *project, QString type) are used.
 * For the second type the functions QStringList availableCreationTypes(Project *pro) and
 * QString displayNameForType(const QString&) are used to generate a list of creatable
 * RunConfigurations, and create(..) is used to create it.
 */
class PROJECTEXPLORER_EXPORT IRunConfigurationFactory : public QObject
{
    Q_OBJECT
public:
    explicit IRunConfigurationFactory(QObject *parent = 0);
    virtual ~IRunConfigurationFactory();
    // used to recreate the runConfigurations when restoring settings
    virtual bool canRestore(const QString &type) const = 0;
    // used to show the list of possible additons to a project, returns a list of types
    virtual QStringList availableCreationTypes(Project *pro) const = 0;
    // used to translate the types to names to display to the user
    virtual QString displayNameForType(const QString &type) const = 0;
    // used to create a run configuration from scratch
    virtual RunConfiguration* create(Project *project, const QString &type) = 0;
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

    // Returns the widget used to configure this runner. Ownership is transferred to the caller
    virtual QWidget *configurationWidget(RunConfiguration *runConfiguration) = 0;
};

/* Each instance of this class represents one item that is run.
 */
class PROJECTEXPLORER_EXPORT RunControl : public QObject {
    Q_OBJECT
public:
    explicit RunControl(RunConfiguration *runConfiguration);
    virtual ~RunControl();
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual bool isRunning() const = 0;
    virtual QString displayName() const;

    bool sameRunConfiguration(RunControl *other);

signals:
    void addToOutputWindow(RunControl *, const QString &line);
    void addToOutputWindowInline(RunControl *, const QString &line);
    void error(RunControl *, const QString &error);
    void started();
    void finished();
public slots:
    void bringApplicationToForeground(qint64 pid);
private slots:
    void bringApplicationToForegroundInternal();

private:
    QString m_displayName;
    const QWeakPointer<RunConfiguration> m_runConfiguration;

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
