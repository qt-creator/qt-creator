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

#ifndef BUILDSTEP_H
#define BUILDSTEP_H

#include "projectconfiguration.h"
#include "projectexplorer_export.h"
#include "task.h"

#include <QtCore/QFutureInterface>
#include <QtGui/QWidget>

namespace ProjectExplorer {

class BuildConfiguration;
class BuildStepList;
class Target;

/*
// BuildSteps are the primary way plugin developers can customize
// how their projects (or projects from other plugins) are build.
//
// Building a project, is done by taking the list of buildsteps
// from the project and calling first init() than run() on them.
//
// That means to change the way your project is build, reimplemnt
// this class and add your Step to the buildStep list of the project.
//
// Note: The projects own the buildstep, do not delete them yourself.
//
// init() is called in the GUI thread and can be used to query the
// project for any information you need.
//
// run() is run via QtConccurrent in a own thread, if you need an
// eventloop you need to create it yourself!
//
*/

class BuildStepConfigWidget;

class PROJECTEXPLORER_EXPORT BuildStep : public ProjectConfiguration
{
    Q_OBJECT

protected:
    BuildStep(BuildStepList *bsl, const QString &id);
    BuildStep(BuildStepList *bsl, BuildStep *bs);

public:
    virtual ~BuildStep();

    // This function is run in the gui thread,
    // use it to retrieve any information that you need in run()
    virtual bool init() = 0;

    // Reimplement this. This function is called when the target is build.
    // This function is NOT run in the gui thread. It runs in its own thread
    // If you need an event loop, you need to create one.
    // The absolute minimal implementation is:
    // fi.reportResult(true);
    virtual void run(QFutureInterface<bool> &fi) = 0;

    // the Widget shown in the target settings dialog for this buildStep
    // ownership is transferred to the caller
    virtual BuildStepConfigWidget *createConfigWidget() = 0;

    // if this function returns true, the user can't delete this BuildStep for this target
    // and the user is prevented from changing the order immutable steps are run
    // the default implementation returns false
    virtual bool immutable() const;

    BuildConfiguration *buildConfiguration() const;
    Target *target() const;

    enum OutputFormat { NormalOutput, ErrorOutput, MessageOutput, ErrorMessageOutput };

signals:
    // Add a task.
    void addTask(const ProjectExplorer::Task &task);
    // The string is added to the generated output, usually in the output
    // window.
    // It should be in plain text, with the format in the parameter
    void addOutput(const QString &string, ProjectExplorer::BuildStep::OutputFormat format);
};

class PROJECTEXPLORER_EXPORT IBuildStepFactory :
    public QObject
{
    Q_OBJECT

public:
    explicit IBuildStepFactory(QObject *parent = 0);
    virtual ~IBuildStepFactory();

    // used to show the list of possible additons to a target, returns a list of types
    virtual QStringList availableCreationIds(BuildStepList *parent) const = 0;
    // used to translate the types to names to display to the user
    virtual QString displayNameForId(const QString &id) const = 0;

    virtual bool canCreate(BuildStepList *parent, const QString &id) const = 0;
    virtual BuildStep *create(BuildStepList *parent, const QString &id) = 0;
    // used to recreate the runConfigurations when restoring settings
    virtual bool canRestore(BuildStepList *parent, const QVariantMap &map) const = 0;
    virtual BuildStep *restore(BuildStepList *parent, const QVariantMap &map) = 0;
    virtual bool canClone(BuildStepList *parent, BuildStep *product) const = 0;
    virtual BuildStep *clone(BuildStepList *parent, BuildStep *product) = 0;
};

class PROJECTEXPLORER_EXPORT BuildConfigWidget
    : public QWidget
{
    Q_OBJECT
public:
    BuildConfigWidget()
        :QWidget(0)
        {}

    virtual QString displayName() const = 0;

    // This is called to set up the config widget before showing it
    virtual void init(BuildConfiguration *bc) = 0;

signals:
    void displayNameChanged(const QString &);
};

class PROJECTEXPLORER_EXPORT BuildStepConfigWidget
    : public QWidget
{
    Q_OBJECT
public:
    BuildStepConfigWidget()
        : QWidget()
        {}
    virtual void init() = 0;
    virtual QString summaryText() const = 0;
    virtual QString displayName() const = 0;
signals:
    void updateSummary();
};

} // namespace ProjectExplorer

Q_DECLARE_METATYPE(ProjectExplorer::BuildStep::OutputFormat)

#endif // BUILDSTEP_H
