// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer.h"

#include <coreplugin/iwelcomepage.h>

#include <utils/filepath.h>

#include <QAbstractListModel>
#include <QCoreApplication>

namespace Core { class SessionModel; }

namespace ProjectExplorer {
namespace Internal {

class SessionsPage;

class ProjectModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum { FilePathRole = Qt::UserRole+1, PrettyFilePathRole, ShortcutRole };

    ProjectModel(QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

public slots:
    void resetProjects();

private:
    RecentProjectsEntries m_projects;
};

class ProjectWelcomePage : public Core::IWelcomePage
{
    Q_OBJECT
public:
    ProjectWelcomePage();

    QString title() const override { return QCoreApplication::translate("QtC::ProjectExplorer", "Projects"); }
    int priority() const override { return 20; }
    Utils::Id id() const override;
    QWidget *createWidget() const override;

    void reloadWelcomeScreenData() const;

public slots:
    void newProject();
    void openProject();

signals:
    void requestProject(const Utils::FilePath &project);

private:
    void openSessionAt(int index);
    void openProjectAt(int index);
    void createActions();

    friend class SessionsPage;
    Core::SessionModel *m_sessionModel = nullptr;
    ProjectModel *m_projectModel = nullptr;
};

} // namespace Internal
} // namespace ProjectExplorer
