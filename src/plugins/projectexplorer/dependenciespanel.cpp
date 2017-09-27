/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "dependenciespanel.h"
#include "project.h"
#include "session.h"

#include <coreplugin/fileiconprovider.h>
#include <coreplugin/icore.h>

#include <utils/detailswidget.h>
#include <utils/algorithm.h>

#include <QDebug>
#include <QSize>
#include <QCoreApplication>

#include <QCheckBox>
#include <QGridLayout>
#include <QTreeView>
#include <QSpacerItem>
#include <QMessageBox>

namespace ProjectExplorer {
namespace Internal {

DependenciesModel::DependenciesModel(Project *project, QObject *parent)
    : QAbstractListModel(parent)
    , m_project(project)
{
    resetModel();

    SessionManager *sessionManager = SessionManager::instance();
    connect(sessionManager, &SessionManager::projectRemoved,
            this, &DependenciesModel::resetModel);
    connect(sessionManager, &SessionManager::projectAdded,
            this, &DependenciesModel::resetModel);
    connect(sessionManager, &SessionManager::sessionLoaded,
            this, &DependenciesModel::resetModel);
}

void DependenciesModel::resetModel()
{
    beginResetModel();
    m_projects = SessionManager::projects();
    m_projects.removeAll(m_project);
    Utils::sort(m_projects, [](Project *a, Project *b) {
        return a->displayName() < b->displayName();
    });
    endResetModel();
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
        return SessionManager::hasDependency(m_project, p) ? Qt::Checked : Qt::Unchecked;
    case Qt::DecorationRole:
        return Core::FileIconProvider::icon(p->projectFilePath().toString());
    default:
        return QVariant();
    }
}

bool DependenciesModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role == Qt::CheckStateRole) {
        Project *p = m_projects.at(index.row());
        auto c = static_cast<const Qt::CheckState>(value.toInt());

        if (c == Qt::Checked) {
            if (SessionManager::addDependency(m_project, p)) {
                emit dataChanged(index, index);
                return true;
            } else {
                QMessageBox::warning(Core::ICore::dialogParent(), QCoreApplication::translate("DependenciesModel", "Unable to Add Dependency"),
                                     QCoreApplication::translate("DependenciesModel", "This would create a circular dependency."));
            }
        } else if (c == Qt::Unchecked) {
            if (SessionManager::hasDependency(m_project, p)) {
                SessionManager::removeDependency(m_project, p);
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

QSize DependenciesView::sizeHint() const
{
    return m_sizeHint;
}

void DependenciesView::setModel(QAbstractItemModel *newModel)
{
    if (QAbstractItemModel *oldModel = model()) {
        disconnect(oldModel, &QAbstractItemModel::rowsInserted,
                   this, &DependenciesView::updateSizeHint);
        disconnect(oldModel, &QAbstractItemModel::rowsRemoved,
                   this, &DependenciesView::updateSizeHint);
        disconnect(oldModel, &QAbstractItemModel::modelReset,
                   this, &DependenciesView::updateSizeHint);
        disconnect(oldModel, &QAbstractItemModel::layoutChanged,
                   this, &DependenciesView::updateSizeHint);
    }

    QTreeView::setModel(newModel);

    if (newModel) {
        connect(newModel, &QAbstractItemModel::rowsInserted,
                this, &DependenciesView::updateSizeHint);
        connect(newModel, &QAbstractItemModel::rowsRemoved,
                this, &DependenciesView::updateSizeHint);
        connect(newModel, &QAbstractItemModel::modelReset,
                this, &DependenciesView::updateSizeHint);
        connect(newModel, &QAbstractItemModel::layoutChanged,
                this, &DependenciesView::updateSizeHint);
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
    if (heightPerRow == -1)
        heightPerRow = 30;
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

DependenciesWidget::DependenciesWidget(Project *project, QWidget *parent) : QWidget(parent),
    m_project(project),
    m_model(new DependenciesModel(project, this))
{
    auto vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(0, 0, 0, 0);
    m_detailsContainer = new Utils::DetailsWidget(this);
    m_detailsContainer->setState(Utils::DetailsWidget::NoSummary);
    vbox->addWidget(m_detailsContainer);

    auto detailsWidget = new QWidget(m_detailsContainer);
    m_detailsContainer->setWidget(detailsWidget);
    auto layout = new QGridLayout(detailsWidget);
    layout->setContentsMargins(0, -1, 0, -1);
    auto treeView = new DependenciesView(this);
    treeView->setModel(m_model);
    treeView->setHeaderHidden(true);
    layout->addWidget(treeView, 0 ,0);
    layout->addItem(new QSpacerItem(0, 0 , QSizePolicy::Expanding, QSizePolicy::Fixed), 0, 1);

    m_cascadeSetActiveCheckBox = new QCheckBox;
    m_cascadeSetActiveCheckBox->setText(tr("Synchronize configuration"));
    m_cascadeSetActiveCheckBox->setToolTip(tr("Synchronize active kit, build, and deploy configuration between projects."));
    m_cascadeSetActiveCheckBox->setChecked(SessionManager::isProjectConfigurationCascading());
    connect(m_cascadeSetActiveCheckBox, &QCheckBox::toggled,
            SessionManager::instance(), &SessionManager::setProjectConfigurationCascading);
    layout->addWidget(m_cascadeSetActiveCheckBox, 1, 0, 2, 1);
}

} // namespace Internal
} // namespace ProjectExplorer
