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
#include "projectexplorer.h"
#include "projectexplorerconstants.h"
#include "iprojectproperties.h"
#include "session.h"
#include "projecttreewidget.h"

#include <coreplugin/minisplitter.h>
#include <coreplugin/fileiconprovider.h>
#include <coreplugin/icore.h>
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

    m_layout->insertWidget(m_layout->count() -1, p.nameLabel);
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

///
// ProjectView
///

ProjectView::ProjectView(QWidget *parent)
    : QTreeWidget(parent)
{
    m_sizeHint = QSize(250, 250);
    setUniformRowHeights(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    QAbstractItemModel *m = model();
    connect(m, SIGNAL(rowsInserted(QModelIndex,int,int)),
            this, SLOT(updateSizeHint()));
    connect(m, SIGNAL(rowsRemoved(QModelIndex,int,int)),
            this, SLOT(updateSizeHint()));
    connect(m, SIGNAL(modelReset()),
            this, SLOT(updateSizeHint()));
    connect(m, SIGNAL(layoutChanged()),
            this, SLOT(updateSizeHint()));
    updateSizeHint();
}

ProjectView::~ProjectView()
{

}

QSize ProjectView::sizeHint() const
{
    return m_sizeHint;
}

void ProjectView::updateSizeHint()
{
    if (!model()) {
        m_sizeHint = QSize(250, 250);
        return;
    }

    int heightOffset = size().height() - viewport()->height();
    int heightPerRow = sizeHintForRow(0);
    if (heightPerRow == -1) {
        heightPerRow = 30;
    }
    int rows = qMin(qMax(model()->rowCount(), 2), 6);
    int height = rows * heightPerRow + heightOffset;
    if (m_sizeHint.height() != height) {
        m_sizeHint.setHeight(height);
        updateGeometry();
    }
}

///
// OnePixelBlackLine
///

#include <QtGui/QPainter>
#include <QtGui/QPaintEvent>
#include <utils/stylehelper.h>

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
    setWindowTitle(tr("Project Explorer"));
    setWindowIcon(QIcon(":/projectexplorer/images/projectexplorer.png"));

    m_projectExplorer = ProjectExplorerPlugin::instance();
    m_session = m_projectExplorer->session();

    m_treeWidget = new ProjectView(this);
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

    m_panelsWidget = new PanelsWidget(this);

    QVBoxLayout *topLevelLayout = new QVBoxLayout(this);
    topLevelLayout->setMargin(0);
    topLevelLayout->setSpacing(0);
    topLevelLayout->addWidget(new Core::Utils::StyledBar(this));
    topLevelLayout->addWidget(m_treeWidget);
    topLevelLayout->addWidget(new OnePixelBlackLine(this));
    topLevelLayout->addWidget(m_panelsWidget);

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
    if (!m_treeWidget->currentItem() && m_treeWidget->topLevelItemCount()) {
        m_treeWidget->setCurrentItem(m_treeWidget->topLevelItem(0), 0, QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
    }

    // TODO
//    const QVariant lastPanel = m_session->value(QLatin1String("ProjectWindow/Panel"));
//    if (lastPanel.isValid()) {
//        const int index = lastPanel.toInt();
//        if (index < m_panelsTabWidget->count())
//            m_panelsTabWidget->setCurrentIndex(index);
//    }
//
//    if ((m_panelsTabWidget->currentIndex() == -1) && m_panelsTabWidget->count())
//        m_panelsTabWidget->setCurrentIndex(0);
}

void ProjectWindow::saveStatus()
{
    // TODO
//    m_session->setValue(QLatin1String("ProjectWindow/Panel"), m_panelsTabWidget->currentIndex());
}

void ProjectWindow::showProperties(ProjectExplorer::Project *project, const QModelIndex & /* subIndex */)
{
    if (debug)
        qDebug() << "ProjectWindow - showProperties called";

    // Remove the tabs from the tab widget first
    m_panelsWidget->clear();

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

void ProjectWindow::updateTreeWidgetStatupProjectChanged(ProjectExplorer::Project *startupProject)
{
    int count = m_treeWidget->topLevelItemCount();
    for (int i = 0; i < count; ++i) {
        QTreeWidgetItem *item = m_treeWidget->topLevelItem(i);
        if (Project *project = findProject(item->data(2, Qt::UserRole).toString())) {
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
    item->setData(2, Qt::UserRole, fileInfo.filePath());
    item->setText(2, QDir::toNativeSeparators(fileInfo.filePath()));

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
        if (item->data(2, Qt::UserRole).toString() == QFileInfo(projectRemoved->file()->fileName()).filePath()) {
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
        if (item->data(2, Qt::UserRole).toString() == QFileInfo(projectRemoved->file()->fileName()).filePath()) {
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
    if (m_currentItemChanged)
        return;
    m_currentItemChanged = true;
    if (current) {
        QString path = current->data(2, Qt::UserRole).toString();
        if (Project *project = findProject(path)) {
            m_projectExplorer->setCurrentFile(project, path);
            showProperties(project, QModelIndex());
            m_currentItemChanged = false;
            return;
        }
    }
    showProperties(0, QModelIndex());
    m_currentItemChanged = false;
}


void ProjectWindow::handleItem(QTreeWidgetItem *item, int column)
{
    if (!item || column != 1) // startup project
        return;

    const QString path = item->data(2, Qt::UserRole).toString();
    Project *project = findProject(path);
    // Project no longer exists
    if (!project)
        return;
     if (!(item->checkState(1) == Qt::Checked)) { // is now unchecked
         if (m_session->startupProject() == project) {
             item->setCheckState(1, Qt::Checked); // uncheck not supported
         }
     } else if (project && project->isApplication()) { // is now checked
         m_session->setStartupProject(project);
     } else {
         item->setCheckState(1, Qt::Unchecked); // check not supported
     }
}
