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

#ifndef QMAKESTEP_H
#define QMAKESTEP_H

#include "qt4projectmanager_global.h"
#include <utils/fileutils.h>
#include <projectexplorer/abstractprocessstep.h>

#include <QStringList>

namespace ProjectExplorer {
class BuildStep;
class IBuildStepFactory;
class Project;
}

namespace Qt4ProjectManager {
class Qt4BuildConfiguration;
class Qt4Project;

namespace Internal {

namespace Ui { class QMakeStep; }

class QMakeStepFactory : public ProjectExplorer::IBuildStepFactory
{
    Q_OBJECT

public:
    explicit QMakeStepFactory(QObject *parent = 0);
    virtual ~QMakeStepFactory();
    bool canCreate(ProjectExplorer::BuildStepList *parent, const Core::Id id) const;
    ProjectExplorer::BuildStep *create(ProjectExplorer::BuildStepList *parent, const Core::Id id);
    bool canClone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *bs) const;
    ProjectExplorer::BuildStep *clone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *bs);
    bool canRestore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map) const;
    ProjectExplorer::BuildStep *restore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map);
    QList<Core::Id> availableCreationIds(ProjectExplorer::BuildStepList *parent) const;
    QString displayNameForId(const Core::Id id) const;
};

} // namespace Internal


class QT4PROJECTMANAGER_EXPORT QMakeStep : public ProjectExplorer::AbstractProcessStep
{
    Q_OBJECT
    friend class Internal::QMakeStepFactory;

    enum QmlLibraryLink {
        DoNotLink = 0,
        DoLink,
        DebugLink
    };

public:
    explicit QMakeStep(ProjectExplorer::BuildStepList *parent);
    virtual ~QMakeStep();

    Qt4BuildConfiguration *qt4BuildConfiguration() const;
    virtual bool init();
    virtual void run(QFutureInterface<bool> &);
    virtual ProjectExplorer::BuildStepConfigWidget *createConfigWidget();
    virtual bool immutable() const;
    void setForced(bool b);
    bool forced();

    // the complete argument line
    QString allArguments(bool shorted = false);
    // deduced arguments e.g. qmljs debugging
    QStringList deducedArguments();
    // deduced arguments with -after, e.g. OBJECTS_DIR for symbian
    QStringList deducedArgumentsAfter();
    // arguments passed to the pro file parser
    QStringList parserArguments();
    // arguments set by the user
    QString userArguments();
    Utils::FileName mkspec();
    void setUserArguments(const QString &arguments);
    bool linkQmlDebuggingLibrary() const;
    void setLinkQmlDebuggingLibrary(bool enable);
    bool isQmlDebuggingLibrarySupported(QString *reason = 0) const;

    QVariantMap toMap() const;

signals:
    void userArgumentsChanged();
    void linkQmlDebuggingLibraryChanged();

protected:
    QMakeStep(ProjectExplorer::BuildStepList *parent, QMakeStep *source);
    QMakeStep(ProjectExplorer::BuildStepList *parent, const Core::Id id);
    virtual bool fromMap(const QVariantMap &map);

    virtual void processStartupFailed();
    virtual bool processSucceeded(int exitCode, QProcess::ExitStatus status);

private:
    void ctor();

    // last values
    bool m_forced;
    bool m_needToRunQMake; // set in init(), read in run()
    QString m_userArgs;
    QmlLibraryLink m_linkQmlDebuggingLibrary;
    bool m_scriptTemplate;
    QList<ProjectExplorer::Task> m_tasks;
};


class QMakeStepConfigWidget : public ProjectExplorer::BuildStepConfigWidget
{
    Q_OBJECT
public:
    QMakeStepConfigWidget(QMakeStep *step);
    ~QMakeStepConfigWidget();
    QString summaryText() const;
    QString additionalSummaryText() const;
    QString displayName() const;
private slots:
    // slots for handling buildconfiguration/step signals
    void qtVersionChanged();
    void qmakeBuildConfigChanged();
    void userArgumentsChanged();
    void linkQmlDebuggingLibraryChanged();

    // slots for dealing with user changes in our UI
    void qmakeArgumentsLineEdited();
    void buildConfigurationSelected();
    void linkQmlDebuggingLibraryChecked(bool checked);

    // other
    void buildQmlDebuggingHelper();

private slots:
    void recompileMessageBoxFinished(int button);

private:
    void updateSummaryLabel();
    void updateQmlDebuggingOption();
    void updateEffectiveQMakeCall();

    void setSummaryText(const QString &);

    Internal::Ui::QMakeStep *m_ui;
    QMakeStep *m_step;
    QString m_summaryText;
    QString m_additionalSummaryText;
    bool m_ignoreChange;
};

} // namespace Qt4ProjectManager

#endif // QMAKESTEP_H
