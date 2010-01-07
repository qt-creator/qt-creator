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

#ifndef DEPENDENCIESDIALOG_H
#define DEPENDENCIESDIALOG_H

#include "iprojectproperties.h"
#include <utils/detailswidget.h>

#include <QtCore/QSize>
#include <QtGui/QWidget>
#include <QtGui/QLabel>
#include <QtGui/QTreeView>

namespace ProjectExplorer {

class Project;
class SessionManager;

namespace Internal {

class DependenciesWidget;

class DependenciesPanelFactory : public IPanelFactory
{
public:
    DependenciesPanelFactory(SessionManager *session);

    bool supports(Project *project);
    IPropertiesPanel *createPanel(Project *project);

private:
    SessionManager *m_session;
};


class DependenciesPanel : public IPropertiesPanel
{
public:
    DependenciesPanel(SessionManager *session, Project *project);
    ~DependenciesPanel();
    QString displayName() const;
    QWidget *widget() const;
    QIcon icon() const;

private:
    DependenciesWidget *m_widget;
    const QIcon m_icon;
};

//
// DependenciesModel
//

class DependenciesModel : public QAbstractListModel
{
    Q_OBJECT
public:
    DependenciesModel(SessionManager *session, Project *project, QObject *parent = 0);
    ~DependenciesModel();

    int rowCount(const QModelIndex &index) const;
    int columnCount(const QModelIndex &index) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
    Qt::ItemFlags flags(const QModelIndex &index) const;

public slots:
    void resetModel();

private:
    SessionManager *m_session;
    Project *m_project;
    QList<Project *> m_projects;
};

class DependenciesView : public QTreeView
{
    Q_OBJECT
public:
    DependenciesView(QWidget *parent);
    ~DependenciesView();
    virtual QSize sizeHint() const;
    virtual void setModel(QAbstractItemModel *model);
private slots:
    void updateSizeHint();
private:
    QSize m_sizeHint;
};

class DependenciesWidget : public QWidget
{
    Q_OBJECT
public:
    DependenciesWidget(SessionManager *session, Project *project,
                       QWidget *parent = 0);
private slots:
    void updateDetails();

private:
    SessionManager *m_session;
    Project *m_project;
    DependenciesModel *m_model;
    Utils::DetailsWidget *m_detailsContainer;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // DEPENDENCIESDIALOG_H
