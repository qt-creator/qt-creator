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

#ifndef PROJECTWELCOMEPAGE_H
#define PROJECTWELCOMEPAGE_H

#include <QAbstractListModel>

#include <coreplugin/iwelcomepage.h>

QT_BEGIN_NAMESPACE
class QQmlEngine;
QT_END_NAMESPACE

namespace ProjectExplorer {
namespace Internal {

class SessionModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum { DefaultSessionRole = Qt::UserRole+1, LastSessionRole, ActiveSessionRole, ProjectsPathRole, ProjectsDisplayRole };

    explicit SessionModel(QObject *parent = 0);
    int rowCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QHash<int, QByteArray> roleNames() const;

    Q_SCRIPTABLE bool isDefaultVirgin() const;

public slots:
    void resetSessions();
    void cloneSession(const QString &session);
    void deleteSession(const QString &session);
    void renameSession(const QString &session);
};


class ProjectModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum { FilePathRole = Qt::UserRole+1, PrettyFilePathRole };

    ProjectModel(QObject *parent = 0);
    int rowCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QHash<int, QByteArray> roleNames() const;

public slots:
    void resetProjects();
};

class ProjectWelcomePage : public Core::IWelcomePage
{
    Q_OBJECT
public:
    ProjectWelcomePage();

    void facilitateQml(QQmlEngine *engine);
    QUrl pageLocation() const;
    QWidget *page() { return 0; }
    QString title() const { return tr("Projects"); }
    int priority() const { return 20; }
    Core::Id id() const;

    void reloadWelcomeScreenData();

public slots:
    void newProject();
    void openProject();

signals:
    void requestProject(const QString &project);
    void requestSession(const QString &session);
    void manageSessions();

private:
    SessionModel *m_sessionModel;
    ProjectModel *m_projectModel;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // PROJECTWELCOMEPAGE_H
