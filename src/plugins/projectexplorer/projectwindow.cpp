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

#include <coreplugin/minisplitter.h>
#include <coreplugin/fileiconprovider.h>
#include <coreplugin/icore.h>
#include <coreplugin/ifile.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/styledbar.h>

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
#include <utils/stylehelper.h>

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
    p.nameLabel = 0;
    p.panelWidget = widget;

    p.marginLayout = 0;
    m_layout->insertWidget(m_layout->count() -1, widget);
    m_panels.append(p);
}

void PanelsWidget::addWidget(const QString &name, QWidget *widget)
{
    Panel p;
    p.nameLabel = new QLabel(this);
    p.nameLabel->setText(name);
    QFont f = p.nameLabel->font();
    f.setBold(true);
    f.setPointSizeF(f.pointSizeF() * 1.4);
    p.nameLabel->setFont(f);

    p.panelWidget = widget;

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
    : QComboBox(parent), ignoreIndexChange(false), m_project(p)
{
    setSizeAdjustPolicy(QComboBox::AdjustToContents);
    foreach(const QString &buildConfiguration, p->buildConfigurations())
        addItem(p->displayNameFor(buildConfiguration), buildConfiguration);

    int index = p->buildConfigurations().indexOf(p->activeBuildConfiguration());
    if (index != -1)
        setCurrentIndex(index);

    connect(p, SIGNAL(buildConfigurationDisplayNameChanged(QString)),
            this, SLOT(nameChanged(QString)));
    connect(p, SIGNAL(activeBuildConfigurationChanged()),
            this, SLOT(activeConfigurationChanged()));
    connect(p, SIGNAL(addedBuildConfiguration(ProjectExplorer::Project *, QString)),
            this, SLOT(addedBuildConfiguration(ProjectExplorer::Project *, QString)));
    connect(p, SIGNAL(removedBuildConfiguration(ProjectExplorer::Project *, QString)),
            this, SLOT(removedBuildConfiguration(ProjectExplorer::Project *, QString)));
    connect(this, SIGNAL(activated(int)),
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
    setItemText(index, m_project->displayNameFor(buildConfiguration));
}

int BuildConfigurationComboBox::nameToIndex(const QString &buildConfiguration)
{
    for (int i=0; i < count(); ++i)
        if (itemData(i) == buildConfiguration)
            return i;
    return -1;
}

void BuildConfigurationComboBox::activeConfigurationChanged()
{
    int index = nameToIndex(m_project->activeBuildConfiguration());
    if (index == -1)
        return;
    ignoreIndexChange = true;
    setCurrentIndex(index);
    ignoreIndexChange = false;
}

void BuildConfigurationComboBox::addedBuildConfiguration(ProjectExplorer::Project *,const QString &buildConfiguration)
{
    ignoreIndexChange = true;
    addItem(m_project->displayNameFor(buildConfiguration), buildConfiguration);
    ignoreIndexChange = false;
}

void BuildConfigurationComboBox::removedBuildConfiguration(ProjectExplorer::Project *, const QString &buildConfiguration)
{
    ignoreIndexChange = true;
    int index = nameToIndex(buildConfiguration);
    removeItem(index);
    ignoreIndexChange = false;
}

void BuildConfigurationComboBox::changedIndex(int newIndex)
{
    if (newIndex == -1)
        return;
    m_project->setActiveBuildConfiguration(itemData(newIndex).toString());
}
///
// ProjectComboBox
///

ProjectComboBox::ProjectComboBox(QWidget *parent)
    : QComboBox(parent), m_lastProject(0)
{
    setSizeAdjustPolicy(QComboBox::AdjustToContents);
    SessionManager *session = ProjectExplorerPlugin::instance()->session();

    foreach(Project *p, session->projects()) {
        addItem(p->name(), QVariant::fromValue((void *) p));
    }

    connect(session, SIGNAL(projectRemoved(ProjectExplorer::Project*)),
            this, SLOT(projectRemoved(ProjectExplorer::Project*)));
    connect(session, SIGNAL(projectAdded(ProjectExplorer::Project*)),
            this, SLOT(projectAdded(ProjectExplorer::Project*)));

    connect(this, SIGNAL(activated(int)),
            SLOT(itemActivated(int)));
}

ProjectComboBox::~ProjectComboBox()
{

}

void ProjectComboBox::projectAdded(ProjectExplorer::Project *p)
{
    addItem(p->name(), QVariant::fromValue((void *) p));
    // Comboboxes don't emit a signal
    if (count() == 1)
        itemActivated(0);
}

void ProjectComboBox::projectRemoved(ProjectExplorer::Project *p)
{
    QList<Project *> projects = ProjectExplorerPlugin::instance()->session()->projects();
    for (int i= 0; i<projects.count(); ++i)
        if (itemData(i, Qt::UserRole).value<void *>() == (void *) p) {
            removeItem(i);
            break;
    }

    // Comboboxes don't emit a signal if the index did't actually change
    if (count() == 0) {
        itemActivated(-1);
    } else {
        setCurrentIndex(0);
        itemActivated(0);
    }
}

void ProjectComboBox::itemActivated(int index)
{
    Project *p = 0;
    QList<Project *> projects = ProjectExplorerPlugin::instance()->session()->projects();
    if (index != -1 && index < projects.size())
        p = projects.at(index);

    if (p != m_lastProject) {
        m_lastProject = p;
        emit projectChanged(p);
    }
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
        p.fillRect(e->rect(), QBrush(StyleHelper::borderColor()));
    }
};

///
// ProjectWindow
///

ProjectWindow::ProjectWindow(QWidget *parent)
    : QWidget(parent), m_currentItemChanged(false)
{
    m_projectExplorer = ProjectExplorerPlugin::instance();
    m_session = m_projectExplorer->session();

    m_panelsWidget = new PanelsWidget(this);

    m_activeConfigurationWidget = new ActiveConfigurationWidget(m_panelsWidget);

    m_projectChooser = new QWidget(m_panelsWidget);
    QHBoxLayout *hbox = new QHBoxLayout(m_projectChooser);
    hbox->setMargin(0);
    hbox->addWidget(new QLabel(tr("Edit Configuration for Project:"), m_projectChooser));
    ProjectComboBox *projectComboBox = new ProjectComboBox(m_projectChooser);
    hbox->addWidget(projectComboBox);

    m_panelsWidget->addWidget(tr("Active Configuration"), m_activeConfigurationWidget);

    m_spacerBetween = new QWidget(this);
    QVBoxLayout *vbox = new QVBoxLayout(m_spacerBetween);
    vbox->setMargin(0);
    m_spacerBetween->setLayout(vbox);
    vbox->addSpacerItem(new QSpacerItem(10, 15, QSizePolicy::Fixed, QSizePolicy::Fixed));
    vbox->addWidget(new OnePixelBlackLine(m_spacerBetween));
    vbox->addSpacerItem(new QSpacerItem(10, 15, QSizePolicy::Fixed, QSizePolicy::Fixed));

    m_panelsWidget->addWidget(m_spacerBetween);

    m_panelsWidget->addWidget(tr("Edit Configuration"), m_projectChooser);

    QVBoxLayout *topLevelLayout = new QVBoxLayout(this);
    topLevelLayout->setMargin(0);
    topLevelLayout->setSpacing(0);
    topLevelLayout->addWidget(new Core::Utils::StyledBar(this));

    topLevelLayout->addWidget(m_panelsWidget);

    connect(projectComboBox, SIGNAL(projectChanged(ProjectExplorer::Project*)),
            this, SLOT(showProperties(ProjectExplorer::Project*)));

    connect(m_session, SIGNAL(sessionLoaded()), this, SLOT(restoreStatus()));
    connect(m_session, SIGNAL(aboutToSaveSession()), this, SLOT(saveStatus()));
}

ProjectWindow::~ProjectWindow()
{
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

    m_panelsWidget->addWidget(tr("Active Configuration"), m_activeConfigurationWidget);
    m_panelsWidget->addWidget(m_spacerBetween);
    m_panelsWidget->addWidget(tr("Edit Configuration"), m_projectChooser);

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

Project *ProjectWindow::findProject(const QString &path) const
{
    QList<Project*> projects = m_session->projects();
    foreach (Project* project, projects)
        if (QFileInfo(project->file()->fileName()).filePath() == path)
            return project;
    return 0;
}
