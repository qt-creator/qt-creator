/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "projectwindow.h"

#include "project.h"
#include "projectexplorer.h"
#include "projectexplorerconstants.h"
#include "iprojectproperties.h"
#include "session.h"
#include "projecttreewidget.h"

#include <coreplugin/minisplitter.h>
#include <coreplugin/fileiconprovider.h>
#include <coreplugin/icore.h>
#include <extensionsystem/pluginmanager.h>

#include <QtCore/QDebug>
#include <QtGui/QApplication>
#include <QtGui/QBoxLayout>
#include <QtGui/QComboBox>
#include <QtGui/QTabWidget>
#include <QtGui/QToolBar>
#include <QtGui/QTreeWidget>
#include <QtGui/QHeaderView>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

namespace {
bool debug = false;
}

ProjectWindow::ProjectWindow(QWidget *parent) : QWidget(parent)
{
    setWindowTitle(tr("Project Explorer"));
    setWindowIcon(QIcon(":/projectexplorer/images/projectexplorer.png"));

    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    ProjectExplorerPlugin *projectExplorer = m_projectExplorer = pm->getObject<ProjectExplorerPlugin>();
    m_session = projectExplorer->session();

    connect(m_session, SIGNAL(sessionLoaded()), this, SLOT(restoreStatus()));
    connect(m_session, SIGNAL(aboutToSaveSession()), this, SLOT(saveStatus()));

    m_treeWidget = new QTreeWidget(this);
    m_treeWidget->setFrameStyle(QFrame::NoFrame);
    m_treeWidget->setRootIsDecorated(false);
    m_treeWidget->header()->setResizeMode(QHeaderView::ResizeToContents);
    m_treeWidget->setHeaderLabels(QStringList()
                                  << tr("Projects")
                                  << tr("Startup")
                                  << tr("Path")
        );

    connect(m_treeWidget, SIGNAL(itemChanged(QTreeWidgetItem*, int)),
            this, SLOT(handleItem(QTreeWidgetItem*, int)), Qt::QueuedConnection);
    connect(m_treeWidget, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem *)),
            this, SLOT(handleCurrentItemChanged(QTreeWidgetItem*)));

    QWidget *panelsWidget = new QWidget;
    m_panelsTabWidget = new QTabWidget;
    m_panelsTabWidget->setDocumentMode(true);
    QVBoxLayout *panelsLayout = new QVBoxLayout(panelsWidget);

    panelsLayout->setSpacing(0);
    panelsLayout->setContentsMargins(0, panelsLayout->margin(), 0, 0);
    panelsLayout->addWidget(m_panelsTabWidget);

    QWidget *dummy = new QWidget;
    QVBoxLayout *dummyLayout = new QVBoxLayout(dummy);
    dummyLayout->setMargin(0);
    dummyLayout->setSpacing(0);
    dummyLayout->addWidget(new QToolBar(dummy));
    dummyLayout->addWidget(m_treeWidget);

    QSplitter *splitter = new Core::MiniSplitter;
    splitter->setOrientation(Qt::Vertical);
    splitter->addWidget(dummy);
    splitter->addWidget(panelsWidget);



    // make sure that the tree treewidget has same size policy as qtabwidget
    m_treeWidget->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred));
    const int treeWidgetMinSize = m_treeWidget->minimumSizeHint().height();
    splitter->setSizes(QList<int>() << treeWidgetMinSize << splitter->height() - treeWidgetMinSize);

    QVBoxLayout *topLayout = new QVBoxLayout(this);
    topLayout->setMargin(0);
    topLayout->setSpacing(0);
    topLayout->addWidget(splitter);

    connect(m_session, SIGNAL(sessionLoaded()), this, SLOT(updateTreeWidget()));
    connect(m_session, SIGNAL(startupProjectChanged(ProjectExplorer::Project*)), this, SLOT(updateTreeWidget()));
    connect(m_session, SIGNAL(projectAdded(ProjectExplorer::Project*)), this, SLOT(updateTreeWidget()));
    connect(m_session, SIGNAL(projectRemoved(ProjectExplorer::Project*)), this, SLOT(updateTreeWidget()));
}

ProjectWindow::~ProjectWindow()
{
}

void ProjectWindow::restoreStatus()
{
    const QVariant lastPanel = m_session->value(QLatin1String("ProjectWindow/Panel"));
    if (lastPanel.isValid()) {
        const int index = lastPanel.toInt();
        if (index < m_panelsTabWidget->count())
            m_panelsTabWidget->setCurrentIndex(index);
    }
}

void ProjectWindow::saveStatus()
{
    m_session->setValue(QLatin1String("ProjectWindow/Panel"), m_panelsTabWidget->currentIndex());
}

void ProjectWindow::showProperties(ProjectExplorer::Project *project, const QModelIndex & /* subIndex */)
{
    if (debug)
        qDebug() << "ProjectWindow - showProperties called";

    // Remove the tabs from the tab widget first
    while (m_panelsTabWidget->count() > 0)
        m_panelsTabWidget->removeTab(0);

    while (m_panels.count()) {
        PropertiesPanel *panel = m_panels.at(0);
        m_panels.removeOne(panel);
        delete panel;
    }

    if (project) {
        QList<IPanelFactory *> pages =
                ExtensionSystem::PluginManager::instance()->getObjects<IPanelFactory>();
        foreach (IPanelFactory *panelFactory, pages) {
            if (panelFactory->supports(project)) {
                PropertiesPanel *panel = panelFactory->createPanel(project);
                if (debug)
                  qDebug() << "ProjectWindow - setting up project properties tab " << panel->name();

                m_panels.append(panel);
                m_panelsTabWidget->addTab(panel->widget(), panel->name());
            }
        }
    }
}

void ProjectWindow::updateTreeWidget()
{
    // This setFocus prevents a crash, which I (daniel) spend the better part of a day tracking down.
    // To explain: Consider the case that a widget on either the build or run settings has Focus
    // Us clearing the m_treewidget will emit a currentItemChanged(0) signal
    // Which is connected to showProperties
    // showProperties will now remove the widget that has focus from m_panelsTabWidget, so the treewidget
    // gets focus, which will in focusIn select the first entry (due to QTreeWidget::clear() implementation,
    // there are still items in the model) which emits another currentItemChanged() signal
    // That one runs fully thorough and deletes all widgets, even that one that we are currently removing
    // from m_panelsTabWidget.
    // To prevent that, we simply prevent the focus switching....
    QWidget *focusWidget = qApp->focusWidget();
    while (focusWidget) {
        if (focusWidget == this) {
            m_treeWidget->setFocus();
            break;
        }
        focusWidget = focusWidget->parentWidget();
    }
    m_treeWidget->clear();

    foreach(Project *project, m_session->projects()) {
        const QFileInfo fileInfo(project->file()->fileName());

        QTreeWidgetItem *item = new QTreeWidgetItem();
        item->setText(0, fileInfo.baseName());
        item->setIcon(0, Core::FileIconProvider::instance()->icon(fileInfo));
        item->setText(2, fileInfo.filePath());

        if (project->isApplication()) {
            bool checked = (m_session->startupProject() == project);
            item->setCheckState(1, checked ? Qt::Checked : Qt::Unchecked);
        }

        m_treeWidget->addTopLevelItem(item);

        if (m_projectExplorer->currentProject() == project) {
            m_treeWidget->setCurrentItem(item, 0, QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
        }
    }


    if  (!m_treeWidget->currentItem()) {
        if (m_treeWidget->topLevelItemCount() > 0)
            m_treeWidget->setCurrentItem(m_treeWidget->topLevelItem(0), 0, QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
        else
            handleCurrentItemChanged(0);
    }


    // Hack around Qt bug
    m_treeWidget->viewport()->update();
}


Project *ProjectWindow::findProject(const QString &path) const
{
    QList<Project*> projects = m_session->projects();
    foreach (Project* project, projects)
        if (project->file()->fileName() == path)
            return project;
    return 0;
}


void ProjectWindow::handleCurrentItemChanged(QTreeWidgetItem *current)
{
    if (current) {
        QString path = current->text(2);
        if (Project *project = findProject(path)) {
            m_projectExplorer->setCurrentFile(project, path);
            showProperties(project, QModelIndex());
            return;
        }
    }

    // we only get here if either current is zero or we didn't find a project for the path
    m_projectExplorer->setCurrentFile(0, QString());
    showProperties(0, QModelIndex());
}


void ProjectWindow::handleItem(QTreeWidgetItem *item, int column)
{
    if (!item || column != 1) // startup project
        return;

    const QString path = item->text(2);
    Project *project = findProject(path);

    if (project && project->isApplication()) {
        if (!(item->checkState(1) == Qt::Checked)) {
            item->setCheckState(1, Qt::Checked); // uncheck not supported
        } else {
            m_session->setStartupProject(project);
        }
    }
}
