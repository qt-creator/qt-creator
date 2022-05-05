/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
