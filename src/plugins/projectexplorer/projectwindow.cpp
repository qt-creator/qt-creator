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

    m_projectExplorer = ProjectExplorerPlugin::instance();
    m_session = m_projectExplorer->session();

    m_treeWidget = new QTreeWidget(this);
    m_treeWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_treeWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_treeWidget->setFrameStyle(QFrame::NoFrame);
    m_treeWidget->setRootIsDecorated(false);
    m_treeWidget->header()->setResizeMode(QHeaderView::ResizeToContents);
    m_treeWidget->setHeaderLabels(QStringList()
                                  << tr("Projects")
                                  << tr("Startup")
                                  << tr("Path")
        );

    connect(m_treeWidget, SIGNAL(itemChanged(QTreeWidgetItem*, int)),
            this, SLOT(handleItem(QTreeWidgetItem*, int)));
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

    connect(m_session, SIGNAL(sessionLoaded()), this, SLOT(restoreStatus()));
    connect(m_session, SIGNAL(aboutToSaveSession()), this, SLOT(saveStatus()));

    connect(m_session, SIGNAL(startupProjectChanged(ProjectExplorer::Project*)), this, SLOT(updateTreeWidgetStatupProjectChanged(ProjectExplorer::Project*)));
    connect(m_session, SIGNAL(projectAdded(ProjectExplorer::Project*)), this, SLOT(updateTreeWidgetProjectAdded(ProjectExplorer::Project*)));
    connect(m_session, SIGNAL(projectRemoved(ProjectExplorer::Project*)), this, SLOT(updateTreeWidgetProjectRemoved(ProjectExplorer::Project*)));
    connect(m_session, SIGNAL(aboutToRemoveProject(ProjectExplorer::Project*)), this, SLOT(updateTreeWidgetAboutToRemoveProject(ProjectExplorer::Project*)));

}

ProjectWindow::~ProjectWindow()
{
}

void ProjectWindow::restoreStatus()
{
    m_panelsTabWidget->setFocus();

    if (!m_treeWidget->currentItem() && m_treeWidget->topLevelItemCount()) {
        m_treeWidget->setCurrentItem(m_treeWidget->topLevelItem(0), 0, QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
    }

    const QVariant lastPanel = m_session->value(QLatin1String("ProjectWindow/Panel"));
    if (lastPanel.isValid()) {
        const int index = lastPanel.toInt();
        if (index < m_panelsTabWidget->count())
            m_panelsTabWidget->setCurrentIndex(index);
    }

    if ((m_panelsTabWidget->currentIndex() == -1) && m_panelsTabWidget->count())
        m_panelsTabWidget->setCurrentIndex(0);
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

void ProjectWindow::updateTreeWidgetStatupProjectChanged(ProjectExplorer::Project *startupProject)
{
    int count = m_treeWidget->topLevelItemCount();
    for (int i = 0; i < count; ++i) {
        QTreeWidgetItem *item = m_treeWidget->topLevelItem(i);
        if (Project *project = findProject(item->text(2))) {
            bool checked = (startupProject == project);
            if (item->checkState(1) != (checked ? Qt::Checked : Qt::Unchecked))
                item->setCheckState(1, checked ? Qt::Checked : Qt::Unchecked);
        } else {
            item->setCheckState(1, Qt::Unchecked);
        }
    }
}

void ProjectWindow::updateTreeWidgetProjectAdded(ProjectExplorer::Project *projectAdded)
{
    int position = m_session->projects().indexOf(projectAdded);
    const QFileInfo fileInfo(projectAdded->file()->fileName());

    QTreeWidgetItem *item = new QTreeWidgetItem();
    item->setText(0, projectAdded->name());
    item->setIcon(0, Core::FileIconProvider::instance()->icon(fileInfo));
    item->setText(2, fileInfo.filePath());

    if (projectAdded->isApplication()) {
        bool checked = (m_session->startupProject() == projectAdded);
        item->setCheckState(1, checked ? Qt::Checked : Qt::Unchecked);
    }

    m_treeWidget->insertTopLevelItem(position, item);
}

void ProjectWindow::updateTreeWidgetAboutToRemoveProject(ProjectExplorer::Project *projectRemoved) {
    int count = m_treeWidget->topLevelItemCount();
    for (int i = 0; i < count; ++i) {
        QTreeWidgetItem *item = m_treeWidget->topLevelItem(i);
        if (item->text(2) == QFileInfo(projectRemoved->file()->fileName()).filePath()) {
            if (m_treeWidget->currentItem() == item) {
                    m_treeWidget->setCurrentItem(0);
            }
        }
    }
}

void ProjectWindow::updateTreeWidgetProjectRemoved(ProjectExplorer::Project *projectRemoved)
{    
    int count = m_treeWidget->topLevelItemCount();
    for (int i = 0; i < count; ++i) {
        QTreeWidgetItem *item = m_treeWidget->topLevelItem(i);
        if (item->text(2) == QFileInfo(projectRemoved->file()->fileName()).filePath()) {
            QTreeWidgetItem *it = m_treeWidget->takeTopLevelItem(i);
            delete it;
            break;
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


void ProjectWindow::handleCurrentItemChanged(QTreeWidgetItem *current)
{
    if (current) {
        QString path = current->text(2);
        if (Project *project = findProject(path)) {
            m_projectExplorer->setCurrentFile(project, path);
            showProperties(project, QModelIndex());
            return;
        }
    } else {
        showProperties(0, QModelIndex());
    }
}


void ProjectWindow::handleItem(QTreeWidgetItem *item, int column)
{

    if (!item || column != 1) // startup project
        return;

    const QString path = item->text(2);
    Project *project = findProject(path);
    if (project && project->isApplication()) {
        if (!(item->checkState(1) == Qt::Checked)) { // is now unchecked
            if (m_session->startupProject() == project) {
                item->setCheckState(1, Qt::Checked); // uncheck not supported
            }
        } else { // is now checked
            m_session->setStartupProject(project);
        }
    }
}
