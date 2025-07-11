// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/iwelcomepage.h>

#include <QCoreApplication>

namespace Utils { class FilePath; }
namespace Core { class SessionModel; }

namespace ProjectExplorer {
namespace Internal {

class ProjectModel;
class SessionsPage;

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

    static QWidget *createRecentProjectsView();

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
