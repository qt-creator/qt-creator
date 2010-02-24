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

#include "dependenciespanel.h"
#include "project.h"
#include "session.h"

#include <coreplugin/fileiconprovider.h>
#include <coreplugin/ifile.h>
#include <utils/detailswidget.h>

#include <QtCore/QVector>
#include <QtCore/QDebug>
#include <QtCore/QSize>
#include <QtCore/QCoreApplication>

#include <QtGui/QLabel>
#include <QtGui/QApplication>
#include <QtGui/QHBoxLayout>
#include <QtGui/QTreeView>
#include <QtGui/QSpacerItem>
#include <QtGui/QHeaderView>
#include <QtGui/QMessageBox>
#include <QtGui/QPushButton>
#include <QtGui/QToolButton>
#include <QtGui/QWidget>
#include <QtGui/QLabel>

namespace ProjectExplorer {
namespace Internal {

DependenciesModel::DependenciesModel(SessionManager *session,
                                     Project *project,
                                     QObject *parent)
    : QAbstractListModel(parent)
    , m_session(session)
    , m_project(project)
    , m_projects(session->projects())
{
    // We can't select ourselves as a dependency
    m_projects.removeAll(m_project);
    connect(session, SIGNAL(projectRemoved(ProjectExplorer::Project*)),
            this, SLOT(resetModel()));
    connect(session, SIGNAL(projectAdded(ProjectExplorer::Project*)),
            this, SLOT(resetModel()));
    connect(session, SIGNAL(sessionLoaded()),
            this, SLOT(resetModel()));
//    qDebug()<<"Dependencies Model"<<this<<"for project"<<project<<"("<<project->file()->fileName()<<")";
}

DependenciesModel::~DependenciesModel()
{
//    qDebug()<<"~DependenciesModel"<<this;
}

void DependenciesModel::resetModel()
{
    m_projects = m_session->projects();
    m_projects.removeAll(m_project);
    reset();
}

int DependenciesModel::rowCount(const QModelIndex &index) const
{
    return index.isValid() ? 0 : m_projects.isEmpty() ? 1 : m_projects.size();
}

int DependenciesModel::columnCount(const QModelIndex &index) const
{
    return index.isValid() ? 0 : 1;
}

QVariant DependenciesModel::data(const QModelIndex &index, int role) const
{
    if (m_projects.isEmpty())
        return role == Qt::DisplayRole
            ? tr("<No other projects in this session>")
            : QVariant();

    const Project *p = m_projects.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        return p->displayName();
    case Qt::CheckStateRole:
        return m_session->hasDependency(m_project, p) ? Qt::Checked : Qt::Unchecked;
    case Qt::DecorationRole:
        return Core::FileIconProvider::instance()->icon(QFileInfo(p->file()->fileName()));
    default:
        return QVariant();
    }
}

bool DependenciesModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role == Qt::CheckStateRole) {
        Project *p = m_projects.at(index.row());
        const Qt::CheckState c = static_cast<Qt::CheckState>(value.toInt());

        if (c == Qt::Checked) {
            if (m_session->addDependency(m_project, p)) {
                emit dataChanged(index, index);
                return true;
            } else {
                QMessageBox::warning(0, QCoreApplication::translate("DependenciesModel", "Unable to add dependency"),
                                     QCoreApplication::translate("DependenciesModel", "This would create a circular dependency."));
            }
        } else if (c == Qt::Unchecked) {
            if (m_session->hasDependency(m_project, p)) {
                m_session->removeDependency(m_project, p);
                emit dataChanged(index, index);
                return true;
            }
        }
    }
    return false;
}

Qt::ItemFlags DependenciesModel::flags(const QModelIndex &index) const
{
    if (m_projects.isEmpty())
        return Qt::NoItemFlags;

    Qt::ItemFlags rc = QAbstractListModel::flags(index);
    if (index.column() == 0)
        rc |= Qt::ItemIsUserCheckable | Qt::ItemIsEditable;
    return rc;
}

//
// DependenciesView
//
DependenciesView::DependenciesView(QWidget *parent)
    : QTreeView(parent)
{
    m_sizeHint = QSize(250, 250);
    setUniformRowHeights(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);
    setRootIsDecorated(false);
}

DependenciesView::~DependenciesView()
{

}

QSize DependenciesView::sizeHint() const
{
    return m_sizeHint;
}

void DependenciesView::setModel(QAbstractItemModel *newModel)
{
    if (QAbstractItemModel *oldModel = model()) {
        disconnect(oldModel, SIGNAL(rowsInserted(QModelIndex,int,int)),
                this, SLOT(updateSizeHint()));
        disconnect(oldModel, SIGNAL(rowsRemoved(QModelIndex,int,int)),
                this, SLOT(updateSizeHint()));
        disconnect(oldModel, SIGNAL(modelReset()),
                this, SLOT(updateSizeHint()));
        disconnect(oldModel, SIGNAL(layoutChanged()),
                this, SLOT(updateSizeHint()));
    }

    QTreeView::setModel(newModel);

    if (newModel) {
        connect(newModel, SIGNAL(rowsInserted(QModelIndex,int,int)),
                this, SLOT(updateSizeHint()));
        connect(newModel, SIGNAL(rowsRemoved(QModelIndex,int,int)),
                this, SLOT(updateSizeHint()));
        connect(newModel, SIGNAL(modelReset()),
                this, SLOT(updateSizeHint()));
        connect(newModel, SIGNAL(layoutChanged()),
                this, SLOT(updateSizeHint()));
    }
    updateSizeHint();
}

void DependenciesView::updateSizeHint()
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
    int rows = qMin(qMax(model()->rowCount(), 2), 10);
    int height = rows * heightPerRow + heightOffset;
    if (m_sizeHint.height() != height) {
        m_sizeHint.setHeight(height);
        updateGeometry();
    }
}

//
// DependenciesWidget
//

DependenciesWidget::DependenciesWidget(SessionManager *session,
                                       Project *project,
                                       QWidget *parent)
    : QWidget(parent)
    , m_session(session)
    , m_project(project)
    , m_model(new DependenciesModel(session, project, this))
{
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(0, 0, 0, 0);
    m_detailsContainer = new Utils::DetailsWidget(this);
    m_detailsContainer->setState(Utils::DetailsWidget::NoSummary);
    vbox->addWidget(m_detailsContainer);

    QWidget *detailsWidget = new QWidget(m_detailsContainer);
    m_detailsContainer->setWidget(detailsWidget);
    QHBoxLayout *layout = new QHBoxLayout(detailsWidget);
    layout->setContentsMargins(0, -1, 0, -1);
    DependenciesView *treeView = new DependenciesView(this);
    treeView->setModel(m_model);
    treeView->setHeaderHidden(true);
    layout->addWidget(treeView);
    layout->addSpacerItem(new QSpacerItem(0, 0 , QSizePolicy::Expanding, QSizePolicy::Fixed));
}

//
// DependenciesPanel
//

DependenciesPanel::DependenciesPanel(SessionManager *session, Project *project) :
    m_widget(new DependenciesWidget(session, project)),
    m_icon(":/projectexplorer/images/ProjectDependencies.png")
{
}

DependenciesPanel::~DependenciesPanel()
{
    delete m_widget;
}

QString DependenciesPanel::displayName() const
{
    return QCoreApplication::translate("DependenciesPanel", "Dependencies");
}

QWidget *DependenciesPanel::widget() const
{
    return m_widget;
}

QIcon DependenciesPanel::icon() const
{
    return m_icon;
}

//
// DependenciesPanelFactory
//

DependenciesPanelFactory::DependenciesPanelFactory(SessionManager *session)
    : m_session(session)
{
}

QString DependenciesPanelFactory::id() const
{
    return QLatin1String(DEPENDENCIES_PANEL_ID);
}

QString DependenciesPanelFactory::displayName() const
{
    return QCoreApplication::translate("DependenciesPanelFactory", "Dependencies");
}

bool DependenciesPanelFactory::supports(Project *project)
{
    Q_UNUSED(project);
    return true;
}

bool DependenciesPanelFactory::supports(Target *target)
{
    Q_UNUSED(target);
    return false;
}

IPropertiesPanel *DependenciesPanelFactory::createPanel(Project *project)
{
    return new DependenciesPanel(m_session, project);
}

IPropertiesPanel *DependenciesPanelFactory::createPanel(Target *target)
{
    Q_UNUSED(target);
    return 0;
}

} // namespace Internal
} // namespace ProjectExplorer
