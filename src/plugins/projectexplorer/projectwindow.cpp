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

#include "projectwindow.h"

#include "project.h"
#include "environment.h"
#include "projectexplorer.h"
#include "projectexplorerconstants.h"
#include "iprojectproperties.h"
#include "session.h"
#include "projecttreewidget.h"
#include "runconfiguration.h"
#include "buildconfiguration.h"

#include <coreplugin/minisplitter.h>
#include <coreplugin/fileiconprovider.h>
#include <coreplugin/icore.h>
#include <coreplugin/ifile.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/styledbar.h>
#include <utils/stylehelper.h>

#include <QtCore/QDebug>
#include <QtGui/QApplication>
#include <QtGui/QBoxLayout>
#include <QtGui/QComboBox>
#include <QtGui/QScrollArea>
#include <QtGui/QTabWidget>
#include <QtGui/QTreeWidget>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QPainter>
#include <QtGui/QPaintEvent>
#include <QtGui/QMenu>


using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

namespace {
bool debug = false;
}

PanelsWidget::PanelsWidget(QWidget *parent)
    : QScrollArea(parent)
{
    QWidget *topwidget = new QWidget;
    QHBoxLayout *topwidgetLayout = new QHBoxLayout;
    topwidgetLayout->setMargin(0);
    topwidgetLayout->setSpacing(0);
    topwidget->setLayout(topwidgetLayout);

    QWidget *verticalWidget = new QWidget;
    verticalWidget->setMaximumWidth(800);
    m_layout = new QVBoxLayout;
    m_layout->addStretch(10);
    verticalWidget->setLayout(m_layout);
    topwidgetLayout->addWidget(verticalWidget);
    topwidgetLayout->addStretch(10);

    setFrameStyle(QFrame::NoFrame);
    setWidgetResizable(true);

    setWidget(topwidget);
}

PanelsWidget::~PanelsWidget()
{
    clear();
}

void PanelsWidget::addWidget(QWidget *widget)
{
    Panel p;
    p.spacer = 0;
    p.nameLabel = 0;
    p.panelWidget = widget;

    p.marginLayout = 0;
    m_layout->insertWidget(m_layout->count() - 1, widget);
    m_panels.append(p);
}

void PanelsWidget::addWidget(const QString &name, QWidget *widget)
{


    Panel p;
    p.spacer = new QSpacerItem(1, 10, QSizePolicy::Fixed, QSizePolicy::Fixed);
    p.nameLabel = new QLabel(this);
    p.nameLabel->setText(name);
    QFont f = p.nameLabel->font();
    f.setBold(true);
    f.setPointSizeF(f.pointSizeF() * 1.2);
    p.nameLabel->setFont(f);

    p.panelWidget = widget;

    m_layout->insertSpacerItem(m_layout->count() - 1, p.spacer);
    m_layout->insertWidget(m_layout->count() - 1, p.nameLabel);
    QHBoxLayout *hboxLayout = new QHBoxLayout();
    hboxLayout->setContentsMargins(20, 0, 0, 0);
    hboxLayout->addWidget(p.panelWidget);
    p.marginLayout = hboxLayout;
    m_layout->insertLayout(m_layout->count() -1, hboxLayout);
    m_panels.append(p);
}

void PanelsWidget::clear()
{
    foreach(Panel p, m_panels) {
        if (p.spacer) {
            m_layout->removeItem(p.spacer);
            delete p.spacer;
        }
        delete p.nameLabel;
        delete p.panelWidget;
        delete p.marginLayout;
    }
    m_panels.clear();
}

void PanelsWidget::removeWidget(QWidget *widget)
{
    for(int i=0; i<m_panels.count(); ++i) {
        const Panel & p = m_panels.at(i);
        if (p.panelWidget == widget) {
            if (p.spacer) {
                m_layout->removeItem(p.spacer);
                delete p.spacer;
            }
            if (p.marginLayout)
                p.marginLayout->removeWidget(p.panelWidget);
            else
                m_layout->removeWidget(p.panelWidget);
            delete p.nameLabel;
            delete p.marginLayout;
            m_panels.removeAt(i);
            break;
        }
    }
}

////
// ActiveConfigurationWidget
////

ActiveConfigurationWidget::ActiveConfigurationWidget(QWidget *parent)
    : QWidget(parent)
{
    QGridLayout *grid = new QGridLayout(this);
    grid->setMargin(0);
    RunConfigurationComboBox *runConfigurationComboBox = new RunConfigurationComboBox(this);
    grid->addWidget(new QLabel(tr("Active run configuration")), 0, 0);
    grid->addWidget(runConfigurationComboBox, 0, 1);

    SessionManager *session = ProjectExplorerPlugin::instance()->session();

    int i = 0;
    foreach(Project *p, session->projects()) {
        ++i;
        BuildConfigurationComboBox *buildConfigurationComboBox = new BuildConfigurationComboBox(p, this);
        QLabel *label = new QLabel("Build configuration for <b>" + p->name() + "</b>", this);
        grid->addWidget(label, i, 0);
        grid->addWidget(buildConfigurationComboBox, i, 1);
        m_buildComboBoxMap.insert(p, qMakePair(buildConfigurationComboBox, label));
    }

    connect(session, SIGNAL(projectAdded(ProjectExplorer::Project*)),
            this, SLOT(projectAdded(ProjectExplorer::Project*)));

    connect(session, SIGNAL(projectRemoved(ProjectExplorer::Project*)),
            this, SLOT(projectRemoved(ProjectExplorer::Project*)));

};

void ActiveConfigurationWidget::projectAdded(Project *p)
{
    QGridLayout *grid = static_cast<QGridLayout *>(layout());
    BuildConfigurationComboBox *buildConfigurationComboBox = new BuildConfigurationComboBox(p, this);
    QLabel *label = new QLabel("Build configuration for <b>" + p->name() + "</b>");
    grid->addWidget(label);
    grid->addWidget(buildConfigurationComboBox);
    m_buildComboBoxMap.insert(p, qMakePair(buildConfigurationComboBox, label));
}

void ActiveConfigurationWidget::projectRemoved(Project *p)
{
    // Find row

    // TODO also remove the label...
    QPair<BuildConfigurationComboBox *, QLabel *> pair = m_buildComboBoxMap.value(p);;
    delete pair.first;
    delete pair.second;
    m_buildComboBoxMap.remove(p);
}


ActiveConfigurationWidget::~ActiveConfigurationWidget()
{

}

////
// RunConfigurationComboBox
////

RunConfigurationComboBox::RunConfigurationComboBox(QWidget *parent)
    : QComboBox(parent), m_ignoreChange(false)
{
    setSizeAdjustPolicy(QComboBox::AdjustToContents);

    SessionManager *session = ProjectExplorer::ProjectExplorerPlugin::instance()->session();

    // Setup the treewidget
    rebuildTree();

    // Connect
    foreach(Project *p, session->projects()) {
        foreach(const QSharedPointer<RunConfiguration> &rc, p->runConfigurations()) {
            connect(rc.data(), SIGNAL(nameChanged()), this, SLOT(rebuildTree()));
        }
        connectToProject(p);
    }


    connect(session, SIGNAL(startupProjectChanged(ProjectExplorer::Project*)),
            this, SLOT(activeRunConfigurationChanged()));

    connect(session, SIGNAL(projectAdded(ProjectExplorer::Project*)),
            this, SLOT(projectAdded(ProjectExplorer::Project*)));
    connect(session, SIGNAL(projectRemoved(ProjectExplorer::Project*)),
            this, SLOT(projectRemoved(ProjectExplorer::Project*)));
    connect(this, SIGNAL(activated(int)),
            this, SLOT(activeItemChanged(int)));
}

RunConfigurationComboBox::~RunConfigurationComboBox()
{

}

int RunConfigurationComboBox::convertTreeIndexToInt(int project, int runconfigurationIndex)
{
    ++runconfigurationIndex;
    ++project;
    for(int i=0; i<count(); ++i) {
        if (itemData(i, Qt::UserRole).toInt() == 0) {
            --project;
        } else if (itemData(i, Qt::UserRole).toInt() == 1 && project == 0) {
            --runconfigurationIndex;
        }
        if (runconfigurationIndex == 0) {
            return i;
        }
    }
    return -1;
}

QPair<int, int> RunConfigurationComboBox::convertIntToTreeIndex(int index)
{
    int projectIndex = -1;
    int runConfigIndex = -1;
    for(int i = 0; i <= index; ++i) {
        if (itemData(i, Qt::UserRole).toInt() == 0) {
            ++projectIndex;
            runConfigIndex = -1;
        } else if (itemData(i, Qt::UserRole).toInt() == 1) {
            ++runConfigIndex;
        }
    }
    return qMakePair(projectIndex, runConfigIndex);
}

void RunConfigurationComboBox::activeItemChanged(int index)
{
    if (m_ignoreChange)
        return;
    m_ignoreChange = true;
    SessionManager *session = ProjectExplorer::ProjectExplorerPlugin::instance()->session();
    QPair<int, int> pair = convertIntToTreeIndex(index);
    if (pair.first == -1) {
        setCurrentIndex(-1);
    } else {
        if (pair.second == -1)
            pair.second = 0;
        QList<Project *> projects = session->projects();
        if (pair.first < projects.count()) {
            Project *p = projects.at(pair.first);
            QList<QSharedPointer<RunConfiguration> > runconfigurations = p->runConfigurations();
            if (pair.second < runconfigurations.count()) {
                session->setStartupProject(p);
                p->setActiveRunConfiguration(runconfigurations.at(pair.second));
                if (currentIndex() != convertTreeIndexToInt(pair.first, pair.second))
                    setCurrentIndex(convertTreeIndexToInt(pair.first, pair.second));
            }
        }
    }
    m_ignoreChange = false;
}

void RunConfigurationComboBox::activeRunConfigurationChanged()
{
    if (m_ignoreChange)
        return;
    m_ignoreChange = true;
    SessionManager *session = ProjectExplorer::ProjectExplorerPlugin::instance()->session();
    Project *startupProject = session->startupProject();
    if (startupProject) {
        int projectIndex = session->projects().indexOf(startupProject);
        int runConfigurationIndex = startupProject->runConfigurations().indexOf(startupProject->activeRunConfiguration());
        setCurrentIndex(convertTreeIndexToInt(projectIndex, runConfigurationIndex));
    } else {
        setCurrentIndex(-1);
    }
    m_ignoreChange = false;
}

void RunConfigurationComboBox::addedRunConfiguration(ProjectExplorer::Project *p, const QString &name)
{
    QSharedPointer<RunConfiguration> runConfiguration = QSharedPointer<RunConfiguration>(0);
    foreach(QSharedPointer<RunConfiguration> rc, p->runConfigurations()) {
        if (rc->name() == name) {
            runConfiguration = rc;
            break;
        }
    }
    if (runConfiguration) {
        connect(runConfiguration.data(), SIGNAL(nameChanged()),
                this, SLOT(rebuildTree()));
    }
    rebuildTree();
}

void RunConfigurationComboBox::removedRunConfiguration(ProjectExplorer::Project *p, const QString &name)
{
    QSharedPointer<RunConfiguration> runConfiguration = QSharedPointer<RunConfiguration>(0);
    foreach(QSharedPointer<RunConfiguration> rc, p->runConfigurations()) {
        if (rc->name() == name) {
            runConfiguration = rc;
            break;
        }
    }
    if (runConfiguration) {
        disconnect(runConfiguration.data(), SIGNAL(nameChanged()),
                this, SLOT(rebuildTree()));
    }

    rebuildTree();
}

void RunConfigurationComboBox::projectAdded(ProjectExplorer::Project *p)
{
    rebuildTree();
    foreach(const QSharedPointer<RunConfiguration> &rc, p->runConfigurations())
        connect(rc.data(), SIGNAL(nameChanged()), this, SLOT(rebuildTree()));
    connectToProject(p);
}

void RunConfigurationComboBox::projectRemoved(ProjectExplorer::Project *p)
{
    rebuildTree();
    disconnectFromProject(p);
}

void RunConfigurationComboBox::connectToProject(ProjectExplorer::Project *p)
{
    connect(p, SIGNAL(activeRunConfigurationChanged()),
            this, SLOT(activeRunConfigurationChanged()));
    connect(p, SIGNAL(addedRunConfiguration(ProjectExplorer::Project *, QString)),
            this, SLOT(addedRunConfiguration(ProjectExplorer::Project *, QString)));
    connect(p, SIGNAL(removedRunConfiguration(ProjectExplorer::Project *, QString)),
            this, SLOT(removedRunConfiguration(ProjectExplorer::Project *, QString)));
}

void RunConfigurationComboBox::disconnectFromProject(ProjectExplorer::Project *p)
{
    disconnect(p, SIGNAL(activeRunConfigurationChanged()),
            this, SLOT(activeRunConfigurationChanged()));
    disconnect(p, SIGNAL(addedRunConfiguration(ProjectExplorer::Project *, QString)),
            this, SLOT(addedRunConfiguration(ProjectExplorer::Project *, QString)));
    disconnect(p, SIGNAL(removedRunConfiguration(ProjectExplorer::Project *, QString)),
            this, SLOT(removedRunConfiguration(ProjectExplorer::Project *, QString)));
}

void RunConfigurationComboBox::rebuildTree()
{
    m_ignoreChange = true;
    clear();

    SessionManager *session = ProjectExplorer::ProjectExplorerPlugin::instance()->session();
    Project *startupProject = session->startupProject();
    foreach(Project *p, session->projects()) {
        addItem(p->name(), QVariant(0));
        foreach(QSharedPointer<RunConfiguration> rc, p->runConfigurations()) {
            addItem("  " + rc->name(), QVariant(1));
            if ((startupProject == p) && (p->activeRunConfiguration() == rc)){
                setCurrentIndex(count() - 1);
            }
        }
    }
    // Select the right index
    m_ignoreChange = false;
}

////
// BuildConfigurationComboBox
////


BuildConfigurationComboBox::BuildConfigurationComboBox(Project *p, QWidget *parent)
    : QStackedWidget(parent), ignoreIndexChange(false), m_project(p)
{
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    m_comboBox = new QComboBox(this);
    m_comboBox->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    addWidget(m_comboBox);

    m_label = new QLabel(this);
    m_label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    addWidget(m_label);

    //m_comboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    foreach(const BuildConfiguration *buildConfiguration, p->buildConfigurations())
        m_comboBox->addItem(buildConfiguration->displayName(), buildConfiguration->name());
    if (p->buildConfigurations().count() == 1) {
        m_label->setText(m_comboBox->itemText(0));
        setCurrentWidget(m_label);
    }

    int index = p->buildConfigurations().indexOf(p->activeBuildConfiguration());
    if (index != -1)
        m_comboBox->setCurrentIndex(index);

    connect(p, SIGNAL(buildConfigurationDisplayNameChanged(QString)),
            this, SLOT(nameChanged(QString)));
    connect(p, SIGNAL(activeBuildConfigurationChanged()),
            this, SLOT(activeConfigurationChanged()));
    connect(p, SIGNAL(addedBuildConfiguration(ProjectExplorer::Project *, QString)),
            this, SLOT(addedBuildConfiguration(ProjectExplorer::Project *, QString)));
    connect(p, SIGNAL(removedBuildConfiguration(ProjectExplorer::Project *, QString)),
            this, SLOT(removedBuildConfiguration(ProjectExplorer::Project *, QString)));
    connect(m_comboBox, SIGNAL(activated(int)),
            this, SLOT(changedIndex(int)));
}

BuildConfigurationComboBox::~BuildConfigurationComboBox()
{

}

void BuildConfigurationComboBox::nameChanged(const QString &buildConfiguration)
{
    int index = nameToIndex(buildConfiguration);
    if (index == -1)
        return;
    const QString &displayName = m_project->buildConfiguration(buildConfiguration)->displayName();
    m_comboBox->setItemText(index, displayName);
    if (m_comboBox->count() == 1)
        m_label->setText(displayName);
}

int BuildConfigurationComboBox::nameToIndex(const QString &buildConfiguration)
{
    for (int i=0; i < m_comboBox->count(); ++i)
        if (m_comboBox->itemData(i) == buildConfiguration)
            return i;
    return -1;
}

void BuildConfigurationComboBox::activeConfigurationChanged()
{
    int index = nameToIndex(m_project->activeBuildConfiguration()->name());
    if (index == -1)
        return;
    ignoreIndexChange = true;
    m_comboBox->setCurrentIndex(index);
    ignoreIndexChange = false;
}

void BuildConfigurationComboBox::addedBuildConfiguration(ProjectExplorer::Project *,const QString &buildConfiguration)
{
    ignoreIndexChange = true;
    m_comboBox->addItem(m_project->buildConfiguration(buildConfiguration)->displayName(), buildConfiguration);

    if (m_comboBox->count() == 2)
        setCurrentWidget(m_comboBox);
    ignoreIndexChange = false;
}

void BuildConfigurationComboBox::removedBuildConfiguration(ProjectExplorer::Project *, const QString &buildConfiguration)
{
    ignoreIndexChange = true;
    int index = nameToIndex(buildConfiguration);
    m_comboBox->removeItem(index);
    if (m_comboBox->count() == 1) {
        m_label->setText(m_comboBox->itemText(0));
        setCurrentWidget(m_label);
    }
    ignoreIndexChange = false;
}

void BuildConfigurationComboBox::changedIndex(int newIndex)
{
    if (newIndex == -1)
        return;
    m_project->setActiveBuildConfiguration(
            m_project->buildConfiguration(m_comboBox->itemData(newIndex).toString()));
}

///
// ProjectLabel
///

ProjectLabel::ProjectLabel(QWidget *parent)
    : QLabel(parent)
{

}

ProjectLabel::~ProjectLabel()
{

}

void ProjectLabel::setProject(ProjectExplorer::Project *p)
{
    if (p)
        setText(tr("Edit Project Settings for Project <b>%1</b>").arg(p->name()));
    else
        setText(tr("No Project loaded"));
}


///
// ProjectPushButton
///

ProjectPushButton::ProjectPushButton(QWidget *parent)
    : QPushButton(parent)
{
    setText(tr("Select Project"));
    m_menu = new QMenu(this);
    setMenu(m_menu);

    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    SessionManager *session = ProjectExplorerPlugin::instance()->session();

    foreach(Project *p, session->projects()) {
        QAction *act = m_menu->addAction(p->name());
        act->setData(QVariant::fromValue((void *) p));
        connect(act, SIGNAL(triggered()),
                this, SLOT(actionTriggered()));
    }

    setEnabled(session->projects().count() > 1);

    connect(session, SIGNAL(projectRemoved(ProjectExplorer::Project*)),
            this, SLOT(projectRemoved(ProjectExplorer::Project*)));
    connect(session, SIGNAL(projectAdded(ProjectExplorer::Project*)),
            this, SLOT(projectAdded(ProjectExplorer::Project*)));
}

ProjectPushButton::~ProjectPushButton()
{

}

void ProjectPushButton::projectAdded(ProjectExplorer::Project *p)
{
    QAction *act = m_menu->addAction(p->name());
    act->setData(QVariant::fromValue((void *) p));
    connect(act, SIGNAL(triggered()),
                this, SLOT(actionTriggered()));

    // Activate it
    if (m_menu->actions().count() == 1)
        emit projectChanged(p);
    else if (m_menu->actions().count() > 1)
        setEnabled(true);
}

void ProjectPushButton::projectRemoved(ProjectExplorer::Project *p)
{
    QList<Project *> projects = ProjectExplorerPlugin::instance()->session()->projects();

    bool needToChange = false;
    foreach(QAction *act, m_menu->actions()) {
        if (act->data().value<void *>() == (void *) p) {
            delete act;
            needToChange = true;
            break;
        }
    }

    // Comboboxes don't emit a signal if the index did't actually change
    if (m_menu->actions().isEmpty()) {
        emit projectChanged(0);
        setEnabled(false);
    } else if (needToChange) {
        emit projectChanged((ProjectExplorer::Project *) m_menu->actions().first()->data().value<void *>());
    }
}

void ProjectPushButton::actionTriggered()
{
    QAction *action = qobject_cast<QAction *>(sender());
    emit projectChanged((ProjectExplorer::Project *) action->data().value<void *>());
}

///
// OnePixelBlackLine
///

class OnePixelBlackLine : public QWidget
{
public:
    OnePixelBlackLine(QWidget *parent)
        : QWidget(parent)
    {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        setMinimumHeight(1);
        setMaximumHeight(1);
    }
    void paintEvent(QPaintEvent *e)
    {
        QPainter p(this);
        p.fillRect(e->rect(), QBrush(Utils::StyleHelper::borderColor()));
    }
};

///
// ProjectWindow
///

ProjectWindow::ProjectWindow(QWidget *parent)
    : QWidget(parent)
{
    ProjectExplorer::SessionManager *session = ProjectExplorerPlugin::instance()->session();

    m_panelsWidget = new PanelsWidget(this);

    m_activeConfigurationWidget = new ActiveConfigurationWidget(m_panelsWidget);

    m_panelsWidget->addWidget(tr("Active Build and Run Configurations"), m_activeConfigurationWidget);

    m_spacerBetween = new QWidget(this);
    QVBoxLayout *vbox = new QVBoxLayout(m_spacerBetween);
    vbox->setMargin(0);
    m_spacerBetween->setLayout(vbox);
    vbox->addSpacerItem(new QSpacerItem(10, 15, QSizePolicy::Fixed, QSizePolicy::Fixed));
    vbox->addWidget(new OnePixelBlackLine(m_spacerBetween));
    vbox->addSpacerItem(new QSpacerItem(10, 15, QSizePolicy::Fixed, QSizePolicy::Fixed));

    m_panelsWidget->addWidget(m_spacerBetween);

    m_projectChooser = new QWidget(m_panelsWidget);
    QHBoxLayout *hbox = new QHBoxLayout(m_projectChooser);
    hbox->setMargin(0);
    ProjectLabel *label = new ProjectLabel(m_projectChooser);
    {
        QFont f = label->font();
        f.setPointSizeF(f.pointSizeF() * 1.4);
        f.setBold(true);
        label->setFont(f);
    }
    hbox->addWidget(label);
    ProjectPushButton *changeProject = new ProjectPushButton(m_projectChooser);
    connect(changeProject, SIGNAL(projectChanged(ProjectExplorer::Project*)),
            label, SLOT(setProject(ProjectExplorer::Project*)));
    hbox->addWidget(changeProject);

    m_panelsWidget->addWidget(m_projectChooser);

    QVBoxLayout *topLevelLayout = new QVBoxLayout(this);
    topLevelLayout->setMargin(0);
    topLevelLayout->setSpacing(0);
    topLevelLayout->addWidget(new Utils::StyledBar(this));

    topLevelLayout->addWidget(m_panelsWidget);

    m_noprojectLabel = new QLabel(this);
    m_noprojectLabel->setText(tr("No project loaded."));
    {
        QFont f = m_noprojectLabel->font();
        f.setPointSizeF(f.pointSizeF() * 1.4);
        f.setBold(true);
        m_noprojectLabel->setFont(f);
    }
    m_noprojectLabel->setMargin(10);
    m_noprojectLabel->setAlignment(Qt::AlignTop);
    topLevelLayout->addWidget(m_noprojectLabel);

    bool noProjects = session->projects().isEmpty();
    m_panelsWidget->setVisible(!noProjects);
    m_noprojectLabel->setVisible(noProjects);

    connect(changeProject, SIGNAL(projectChanged(ProjectExplorer::Project*)),
            this, SLOT(showProperties(ProjectExplorer::Project*)));

    connect(session, SIGNAL(sessionLoaded()), this, SLOT(restoreStatus()));
    connect(session, SIGNAL(aboutToSaveSession()), this, SLOT(saveStatus()));

    connect(session, SIGNAL(projectAdded(ProjectExplorer::Project*)),
            this, SLOT(projectAdded()));
    connect(session, SIGNAL(projectRemoved(ProjectExplorer::Project*)),
            this, SLOT(projectRemoved()));
}

ProjectWindow::~ProjectWindow()
{
}

void ProjectWindow::projectAdded()
{
    m_panelsWidget->setVisible(true);
    m_noprojectLabel->setVisible(false);
}

void ProjectWindow::projectRemoved()
{
    if (ProjectExplorerPlugin::instance()->session()->projects().isEmpty()) {
        m_panelsWidget->setVisible(false);
        m_noprojectLabel->setVisible(true);
    }
}

void ProjectWindow::restoreStatus()
{
    // TODO
}

void ProjectWindow::saveStatus()
{
    // TODO
}

void ProjectWindow::showProperties(Project *project)
{
    if (debug)
        qDebug() << "ProjectWindow - showProperties called";

    m_panelsWidget->removeWidget(m_activeConfigurationWidget);
    m_panelsWidget->removeWidget(m_spacerBetween);
    m_panelsWidget->removeWidget(m_projectChooser);

    // Remove the tabs from the tab widget first
    m_panelsWidget->clear();

    m_panelsWidget->addWidget(tr("Active Build and Run Configurations"), m_activeConfigurationWidget);
    m_panelsWidget->addWidget(m_spacerBetween);
    m_panelsWidget->addWidget(m_projectChooser);

    if (project) {
        QList<IPanelFactory *> pages =
                ExtensionSystem::PluginManager::instance()->getObjects<IPanelFactory>();
        foreach (IPanelFactory *panelFactory, pages) {
            if (panelFactory->supports(project)) {
                PropertiesPanel *panel = panelFactory->createPanel(project);
                if (debug)
                  qDebug() << "ProjectWindow - setting up project properties tab " << panel->name();
                m_panelsWidget->addWidget(panel->name(), panel->widget());
            }
        }
    }
}
