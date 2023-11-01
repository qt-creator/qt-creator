// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gitlaboptionspage.h"

#include "gitlabparameters.h"
#include "gitlabtr.h"

#include <coreplugin/icore.h>

#include <utils/algorithm.h>
#include <utils/aspects.h>
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

class GitLabServerWidget : public QWidget
{
public:
    enum Mode { Display, Edit };
    explicit GitLabServerWidget(Mode m, QWidget *parent = nullptr);

    GitLabServer gitLabServer() const;
    void setGitLabServer(const GitLabServer &server);

private:
    Mode m_mode = Display;
    Id m_id;
    StringAspect m_host;
    StringAspect m_description;
    StringAspect m_token;
    IntegerAspect m_port;
    BoolAspect m_secure;
};

GitLabServerWidget::GitLabServerWidget(Mode m, QWidget *parent)
    : QWidget(parent)
    , m_mode(m)
{
    m_host.setLabelText(Tr::tr("Host:"));
    m_host.setDisplayStyle(m == Display ? StringAspect::LabelDisplay
                                        : StringAspect::LineEditDisplay);
    m_host.setValidationFunction([](FancyLineEdit *l, QString *) {
        return hostValid(l->text());
    });

    m_description.setLabelText(Tr::tr("Description:"));
    m_description.setDisplayStyle(m == Display ? StringAspect::LabelDisplay
                                               : StringAspect::LineEditDisplay);

    m_token.setLabelText(Tr::tr("Access token:"));
    m_token.setDisplayStyle(m == Display ? StringAspect::LabelDisplay
                                         : StringAspect::LineEditDisplay);
    m_token.setVisible(m == Edit);

    m_port.setLabelText(Tr::tr("Port:"));
    m_port.setRange(1, 65535);
    m_port.setValue(GitLabServer::defaultPort);
    m_port.setEnabled(m == Edit);
    m_secure.setLabelText(Tr::tr("HTTPS:"));
    m_secure.setLabelPlacement(BoolAspect::LabelPlacement::InExtraLabel);
    m_secure.setDefaultValue(true);
    m_secure.setEnabled(m == Edit);

    using namespace Layouting;

    Row {
        Form {
            m_host, br,
            m_description, br,
            m_token, br,
            m_port, br,
            m_secure,
            m == Edit ? normalMargin : noMargin
        },
    }.attachTo(this);
}

GitLabServer GitLabServerWidget::gitLabServer() const
{
    GitLabServer result;
    result.id = m_mode == Edit ? Id::fromName(QUuid::createUuid().toByteArray()) : m_id;
    result.host = m_host();
    result.description = m_description();
    result.token = m_token();
    result.port = m_port();
    result.secure = m_secure();
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

class GitLabOptionsWidget : public Core::IOptionsPageWidget
{
public:
    explicit GitLabOptionsWidget(GitLabParameters *parameters);

private:
    void showEditServerDialog();
    void showAddServerDialog();
    void removeCurrentTriggered();
    void addServer(const GitLabServer &newServer);
    void modifyCurrentServer(const GitLabServer &newServer);
    void updateButtonsState();

    GitLabParameters *m_parameters = nullptr;
    GitLabServerWidget *m_gitLabServerWidget = nullptr;
    QPushButton *m_edit = nullptr;
    QPushButton *m_remove = nullptr;
    QPushButton *m_add = nullptr;
    QComboBox *m_defaultGitLabServer = nullptr;
    FilePathAspect m_curl;
};

GitLabOptionsWidget::GitLabOptionsWidget(GitLabParameters *params)
    : m_parameters(params)
{
    auto defaultLabel = new QLabel(Tr::tr("Default:"), this);
    m_defaultGitLabServer = new QComboBox(this);
    m_curl.setLabelText(Tr::tr("curl:"));
    m_curl.setExpectedKind(PathChooser::ExistingCommand);

    m_gitLabServerWidget = new GitLabServerWidget(GitLabServerWidget::Display, this);

    m_edit = new QPushButton(Tr::tr("Edit..."), this);
    m_edit->setToolTip(Tr::tr("Edit current selected GitLab server configuration."));
    m_remove = new QPushButton(Tr::tr("Remove"), this);
    m_remove->setToolTip(Tr::tr("Remove current selected GitLab server configuration."));
    m_add = new QPushButton(Tr::tr("Add..."), this);
    m_add->setToolTip(Tr::tr("Add new GitLab server configuration."));

    using namespace Layouting;

    Grid {
        Form {
            defaultLabel, m_defaultGitLabServer, br,
            Row { Group { Column { m_gitLabServerWidget, Space(1) } } }, br,
            m_curl, br,
        }, Column { m_add, m_edit, m_remove, st },
    }.attachTo(this);

    m_curl.setValue(params->curl);

    for (const auto &gitLabServer : params->gitLabServers) {
        m_defaultGitLabServer->addItem(gitLabServer.displayString(),
                                       QVariant::fromValue(gitLabServer));
    }

    const GitLabServer found = params->currentDefaultServer();
    if (found.id.isValid()) {
        m_defaultGitLabServer->setCurrentIndex(m_defaultGitLabServer->findData(
                                                   QVariant::fromValue(found)));
    }
    updateButtonsState();

    connect(m_edit, &QPushButton::clicked, this, &GitLabOptionsWidget::showEditServerDialog);
    connect(m_remove, &QPushButton::clicked, this, &GitLabOptionsWidget::removeCurrentTriggered);
    connect(m_add, &QPushButton::clicked, this, &GitLabOptionsWidget::showAddServerDialog);
    connect(m_defaultGitLabServer, &QComboBox::currentIndexChanged, this, [this] {
        m_gitLabServerWidget->setGitLabServer(
                    m_defaultGitLabServer->currentData().value<GitLabServer>());
    });

    setOnApply([this] {
        GitLabParameters result;
        // get all configured gitlabservers
        for (int i = 0, end = m_defaultGitLabServer->count(); i < end; ++i)
            result.gitLabServers.append(m_defaultGitLabServer->itemData(i).value<GitLabServer>());
        if (m_defaultGitLabServer->count())
            result.defaultGitLabServer = m_defaultGitLabServer->currentData().value<GitLabServer>().id;
        result.curl = m_curl();

        if (result != *m_parameters) {
            m_parameters->assign(result);
            m_parameters->toSettings(Core::ICore::settings());
            emit m_parameters->changed();
        }
    });
}

void GitLabOptionsWidget::showEditServerDialog()
{
    const GitLabServer old = m_defaultGitLabServer->currentData().value<GitLabServer>();
    QDialog d;
    d.setWindowTitle(Tr::tr("Edit Server..."));
    QVBoxLayout *layout = new QVBoxLayout;
    GitLabServerWidget *serverWidget = new GitLabServerWidget(GitLabServerWidget::Edit, this);
    serverWidget->setGitLabServer(old);
    layout->addWidget(serverWidget);
    auto buttons = new QDialogButtonBox(QDialogButtonBox::Cancel, this);
    auto modifyButton = buttons->addButton(Tr::tr("Modify"), QDialogButtonBox::AcceptRole);
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
    d.setWindowTitle(Tr::tr("Add Server..."));
    QVBoxLayout *layout = new QVBoxLayout;
    GitLabServerWidget *serverWidget = new GitLabServerWidget(GitLabServerWidget::Edit, this);
    layout->addWidget(serverWidget);
    auto buttons = new QDialogButtonBox(QDialogButtonBox::Cancel, this);
    auto addButton = buttons->addButton(Tr::tr("Add"), QDialogButtonBox::AcceptRole);
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

// GitLabOptionsPage

GitLabOptionsPage::GitLabOptionsPage(GitLabParameters *p)
{
    setId(Constants::GITLAB_SETTINGS);
    setDisplayName(Tr::tr("GitLab"));
    setCategory(VcsBase::Constants::VCS_SETTINGS_CATEGORY);
    setWidgetCreator([p] { return new GitLabOptionsWidget(p); });
}

} // namespace GitLab
