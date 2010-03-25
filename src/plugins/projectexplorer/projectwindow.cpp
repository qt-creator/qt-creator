/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "doubletabwidget.h"

#include "project.h"
#include "environment.h"
#include "projectexplorer.h"
#include "projectexplorerconstants.h"
#include "iprojectproperties.h"
#include "session.h"
#include "target.h"
#include "projecttreewidget.h"
#include "runconfiguration.h"
#include "buildsettingspropertiespage.h"
#include "runsettingspropertiespage.h"
#include "targetsettingspanel.h"

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

} // anonymous namespace

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
        QColor fillColor = Utils::StyleHelper::mergedColors(
                palette().button().color(), Qt::black, 80);
        p.fillRect(contentsRect(), fillColor);
    }
};

class RootWidget : public QWidget
{
public:
    RootWidget(QWidget *parent) : QWidget(parent) {}
    void paintEvent(QPaintEvent *);
};

void RootWidget::paintEvent(QPaintEvent *e)
{
    QWidget::paintEvent(e);

    QPainter painter(this);
    QColor light = Utils::StyleHelper::mergedColors(
            palette().button().color(), Qt::white, 30);
    QColor dark = Utils::StyleHelper::mergedColors(
            palette().button().color(), Qt::black, 85);

    painter.setPen(light);
    painter.drawLine(rect().topRight(), rect().bottomRight());
    painter.setPen(dark);
    painter.drawLine(rect().topRight() - QPoint(1,0), rect().bottomRight() - QPoint(1,0));
}

///
// PanelsWidget
///

PanelsWidget::PanelsWidget(QWidget *parent) :
    QScrollArea(parent),
    m_root(new RootWidget(this))
{
    // We want a 900px wide widget with and the scrollbar at the
    // side of the screen.
    m_root->setFixedWidth(900);
    m_root->setContentsMargins(0, 0, 40, 0);
    
    QPalette pal = m_root->palette();
    QColor background = Utils::StyleHelper::mergedColors(
            palette().window().color(), Qt::white, 85);
    pal.setColor(QPalette::All, QPalette::Window, background.darker(102));
    setPalette(pal);
    pal.setColor(QPalette::All, QPalette::Window, background);
    m_root->setPalette(pal);
    // The layout holding the individual panels:
    m_layout = new QGridLayout(m_root);
    m_layout->setColumnMinimumWidth(0, ICON_SIZE + 4);
    m_layout->setSpacing(0);

    setWidget(m_root);
    setFrameStyle(QFrame::NoFrame);
    setWidgetResizable(true);
}

PanelsWidget::~PanelsWidget()
{
    qDeleteAll(m_panels);
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
void PanelsWidget::addPropertiesPanel(IPropertiesPanel *panel)
{
    QTC_ASSERT(panel, return);

    const int headerRow(m_layout->rowCount() - 1);
    m_layout->setRowStretch(headerRow, 0);

    // icon:
    if (!panel->icon().isNull()) {
        QLabel *iconLabel = new QLabel(m_root);
        iconLabel->setPixmap(panel->icon().pixmap(ICON_SIZE, ICON_SIZE));
        iconLabel->setContentsMargins(0, ABOVE_HEADING_MARGIN, 0, 0);
        m_layout->addWidget(iconLabel, headerRow, 0, 3, 1, Qt::AlignTop | Qt::AlignHCenter);
    }

    // name:
    QLabel *nameLabel = new QLabel(m_root);
    nameLabel->setText(panel->displayName());
    QPalette palette = nameLabel->palette();
    palette.setBrush(QPalette::All, QPalette::Foreground, QColor(0, 0, 0, 110));
    nameLabel->setPalette(palette);
    nameLabel->setContentsMargins(0, ABOVE_HEADING_MARGIN, 0, 0);
    QFont f = nameLabel->font();
    f.setBold(true);
    f.setPointSizeF(f.pointSizeF() * 1.6);
    nameLabel->setFont(f);
    m_layout->addWidget(nameLabel, headerRow, 1, 1, 1, Qt::AlignVCenter | Qt::AlignLeft);

    // line:
    const int lineRow(headerRow + 1);
    QWidget *line = new OnePixelBlackLine(m_root);
    m_layout->addWidget(line, lineRow, 1, 1, -1, Qt::AlignTop);

    // add the widget:
    const int widgetRow(lineRow + 1);
    addPanelWidget(panel, widgetRow);
}

void PanelsWidget::addPanelWidget(IPropertiesPanel *panel, int row)
{
    QWidget *widget = panel->widget();
    int leftMargin = (panel->flags() & IPropertiesPanel::NoLeftMargin)
                     ? 0 : Constants::PANEL_LEFT_MARGIN;
    widget->setContentsMargins(leftMargin,
                               ABOVE_CONTENTS_MARGIN, 0,
                               BELOW_CONTENTS_MARGIN);
    widget->setParent(m_root);
    m_layout->addWidget(widget, row, 0, 1, 2);

    const int stretchRow(row + 1);
    m_layout->setRowStretch(stretchRow, 10);

    m_panels.append(panel);
}

///
// ProjectWindow
///

ProjectWindow::ProjectWindow(QWidget *parent)
    : QWidget(parent),
    m_currentWidget(0)
{
    ProjectExplorer::SessionManager *session = ProjectExplorerPlugin::instance()->session();

    // Setup overall layout:
    QVBoxLayout *viewLayout = new QVBoxLayout(this);
    viewLayout->setMargin(0);
    viewLayout->setSpacing(0);

    m_tabWidget = new DoubleTabWidget(this);
    viewLayout->addWidget(m_tabWidget);

    // Setup our container for the contents:
    m_centralWidget = new QStackedWidget(this);
    viewLayout->addWidget(m_centralWidget);

    // connects:
    connect(m_tabWidget, SIGNAL(currentIndexChanged(int,int)),
            this, SLOT(showProperties(int,int)));

    connect(session, SIGNAL(sessionLoaded()), this, SLOT(restoreStatus()));
    connect(session, SIGNAL(aboutToSaveSession()), this, SLOT(saveStatus()));

    connect(session, SIGNAL(projectAdded(ProjectExplorer::Project*)),
            this, SLOT(registerProject(ProjectExplorer::Project*)));
    connect(session, SIGNAL(aboutToRemoveProject(ProjectExplorer::Project*)),
            this, SLOT(deregisterProject(ProjectExplorer::Project*)));

    connect(session, SIGNAL(startupProjectChanged(ProjectExplorer::Project*)),
            this, SLOT(startupProjectChanged(ProjectExplorer::Project *)));

    // Update properties to empty project for now:
    showProperties(-1, -1);
}

ProjectWindow::~ProjectWindow()
{
}

void ProjectWindow::shutdown()
{
    showProperties(-1, -1); // TODO that's a bit stupid, but otherwise stuff is still
                             // connected to the session
    disconnect(ProjectExplorerPlugin::instance()->session(), 0, this, 0);
}

void ProjectWindow::registerProject(ProjectExplorer::Project *project)
{
    if (!project || m_tabIndexToProject.contains(project))
        return;

    // find index to insert:
    int index = -1;
    for (int i = 0; i <= m_tabIndexToProject.count(); ++i) {
        if (i == m_tabIndexToProject.count() ||
            m_tabIndexToProject.at(i)->displayName() > project->displayName()) {
            index = i;
            break;
        }
    }

    QStringList subtabs;
    if (project->supportedTargetIds().count() <= 1) {
        // Show the target specific pages directly
        QList<ITargetPanelFactory *> factories =
                ExtensionSystem::PluginManager::instance()->getObjects<ITargetPanelFactory>();

        foreach (ITargetPanelFactory *factory, factories) {
            if (factory->supports(project->activeTarget()))
                subtabs << factory->displayName();
        }
    } else {
        // Use the Targets page
        subtabs << QCoreApplication::translate("TargetSettingsPanelFactory", "Targets");
    }

    // Add the project specific pages

    QList<IProjectPanelFactory *> factories = ExtensionSystem::PluginManager::instance()->getObjects<IProjectPanelFactory>();
    foreach (IProjectPanelFactory *panelFactory, factories) {
        if (panelFactory->supports(project))
            subtabs << panelFactory->displayName();
    }

    m_tabIndexToProject.insert(index, project);
    m_tabWidget->insertTab(index, project->displayName(), subtabs);

    connect(project, SIGNAL(supportedTargetIdsChanged()),
            this, SLOT(refreshProject()));
}

void ProjectWindow::deregisterProject(ProjectExplorer::Project *project)
{
    int index = m_tabIndexToProject.indexOf(project);
    if (index < 0)
        return;

    disconnect(project, SIGNAL(supportedTargetIdsChanged()),
               this, SLOT(refreshProject()));

    m_tabIndexToProject.removeAt(index);
    m_tabWidget->removeTab(index);
}

void ProjectWindow::restoreStatus()
{
    // TODO
}

void ProjectWindow::saveStatus()
{
    // TODO
}

void ProjectWindow::refreshProject()
{
    Project *project = qobject_cast<ProjectExplorer::Project *>(sender());
    if (!m_tabIndexToProject.contains(project))
        return;

    // TODO this changes the subindex
    int index = m_tabWidget->currentIndex();
    deregisterProject(project);
    registerProject(project);
    m_tabWidget->setCurrentIndex(index);
}

void ProjectWindow::startupProjectChanged(ProjectExplorer::Project *p)
{
    int index = m_tabIndexToProject.indexOf(p);
    m_tabWidget->setCurrentIndex(index);
}

void ProjectWindow::showProperties(int index, int subIndex)
{
    if (index < 0 || index >= m_tabIndexToProject.count())
        return;

    Project *project = m_tabIndexToProject.at(index);

    // Set up custom panels again:
    int pos = 0;
    IPanelFactory *fac = 0;
    if (project->supportedTargetIds().count() > 1) {
        if (subIndex == 0) {
            // Targets page
            removeCurrentWidget();
            m_currentWidget = new TargetSettingsPanelWidget(project);
            m_centralWidget->addWidget(m_currentWidget);
            m_centralWidget->setCurrentWidget(m_currentWidget);
        }
        ++pos;
    } else {
        // No Targets page, target specific pages are first in the list
        foreach (ITargetPanelFactory *panelFactory, ExtensionSystem::PluginManager::instance()->getObjects<ITargetPanelFactory>()) {
            if (panelFactory->supports(project->activeTarget())) {
                if (subIndex == pos) {
                    fac = panelFactory;
                    break;
                }
                ++pos;
            }
        }
    }

    if (!fac) {
        foreach (IProjectPanelFactory *panelFactory, ExtensionSystem::PluginManager::instance()->getObjects<IProjectPanelFactory>()) {
            if (panelFactory->supports(project)) {
                if (subIndex == pos) {
                    fac = panelFactory;
                    break;
                }
                ++pos;
            }
        }
    }

    if (fac) {
        removeCurrentWidget();

        IPropertiesPanel *panel;
        if (ITargetPanelFactory *ipf = qobject_cast<ITargetPanelFactory *>(fac))
            panel = ipf->createPanel(project->activeTarget());
        else if (IProjectPanelFactory *ipf = qobject_cast<IProjectPanelFactory *>(fac))
            panel = ipf->createPanel(project);
        Q_ASSERT(panel);

        PanelsWidget *panelsWidget = new PanelsWidget(m_centralWidget);
        panelsWidget->addPropertiesPanel(panel);
        m_currentWidget = panelsWidget;
        m_centralWidget->addWidget(m_currentWidget);
        m_centralWidget->setCurrentWidget(m_currentWidget);

    }

    ProjectExplorerPlugin::instance()->session()->setStartupProject(project);
}

void ProjectWindow::removeCurrentWidget()
{
    if (m_currentWidget) {
        m_centralWidget->removeWidget(m_currentWidget);
        if (m_currentWidget) {
            delete m_currentWidget;
            m_currentWidget = 0;
        }
    }
}
