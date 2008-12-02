/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/
#include "dependenciesdialog.h"
#include "project.h"
#include "session.h"

#include <QtCore/QVector>
#include <QtCore/QDebug>
#include <QtCore/QAbstractTableModel>
#include <QtGui/QPushButton>
#include <QtGui/QHeaderView>

namespace ProjectExplorer {
namespace Internal {

// ------ DependencyModel

class DependencyModel : public QAbstractTableModel {
public:
    typedef  ProjectExplorer::Project Project;
    typedef  DependenciesDialog::ProjectList ProjectList;

    DependencyModel(SessionManager *sln, const ProjectList &projectList, QObject * parent = 0);

    virtual int rowCount(const QModelIndex&) const { return m_projects.size(); }
    virtual int columnCount(const QModelIndex&) const { return m_projects.size(); }

    virtual QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const;
    bool setData ( const QModelIndex & index, const QVariant & value, int role = Qt::EditRole);

    virtual Qt::ItemFlags flags ( const QModelIndex & index ) const;

    QVariant headerData ( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;

    // Apply changed items
    unsigned apply(SessionManager *sln) const;

    void resetDependencies();

private:

    struct Entry {
        Entry(SessionManager *sln, Project *rootProject, Project *dependentProject);
        Entry() : m_dependentProject(0), m_dependent(false), m_defaultValue(false), m_canAddDependency(false) {}
        Project* m_dependentProject;
        bool m_dependent;
        bool m_defaultValue;
        bool m_canAddDependency;
    };

    // column
    typedef QVector<Entry> ProjectDependencies;
    typedef  QList<ProjectDependencies> Projects;
    Projects m_projects;
    ProjectList m_projectList;
};

DependencyModel::Entry::Entry(SessionManager *sln,
                              Project *rootProject,
                              Project *dependentProject) :
    m_dependentProject(dependentProject),
    m_dependent(sln->hasDependency(rootProject, dependentProject)),
    m_defaultValue(m_dependent),
    m_canAddDependency(sln->canAddDependency(rootProject, dependentProject))
{
}

DependencyModel::DependencyModel(SessionManager *sln,
                                 const ProjectList &projectList,
                                 QObject * parent) :
    QAbstractTableModel(parent),
    m_projectList(projectList)
{
    const int count = projectList.size();
    for (int p = 0; p < count; p++) {
        Project *rootProject = projectList.at(p);
        ProjectDependencies dependencies;
        dependencies.reserve(count);
        for (int d = 0; d < count ; d++)
            dependencies.push_back(p == d ? Entry() : Entry(sln, rootProject, projectList.at(d)));

        m_projects += dependencies;
    }
}

QVariant DependencyModel::data ( const QModelIndex & index, int role  ) const
{
    static const QVariant empty = QVariant(QString());
    // TO DO: find a checked icon
    static const QVariant checked = QVariant(QString(QLatin1Char('x')));

    const int p = index.column();
    const int d = index.row();
    switch (role) {
    case Qt::EditRole:
        return QVariant(m_projects[p][d].m_dependent);
    case Qt::DisplayRole:
        return m_projects[p][d].m_dependent ?  checked : empty;
    default:
        break;
    }
    return QVariant();
}

bool DependencyModel::setData ( const QModelIndex & index, const QVariant & value, int role)
{
  switch (role) {
    case Qt::EditRole: {
        const int p = index.column();
        const int d = index.row();
        if (d == p)
            return false;
        Entry &e(m_projects[p][d]);
        e.m_dependent = value.toBool();
        emit dataChanged(index, index);
    }
        return true;
    default:
        break;
    }
    return false;
}

Qt::ItemFlags DependencyModel::flags ( const QModelIndex & index ) const
{
    const int p = index.column();
    const int d = index.row();

    if (d == p)
        return 0;

    const Entry &e(m_projects[p][d]);
    Qt::ItemFlags rc = Qt::ItemIsEnabled|Qt::ItemIsUserCheckable | Qt::ItemIsSelectable;
    if (e.m_canAddDependency)
        rc |= Qt::ItemIsEditable;
    return rc;
}

QVariant DependencyModel::headerData ( int section, Qt::Orientation , int role ) const
{
    switch (role) {
    case Qt::DisplayRole:
        return QVariant(m_projectList.at(section)->name());
    default:
        break;
    }
    return QVariant();
}

void DependencyModel::resetDependencies()
{
    if (const int count = m_projectList.size()) {
        for (int p = 0; p < count; p++)
            for (int d = 0; d < count; d++)
                m_projects[p][d].m_dependent = false;
        reset();
    }
}

unsigned DependencyModel::apply(SessionManager *sln) const
{
    unsigned rc = 0;
    const int count = m_projectList.size();
    for (int p = 0; p < count; p++) {
        Project *rootProject = m_projectList.at(p);
        for (int d = 0; d < count; d++) {
            if (d != p) {
                const  Entry &e(m_projects[p][d]);
                if (e.m_dependent != e. m_defaultValue) {
                    rc++;
                    if (e.m_dependent) {
                        sln->addDependency(rootProject, e.m_dependentProject);
                    } else {
                        sln->removeDependency(rootProject, e.m_dependentProject);
                    }
                }
            }
        }
    }
    return rc;
}

// ------ DependenciesDialog
DependenciesDialog::DependenciesDialog(QWidget *parent, SessionManager *sln) :
    QDialog(parent),
    m_sln(sln),
    m_projectList(m_sln->projects()),
    m_model(new DependencyModel(sln, m_projectList))
{
    m_ui.setupUi(this);
    m_ui.buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
    QPushButton *resetButton = m_ui.buttonBox->addButton (QDialogButtonBox::Reset);
    connect(resetButton, SIGNAL(clicked()), this, SLOT(reset()));

    m_ui.dependencyTable->setModel(m_model);
}

void DependenciesDialog::accept()
{
    m_model->apply(m_sln);
    QDialog::accept();
}

void DependenciesDialog::reset()
{
    m_model->resetDependencies();
}

DependenciesDialog::~DependenciesDialog()
{
}

}
}
