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
** contact the sales department at http://www.qtsoftware.com/contact.
**
**************************************************************************/

#include "dependenciespanel.h"
#include "project.h"
#include "session.h"

#include <coreplugin/fileiconprovider.h>

#include <QtCore/QVector>
#include <QtCore/QDebug>
#include <QtCore/QAbstractListModel>
#include <QtGui/QHeaderView>
#include <QtGui/QMessageBox>
#include <QtGui/QPushButton>
#include <QtCore/QCoreApplication>

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
    qDebug()<<"Resetting";
    m_projects = m_session->projects();
    m_projects.removeAll(m_project);
    reset();
}

int DependenciesModel::rowCount(const QModelIndex &index) const
{
    return index.isValid() ? 0 : m_projects.size();
}

int DependenciesModel::columnCount(const QModelIndex &index) const
{
    return index.isValid() ? 0 : 1;
}

QVariant DependenciesModel::data(const QModelIndex &index, int role) const
{
    const Project *p = m_projects.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        return p->name();
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
    qDebug() << index << value << role << value.toBool();

    if (role == Qt::CheckStateRole) {
        const Project *p = m_projects.at(index.row());
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
    Qt::ItemFlags rc = QAbstractListModel::flags(index);
    if (index.column() == 0)
        rc |= Qt::ItemIsUserCheckable | Qt::ItemIsEditable;
    return rc;
}

//
// DependenciesWidget
//

class DependenciesWidget : public QWidget
{
public:
    DependenciesWidget(SessionManager *session, Project *project,
                       QWidget *parent = 0);

private:
    SessionManager *m_session;
    DependenciesModel *m_model;
};

DependenciesWidget::DependenciesWidget(SessionManager *session,
                                       Project *project,
                                       QWidget *parent)
    : QWidget(parent)
    , m_session(session)
    , m_model(new DependenciesModel(session, project, this))
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    QTreeView *treeView = new QTreeView(this);
    treeView->setModel(m_model);
    treeView->setHeaderHidden(true);
    treeView->setMinimumHeight(250);
    layout->addWidget(treeView);
    layout->addSpacerItem(new QSpacerItem(0, 0 , QSizePolicy::Expanding, QSizePolicy::Fixed));
}

//
// DependenciesPanel
//

DependenciesPanel::DependenciesPanel(SessionManager *session, Project *project)
    : PropertiesPanel()
    , m_widget(new DependenciesWidget(session, project))
{
}

DependenciesPanel::~DependenciesPanel()
{
    delete m_widget;
}

QString DependenciesPanel::name() const
{
    return tr("Dependencies");
}

QWidget *DependenciesPanel::widget()
{
    return m_widget;
}

//
// DependenciesPanelFactory
//

DependenciesPanelFactory::DependenciesPanelFactory(SessionManager *session)
    : m_session(session)
{
}

bool DependenciesPanelFactory::supports(Project * /* project */)
{
    return true;
}

PropertiesPanel *DependenciesPanelFactory::createPanel(Project *project)
{
    return new DependenciesPanel(m_session, project);
}

} // namespace Internal
} // namespace ProjectExplorer
