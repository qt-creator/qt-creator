// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gitlaboptionspage.h"

#include "gitlabparameters.h"

#include <coreplugin/icore.h>

#include <utils/algorithm.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcassert.h>

#include <vcsbase/vcsbaseconstants.h>

#include <QAction>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QRegularExpression>
#include <QUuid>

using namespace Utils;

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
    return (host == "localhost") || dn.match(host).hasMatch();
}

GitLabServerWidget::GitLabServerWidget(Mode m, QWidget *parent)
    : QWidget(parent)
    , m_mode(m)
{
    m_host.setLabelText(GitLabOptionsPage::tr("Host:"));
    m_host.setDisplayStyle(m == Display ? Utils::StringAspect::LabelDisplay
                                        : Utils::StringAspect::LineEditDisplay);
    m_host.setValidationFunction([](Utils::FancyLineEdit *l, QString *) {
        return hostValid(l->text());
    });

    m_description.setLabelText(GitLabOptionsPage::tr("Description:"));
    m_description.setDisplayStyle(m == Display ? Utils::StringAspect::LabelDisplay
                                               : Utils::StringAspect::LineEditDisplay);

    m_token.setLabelText(GitLabOptionsPage::tr("Access token:"));
    m_token.setDisplayStyle(m == Display ? Utils::StringAspect::LabelDisplay
                                         : Utils::StringAspect::LineEditDisplay);
    m_token.setVisible(m == Edit);

    m_port.setLabelText(GitLabOptionsPage::tr("Port:"));
    m_port.setRange(1, 65535);
    m_port.setValue(GitLabServer::defaultPort);
    m_port.setEnabled(m == Edit);
    m_secure.setLabelText(GitLabOptionsPage::tr("HTTPS:"));
    m_secure.setLabelPlacement(Utils::BoolAspect::LabelPlacement::InExtraLabel);
    m_secure.setDefaultValue(true);
    m_secure.setEnabled(m == Edit);

    using namespace Layouting;

    Row {
        Form {
            m_host,
            m_description,
            m_token,
            m_port,
            m_secure
        },
    }.attachTo(this, m == Edit ? WithMargins : WithoutMargins);
}

GitLabServer GitLabServerWidget::gitLabServer() const
{
    GitLabServer result;
    result.id = m_mode == Edit ? Utils::Id::fromName(QUuid::createUuid().toByteArray()) : m_id;
    result.host = m_host.value();
    result.description = m_description.value();
    result.token = m_token.value();
    result.port = m_port.value();
    result.secure = m_secure.value();
    return result;
}

void GitLabServerWidget::setGitLabServer(const GitLabServer &server)
{
    m_id = server.id;
    m_host.setValue(server.host);
    m_description.setValue(server.description);
    m_token.setValue(server.token);
    m_port.setValue(server.port);
    m_secure.setValue(server.secure);
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

    Grid {
        Form {
            defaultLabel, m_defaultGitLabServer, br,
            Row { Group { Column { m_gitLabServerWidget, Space(1) } } }, br,
            m_curl, br,
        }, Column { m_add, m_edit, m_remove, st },
    }.attachTo(this);

    connect(m_edit, &QPushButton::clicked, this, &GitLabOptionsWidget::showEditServerDialog);
    connect(m_remove, &QPushButton::clicked, this, &GitLabOptionsWidget::removeCurrentTriggered);
    connect(m_add, &QPushButton::clicked, this, &GitLabOptionsWidget::showAddServerDialog);
    connect(m_defaultGitLabServer, &QComboBox::currentIndexChanged, this, [this] {
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
    setId(Constants::GITLAB_SETTINGS);
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
