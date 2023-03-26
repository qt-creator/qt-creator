// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "gitlabparameters.h"

#include <coreplugin/dialogs/ioptionspage.h>
#include <utils/aspects.h>

#include <QPointer>

QT_BEGIN_NAMESPACE
class QComboBox;
class QPushButton;
QT_END_NAMESPACE

namespace GitLab {

namespace Constants {
const char GITLAB_SETTINGS[] = "GitLab";
} // namespace Constants

class GitLabServerWidget : public QWidget
{
public:
    enum Mode { Display, Edit };
    explicit GitLabServerWidget(Mode m, QWidget *parent = nullptr);

    GitLabServer gitLabServer() const;
    void setGitLabServer(const GitLabServer &server);

    bool isValid() const;
private:
    Mode m_mode = Display;
    Utils::Id m_id;
    Utils::StringAspect m_host;
    Utils::StringAspect m_description;
    Utils::StringAspect m_token;
    Utils::IntegerAspect m_port;
    Utils::BoolAspect m_secure;
};

class GitLabOptionsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit GitLabOptionsWidget(QWidget *parent = nullptr);

    GitLabParameters parameters() const;
    void setParameters(const GitLabParameters &params);

private:
    void showEditServerDialog();
    void showAddServerDialog();
    void removeCurrentTriggered();
    void addServer(const GitLabServer &newServer);
    void modifyCurrentServer(const GitLabServer &newServer);
    void updateButtonsState();

    GitLabServerWidget *m_gitLabServerWidget = nullptr;
    QPushButton *m_edit = nullptr;
    QPushButton *m_remove = nullptr;
    QPushButton *m_add = nullptr;
    QComboBox *m_defaultGitLabServer = nullptr;
    Utils::StringAspect m_curl;
};

class GitLabOptionsPage : public Core::IOptionsPage
{
    Q_OBJECT
public:
    explicit GitLabOptionsPage(GitLabParameters *p, QObject *parent = nullptr);

    QWidget *widget() final;
    void apply() final;
    void finish() final;

signals:
    void settingsChanged();

private:
    void addServer();

    GitLabParameters *m_parameters;
    QPointer<GitLabOptionsWidget> m_widget;
};

} // namespace GitLab
