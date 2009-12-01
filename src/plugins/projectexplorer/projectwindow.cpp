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
#include <utils/qtcassert.h>
#include <utils/styledbar.h>
#include <utils/stylehelper.h>

#include <QtGui/QApplication>
#include <QtGui/QBoxLayout>
#include <QtGui/QComboBox>
#include <QtGui/QScrollArea>
#include <QtGui/QLabel>
#include <QtGui/QPainter>
#include <QtGui/QPaintEvent>
#include <QtGui/QMenu>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

namespace {
const int ICON_SIZE(64);

const int ABOVE_HEADING_MARGIN(10);
const int ABOVE_CONTENTS_MARGIN(4);
const int BELOW_CONTENTS_MARGIN(16);
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
        Q_UNUSED(e);
        QPainter p(this);
        p.fillRect(contentsRect(), QBrush(Utils::StyleHelper::borderColor()));
    }
};

///
// PanelsWidget::Panel
///

PanelsWidget::Panel::Panel(QWidget * w) :
    iconLabel(0), lineWidget(0), nameLabel(0), panelWidget(w)
{ }

PanelsWidget::Panel::~Panel()
{
    delete iconLabel;
    delete lineWidget;
    delete nameLabel;
    // do not delete panelWidget, we do not own it!
}

///
// PanelsWidget
///

PanelsWidget::PanelsWidget(QWidget *parent) :
    QScrollArea(parent),
    m_root(new QWidget(this))
{
    // We want a 800px wide widget with and the scrollbar at the
    // side of the screen.
    m_root->setMaximumWidth(800);
    // The layout holding the individual panels:
    m_layout = new QGridLayout(m_root);
    m_layout->setColumnMinimumWidth(0, ICON_SIZE + 4);
    m_layout->setSpacing(0);
    m_layout->setRowStretch(0, 10);

    // Add horizontal space to the left of our widget:
    QHBoxLayout *hbox = new QHBoxLayout;
    hbox->setMargin(0);
    hbox->setSpacing(0);

    hbox->addWidget(m_root);
    hbox->addStretch(10);

    // create a widget to hold the scrollarea:
    QWidget * scrollArea = new QWidget();
    scrollArea->setLayout(hbox);

    setWidget(scrollArea);
    setFrameStyle(QFrame::NoFrame);
    setWidgetResizable(true);
}

PanelsWidget::~PanelsWidget()
{
    clear();
}

/*
 * Add a widget into the grid layout of the PanelsWidget.
 *
 *     ...
 * +--------+-------------------------------------------+ ABOVE_CONTENTS_MARGIN
 * |          widget (with contentsmargins adjusted!)   |
 * +--------+-------------------------------------------+ BELOW_CONTENTS_MARGIN
 */
void PanelsWidget::addWidget(QWidget *widget)
{
    QTC_ASSERT(widget, return);

    const int row(m_layout->rowCount() - 1);
    m_layout->setRowStretch(row, 0);
    addPanelWidget(new Panel(widget), row);
}

/*
 * Add a widget with heading information into the grid
 * layout of the PanelsWidget.
 *
 *     ...
 * +--------+-------------------------------------------+ ABOVE_HEADING_MARGIN
 * | icon   | name                                      |
 * +        +-------------------------------------------+
 * |        | line                                      |
 * +--------+-------------------------------------------+ ABOVE_CONTENTS_MARGIN
 * |          widget (with contentsmargins adjusted!)   |
 * +--------+-------------------------------------------+ BELOW_CONTENTS_MARGIN
 */
void PanelsWidget::addWidget(const QString &name, QWidget *widget, const QIcon & icon)
{
    QTC_ASSERT(widget, return);

    Panel *p = new Panel(widget);

    // icon:
    const int headerRow(m_layout->rowCount() - 1);
    m_layout->setRowStretch(headerRow, 0);

    if (!icon.isNull()) {
        p->iconLabel = new QLabel(m_root);
        p->iconLabel->setPixmap(icon.pixmap(ICON_SIZE, ICON_SIZE));
        p->iconLabel->setContentsMargins(0, ABOVE_HEADING_MARGIN, 0, 0);
        m_layout->addWidget(p->iconLabel, headerRow, 0, 2, 1, Qt::AlignTop | Qt::AlignHCenter);
    }

    // name:
    p->nameLabel = new QLabel(m_root);
    p->nameLabel->setText(name);
    p->nameLabel->setContentsMargins(0, ABOVE_HEADING_MARGIN, 0, 0);
    QFont f = p->nameLabel->font();
    f.setBold(true);
    f.setPointSizeF(f.pointSizeF() * 1.4);
    p->nameLabel->setFont(f);
    m_layout->addWidget(p->nameLabel, headerRow, 1, 1, 1, Qt::AlignBottom | Qt::AlignLeft);

    // line:
    const int lineRow(headerRow + 1);
    p->lineWidget = new OnePixelBlackLine(m_root);
    m_layout->addWidget(p->lineWidget, lineRow, 1);

    // add the widget:
    const int widgetRow(lineRow + 1);
    addPanelWidget(p, widgetRow);
}

QWidget *PanelsWidget::rootWidget() const
{
    return m_root;
}

void PanelsWidget::clear()
{
    foreach (Panel* p, m_panels) {
        if (p->iconLabel)
            m_layout->removeWidget(p->iconLabel);
        if (p->lineWidget)
            m_layout->removeWidget(p->lineWidget);
        if (p->nameLabel)
            m_layout->removeWidget(p->nameLabel);
        if (p->panelWidget)
            m_layout->removeWidget(p->panelWidget);
        delete p;
    }
    m_panels.clear();
}

void PanelsWidget::addPanelWidget(Panel *panel, int row)
{
    panel->panelWidget->setContentsMargins(m_layout->columnMinimumWidth(0),
                                           ABOVE_CONTENTS_MARGIN, 0,
                                           BELOW_CONTENTS_MARGIN);
    m_layout->addWidget(panel->panelWidget, row, 0, 1, 2);

    const int stretchRow(row + 1);
    m_layout->setRowStretch(stretchRow, 10);

    m_panels.append(panel);
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
        foreach(RunConfiguration *rc, p->runConfigurations()) {
            connect(rc, SIGNAL(nameChanged()), this, SLOT(rebuildTree()));
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
            QList<RunConfiguration *> runconfigurations = p->runConfigurations();
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
    RunConfiguration *runConfiguration = 0;
    foreach(RunConfiguration *rc, p->runConfigurations()) {
        if (rc->name() == name) {
            runConfiguration = rc;
            break;
        }
    }
    if (runConfiguration) {
        connect(runConfiguration, SIGNAL(nameChanged()),
                this, SLOT(rebuildTree()));
    }
    rebuildTree();
}

void RunConfigurationComboBox::removedRunConfiguration(ProjectExplorer::Project *p, const QString &name)
{
    Q_UNUSED(p)
    Q_UNUSED(name)
    rebuildTree();
}

void RunConfigurationComboBox::projectAdded(ProjectExplorer::Project *p)
{
    rebuildTree();
    foreach(RunConfiguration *rc, p->runConfigurations())
        connect(rc, SIGNAL(nameChanged()), this, SLOT(rebuildTree()));
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
        foreach(RunConfiguration *rc, p->runConfigurations()) {
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
        m_comboBox->addItem(buildConfiguration->displayName(), buildConfiguration);
    if (p->buildConfigurations().count() == 1) {
        m_label->setText(m_comboBox->itemText(0));
        setCurrentWidget(m_label);
    }

    int index = p->buildConfigurations().indexOf(p->activeBuildConfiguration());
    if (index != -1)
        m_comboBox->setCurrentIndex(index);

    // TODO
//    connect(p, SIGNAL(buildConfigurationDisplayNameChanged(QString)),
    //            this, SLOT(nameChanged(ProjectExplorer::BuildConfiguration *)));
    connect(p, SIGNAL(activeBuildConfigurationChanged()),
            this, SLOT(activeConfigurationChanged()));
    connect(p, SIGNAL(addedBuildConfiguration(ProjectExplorer::Project*,ProjectExplorer::BuildConfiguration*)),
            this, SLOT(addedBuildConfiguration(ProjectExplorer::Project*,ProjectExplorer::BuildConfiguration*)));
    connect(p, SIGNAL(removedBuildConfiguration(ProjectExplorer::Project*,ProjectExplorer::BuildConfiguration*)),
            this, SLOT(removedBuildConfiguration(ProjectExplorer::Project*,ProjectExplorer::BuildConfiguration*)));
    connect(m_comboBox, SIGNAL(activated(int)),
            this, SLOT(changedIndex(int)));
}

BuildConfigurationComboBox::~BuildConfigurationComboBox()
{

}

void BuildConfigurationComboBox::nameChanged(BuildConfiguration *bc)
{
    const int index(buildConfigurationToIndex(bc));
    if (index == -1)
        return;
    const QString &displayName = bc->displayName();
    m_comboBox->setItemText(index, displayName);
    if (m_comboBox->count() == 1)
        m_label->setText(displayName);
}

int BuildConfigurationComboBox::buildConfigurationToIndex(BuildConfiguration *bc)
{
    for (int i=0; i < m_comboBox->count(); ++i)
        if (m_comboBox->itemData(i).value<BuildConfiguration *>() == bc)
            return i;
    return -1;
}

void BuildConfigurationComboBox::activeConfigurationChanged()
{
    const int index(buildConfigurationToIndex(m_project->activeBuildConfiguration()));
    if (index == -1)
        return;
    ignoreIndexChange = true;
    m_comboBox->setCurrentIndex(index);
    ignoreIndexChange = false;
}

void BuildConfigurationComboBox::addedBuildConfiguration(ProjectExplorer::Project *project,
                                                         BuildConfiguration *bc)
{
    Q_UNUSED(project);
    ignoreIndexChange = true;
    m_comboBox->addItem(bc->displayName(), QVariant::fromValue(bc));

    if (m_comboBox->count() == 2)
        setCurrentWidget(m_comboBox);
    ignoreIndexChange = false;
}

void BuildConfigurationComboBox::removedBuildConfiguration(ProjectExplorer::Project *project,
                                                           BuildConfiguration * bc)
{
    Q_UNUSED(project);
    ignoreIndexChange = true;
    const int index(buildConfigurationToIndex(bc));
    if (index == -1)
        return;
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
    m_project->setActiveBuildConfiguration(m_comboBox->itemData(newIndex).value<BuildConfiguration *>());
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
// ProjectWindow
///

ProjectWindow::ProjectWindow(QWidget *parent)
    : QWidget(parent)
{
    ProjectExplorer::SessionManager *session = ProjectExplorerPlugin::instance()->session();

    // Setup overall layout:
    QVBoxLayout *viewLayout = new QVBoxLayout(this);
    viewLayout->setMargin(0);
    viewLayout->setSpacing(0);

    // Add toolbar used everywhere
    viewLayout->addWidget(new Utils::StyledBar(this));

    // Setup our container for the contents:
    m_panelsWidget = new PanelsWidget(this);
    viewLayout->addWidget(m_panelsWidget);

    // Run and build configuration selection panel:
    m_activeConfigurationWidget = new ActiveConfigurationWidget(m_panelsWidget->rootWidget());

    // Spacer and line panel:
    m_spacerBetween = new OnePixelBlackLine(m_panelsWidget->rootWidget());

    // Project chooser panel:
    m_projectChooser = new QWidget(m_panelsWidget->rootWidget());
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

    // no projects label:
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
    viewLayout->addWidget(m_noprojectLabel);

    // show either panels or no projects label:
    bool noProjects = session->projects().isEmpty();
    m_panelsWidget->setVisible(!noProjects);
    m_noprojectLabel->setVisible(noProjects);

    // connects:
    connect(changeProject, SIGNAL(projectChanged(ProjectExplorer::Project*)),
            this, SLOT(showProperties(ProjectExplorer::Project*)));

    connect(session, SIGNAL(sessionLoaded()), this, SLOT(restoreStatus()));
    connect(session, SIGNAL(aboutToSaveSession()), this, SLOT(saveStatus()));

    connect(session, SIGNAL(projectAdded(ProjectExplorer::Project*)),
            this, SLOT(projectAdded()));
    connect(session, SIGNAL(projectRemoved(ProjectExplorer::Project*)),
            this, SLOT(projectRemoved()));

    // Update properties to empty project for now:
    showProperties(0);
}

ProjectWindow::~ProjectWindow()
{
    qDeleteAll(m_panels);
    m_panels.clear();
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
    // Remove all existing panels:
    m_panelsWidget->clear();

    // delete custom panels:
    qDeleteAll(m_panels);
    m_panels.clear();

    // Set up our default panels:
    m_panelsWidget->addWidget(tr("Active Build and Run Configurations"), m_activeConfigurationWidget, QIcon());
    m_panelsWidget->addWidget(m_spacerBetween);
    m_panelsWidget->addWidget(m_projectChooser);

    // Set up custom panels again:
    if (project) {
        QList<IPanelFactory *> pages =
                ExtensionSystem::PluginManager::instance()->getObjects<IPanelFactory>();
        foreach (IPanelFactory *panelFactory, pages) {
            if (panelFactory->supports(project)) {
                IPropertiesPanel *panel = panelFactory->createPanel(project);
                m_panelsWidget->addWidget(panel->name(), panel->widget(), panel->icon());
                m_panels.push_back(panel);
            }
        }
    }
}
