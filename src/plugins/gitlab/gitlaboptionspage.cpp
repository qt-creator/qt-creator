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

#include "gitlaboptionspage.h"

#include "gitlabparameters.h"

#include <coreplugin/icore.h>
#include <utils/algorithm.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcassert.h>
#include <vcsbase/vcsbaseconstants.h>

#include <QAction>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QRegularExpression>
#include <QUuid>

namespace GitLab {

static bool hostValid(const QString &host)
{
    static const QRegularExpression ip(R"(^(\d+).(\d+).(\d+).(\d+)$)");
    static const QRegularExpression dn(R"(^([a-zA-Z0-9][a-zA-Z0-9-]+\.)+[a-zA-Z0-9][a-zA-Z0-9-]+$)");
    const QRegularExpressionMatch match = ip.match(host);
    if (match.hasMatch()) {
        for (int i = 1; i < 5; ++i) {
            int val = match.captured(i).toInt();
            if (val < 0 || val > 255)
                return false;
        }
        return true;
    }
    return dn.match(host).hasMatch();
}

GitLabServerWidget::GitLabServerWidget(Mode m, QWidget *parent)
    : QWidget(parent)
    , m_mode(m)
{
    using namespace Utils::Layouting;

    auto hostLabel = new QLabel(tr("Host:"), this);
    m_host.setDisplayStyle(m == Display ? Utils::StringAspect::LabelDisplay
                                        : Utils::StringAspect::LineEditDisplay);
    m_host.setValidationFunction([](Utils::FancyLineEdit *l, QString *) {
        return hostValid(l->text());
    });
    auto descriptionLabel = new QLabel(tr("Description:"), this);
    m_description.setDisplayStyle(m == Display ? Utils::StringAspect::LabelDisplay
                                               : Utils::StringAspect::LineEditDisplay);
    auto tokenLabel = new QLabel(tr("Access token:"), this);
    m_token.setDisplayStyle(m == Display ? Utils::StringAspect::LabelDisplay
                                         : Utils::StringAspect::LineEditDisplay);
    m_token.setVisible(m == Edit);
    tokenLabel->setVisible(m == Edit);
    m_port.setRange(1, 65535);
    auto portLabel = new QLabel(tr("Port:"), this);
    m_port.setDefaultValue(GitLabServer::defaultPort);
    m_port.setEnabled(m == Edit);

    using namespace Utils::Layouting;
    const Break nl;

    Form {
        hostLabel, m_host, nl,
        descriptionLabel, m_description, nl,
        tokenLabel, m_token, nl,
        portLabel, Span(1, Row { m_port, Stretch() }), nl,
    }.attachTo(this, m == Edit);
}

GitLabServer GitLabServerWidget::gitLabServer() const
{
    GitLabServer result;
    result.id = m_mode == Edit ? Utils::Id::fromName(QUuid::createUuid().toByteArray()) : m_id;
    result.host = m_host.value();
    result.description = m_description.value();
    result.token = m_token.value();
    result.port = m_port.value();
    return result;
}

void GitLabServerWidget::setGitLabServer(const GitLabServer &server)
{
    m_id = server.id;
    m_host.setValue(server.host);
    m_description.setValue(server.description);
    m_token.setValue(server.token);
    m_port.setValue(server.port);
}

GitLabOptionsWidget::GitLabOptionsWidget(QWidget *parent)
    : QWidget(parent)
{
    auto defaultLabel = new QLabel(tr("Default:"), this);
    m_defaultGitLabServer = new QComboBox(this);
    m_curl.setDisplayStyle(Utils::StringAspect::DisplayStyle::PathChooserDisplay);
    m_curl.setLabelText(tr("curl:"));
    m_curl.setExpectedKind(Utils::PathChooser::ExistingCommand);

    m_gitLabServerWidget = new GitLabServerWidget(GitLabServerWidget::Display, this);

    m_edit = new QPushButton(tr("Edit..."), this);
    m_edit->setToolTip(tr("Edit current selected GitLab server configuration."));
    m_remove = new QPushButton(tr("Remove"), this);
    m_remove->setToolTip(tr("Remove current selected GitLab server configuration."));
    m_add = new QPushButton(tr("Add..."), this);
    m_add->setToolTip(tr("Add new GitLab server configuration."));

    using namespace Utils::Layouting;
    const Break nl;

    Grid {
        Form {
            defaultLabel, m_defaultGitLabServer, nl,
            Row { Group { m_gitLabServerWidget, Space(1) } }, nl,
            m_curl, nl,
        }, Column { m_add, m_edit, m_remove, Stretch() },
    }.attachTo(this);

    connect(m_edit, &QPushButton::clicked, this, &GitLabOptionsWidget::showEditServerDialog);
    connect(m_remove, &QPushButton::clicked, this, &GitLabOptionsWidget::removeCurrentTriggered);
    connect(m_add, &QPushButton::clicked, this, &GitLabOptionsWidget::showAddServerDialog);
    connect(m_defaultGitLabServer, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this]() {
        m_gitLabServerWidget->setGitLabServer(
                    m_defaultGitLabServer->currentData().value<GitLabServer>());
    });
}

GitLabParameters GitLabOptionsWidget::parameters() const
{
    GitLabParameters result;
    // get all configured gitlabservers
    for (int i = 0, end = m_defaultGitLabServer->count(); i < end; ++i)
        result.gitLabServers.append(m_defaultGitLabServer->itemData(i).value<GitLabServer>());
    if (m_defaultGitLabServer->count())
        result.defaultGitLabServer = m_defaultGitLabServer->currentData().value<GitLabServer>().id;
    result.curl = m_curl.filePath();
    return result;
}

void GitLabOptionsWidget::setParameters(const GitLabParameters &params)
{
    m_curl.setFilePath(params.curl);

    for (const auto &gitLabServer : params.gitLabServers) {
        m_defaultGitLabServer->addItem(gitLabServer.displayString(),
                                       QVariant::fromValue(gitLabServer));
    }

    const GitLabServer found = params.currentDefaultServer();
    if (found.id.isValid()) {
        m_defaultGitLabServer->setCurrentIndex(m_defaultGitLabServer->findData(
                                                   QVariant::fromValue(found)));
    }
    updateButtonsState();
}

void GitLabOptionsWidget::showEditServerDialog()
{
    const GitLabServer old = m_defaultGitLabServer->currentData().value<GitLabServer>();
    QDialog d;
    d.setWindowTitle(tr("Edit Server..."));
    QVBoxLayout *layout = new QVBoxLayout;
    GitLabServerWidget *serverWidget = new GitLabServerWidget(GitLabServerWidget::Edit, this);
    serverWidget->setGitLabServer(old);
    layout->addWidget(serverWidget);
    auto buttons = new QDialogButtonBox(QDialogButtonBox::Cancel, this);
    auto modifyButton = buttons->addButton(tr("Modify"), QDialogButtonBox::AcceptRole);
    connect(modifyButton, &QPushButton::clicked, &d, &QDialog::accept);
    connect(buttons->button(QDialogButtonBox::Cancel), &QPushButton::clicked, &d, &QDialog::reject);
    layout->addWidget(buttons);
    d.setLayout(layout);
    d.resize(300, 200);
    if (d.exec() != QDialog::Accepted)
        return;

    const GitLabServer server = serverWidget->gitLabServer();
    if (server != old && hostValid(server.host))
        modifyCurrentServer(server);
}

void GitLabOptionsWidget::showAddServerDialog()
{
    QDialog d;
    d.setWindowTitle(tr("Add Server..."));
    QVBoxLayout *layout = new QVBoxLayout;
    GitLabServerWidget *serverWidget = new GitLabServerWidget(GitLabServerWidget::Edit, this);
    layout->addWidget(serverWidget);
    auto buttons = new QDialogButtonBox(QDialogButtonBox::Cancel, this);
    auto addButton = buttons->addButton(tr("Add"), QDialogButtonBox::AcceptRole);
    connect(addButton, &QPushButton::clicked, &d, &QDialog::accept);
    connect(buttons->button(QDialogButtonBox::Cancel), &QPushButton::clicked, &d, &QDialog::reject);
    layout->addWidget(buttons);
    d.setLayout(layout);
    d.resize(300, 200);
    if (d.exec() != QDialog::Accepted)
        return;

    const GitLabServer server = serverWidget->gitLabServer();
    if (hostValid(server.host))
        addServer(server);
}

void GitLabOptionsWidget::removeCurrentTriggered()
{
    int current = m_defaultGitLabServer->currentIndex();
    if (current > -1)
        m_defaultGitLabServer->removeItem(current);
    updateButtonsState();
}

void GitLabOptionsWidget::addServer(const GitLabServer &newServer)
{
    QTC_ASSERT(newServer.id.isValid(), return);
    const QVariant variant = QVariant::fromValue(newServer);
    m_defaultGitLabServer->addItem(newServer.displayString(), variant);
    int index = m_defaultGitLabServer->findData(variant);
    m_defaultGitLabServer->setCurrentIndex(index);
    m_gitLabServerWidget->setGitLabServer(newServer);
    updateButtonsState();
}

void GitLabOptionsWidget::modifyCurrentServer(const GitLabServer &newServer)
{
    int current = m_defaultGitLabServer->currentIndex();
    if (current > -1)
        m_defaultGitLabServer->setItemData(current, newServer.displayString(), Qt::DisplayRole);
    m_defaultGitLabServer->setItemData(current, QVariant::fromValue(newServer));
    m_gitLabServerWidget->setGitLabServer(newServer);
}

void GitLabOptionsWidget::updateButtonsState()
{
    const bool hasItems = m_defaultGitLabServer->count() > 0;
    m_edit->setEnabled(hasItems);
    m_remove->setEnabled(hasItems);
}

GitLabOptionsPage::GitLabOptionsPage(GitLabParameters *p, QObject *parent)
    : Core::IOptionsPage{parent}
    , m_parameters(p)
{
    setId("GitLab");
    setDisplayName(tr("GitLab"));
    setCategory(VcsBase::Constants::VCS_SETTINGS_CATEGORY);
}

QWidget *GitLabOptionsPage::widget()
{
    if (!m_widget) {
        m_widget = new GitLabOptionsWidget;
        m_widget->setParameters(*m_parameters);
    }
    return m_widget;
}

void GitLabOptionsPage::apply()
{
    if (GitLabOptionsWidget *w = m_widget.data()) {
        GitLabParameters newParameters = w->parameters();
        if (newParameters != *m_parameters) {
            *m_parameters = newParameters;
            m_parameters->toSettings(Core::ICore::settings());
            emit settingsChanged();
        }
    }
}

void GitLabOptionsPage::finish()
{
    delete m_widget;
}

} // namespace GitLab
