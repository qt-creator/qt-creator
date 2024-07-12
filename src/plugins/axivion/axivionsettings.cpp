// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "axivionsettings.h"

#include "axiviontr.h"

#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/icore.h>

#include <utils/algorithm.h>
#include <utils/id.h>
#include <utils/layoutbuilder.h>
#include <utils/stringutils.h>

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QVBoxLayout>

using namespace Core;
using namespace Utils;

namespace Axivion::Internal {

bool AxivionServer::operator==(const AxivionServer &other) const
{
    return id == other.id && dashboard == other.dashboard && username == other.username;
}

bool AxivionServer::operator!=(const AxivionServer &other) const
{
    return !(*this == other);
}

QJsonObject AxivionServer::toJson() const
{
    QJsonObject result;
    result.insert("id", id.toString());
    result.insert("dashboard", dashboard);
    result.insert("username", username);
    return result;
}

static QString fixUrl(const QString &url)
{
    const QString trimmed = Utils::trimBack(url, ' ');
    return trimmed.endsWith('/') ? trimmed : trimmed + '/';
}

AxivionServer AxivionServer::fromJson(const QJsonObject &json)
{
    const AxivionServer invalidServer;
    const QJsonValue id = json.value("id");
    if (id == QJsonValue::Undefined)
        return invalidServer;
    const QJsonValue dashboard = json.value("dashboard");
    if (dashboard == QJsonValue::Undefined)
        return invalidServer;
    const QJsonValue username = json.value("username");
    if (username == QJsonValue::Undefined)
        return invalidServer;
    return {Id::fromString(id.toString()), fixUrl(dashboard.toString()), username.toString()};
}

static FilePath tokensFilePath()
{
    return FilePath::fromString(ICore::settings()->fileName()).parentDir()
            .pathAppended("qtcreator/axivion.json");
}

static void writeTokenFile(const FilePath &filePath, const QList<AxivionServer> &servers)
{
    QJsonDocument doc;
    QJsonArray serverArray;
    for (const AxivionServer &server : servers)
        serverArray.append(server.toJson());
    doc.setArray(serverArray);
    // FIXME error handling?
    filePath.writeFileContents(doc.toJson());
    filePath.setPermissions(QFile::ReadUser | QFile::WriteUser);
}

static QList<AxivionServer> readTokenFile(const FilePath &filePath)
{
    if (!filePath.exists())
        return {};
    expected_str<QByteArray> contents = filePath.fileContents();
    if (!contents)
        return {};
    const QJsonDocument doc = QJsonDocument::fromJson(*contents);
    if (doc.isObject()) // old approach
        return { AxivionServer::fromJson(doc.object()) };
    if (!doc.isArray())
        return {};

    QList<AxivionServer> result;
    const QJsonArray serverArray = doc.array();
    for (const auto &serverValue : serverArray) {
        if (!serverValue.isObject())
            continue;
        result.append(AxivionServer::fromJson(serverValue.toObject()));
    }
    return result;
}

// AxivionSetting

AxivionSettings &settings()
{
    static AxivionSettings theSettings;
    return theSettings;
}

AxivionSettings::AxivionSettings()
{
    setSettingsGroup("Axivion");

    highlightMarks.setSettingsKey("HighlightMarks");
    highlightMarks.setLabelText(Tr::tr("Highlight marks"));
    highlightMarks.setToolTip(Tr::tr("Marks issues on the scroll bar."));
    highlightMarks.setDefaultValue(false);
    m_defaultServerId.setSettingsKey("DefaultDashboardId");
    AspectContainer::readSettings();

    m_allServers = readTokenFile(tokensFilePath());

    if (m_allServers.size() == 1 && m_defaultServerId().isEmpty()) // handle settings transition
        m_defaultServerId.setValue(m_allServers.first().id.toString());
}

void AxivionSettings::toSettings() const
{
    writeTokenFile(tokensFilePath(), m_allServers);
    AspectContainer::writeSettings();
}

Id AxivionSettings::defaultDashboardId() const
{
    return Id::fromString(m_defaultServerId());
}

const AxivionServer AxivionSettings::defaultServer() const
{
    return serverForId(defaultDashboardId());
}

const AxivionServer AxivionSettings::serverForId(const Utils::Id &id) const
{
    return Utils::findOrDefault(m_allServers, [&id](const AxivionServer &server) {
        return id == server.id;
    });
}

void AxivionSettings::disableCertificateValidation(const Utils::Id &id)
{
    const int index = Utils::indexOf(m_allServers, [&id](const AxivionServer &server) {
        return id == server.id;
    });
    if (index == -1)
        return;

    m_allServers[index].validateCert = false;
}

void AxivionSettings::updateDashboardServers(const QList<AxivionServer> &other)
{
    if (m_allServers == other)
        return;

    const Id oldDefault = defaultDashboardId();
    if (!Utils::anyOf(other, [&oldDefault](const AxivionServer &s) { return s.id == oldDefault; }))
        m_defaultServerId.setValue(other.isEmpty() ? QString{} : other.first().id.toString());

    m_allServers = other;
    emit changed(); // should we be more detailed? (id)
}

// AxivionSettingsPage

// may allow some invalid, but does some minimal check for legality
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

static bool isUrlValid(const QString &in)
{
    const QUrl url(in);
    return hostValid(url.host()) && (url.scheme() == "https" || url.scheme() == "http");
}

class DashboardSettingsWidget : public QWidget
{
public:
    explicit DashboardSettingsWidget(QWidget *parent, QPushButton *ok = nullptr);

    AxivionServer dashboardServer() const;
    void setDashboardServer(const AxivionServer &server);

    bool isValid() const;

private:
    Id m_id;
    StringAspect m_dashboardUrl;
    StringAspect m_username;
    BoolAspect m_valid;
};

DashboardSettingsWidget::DashboardSettingsWidget(QWidget *parent, QPushButton *ok)
    : QWidget(parent)
{
    m_dashboardUrl.setLabelText(Tr::tr("Dashboard URL:"));
    m_dashboardUrl.setDisplayStyle(StringAspect::LineEditDisplay);
    m_dashboardUrl.setValidationFunction([](FancyLineEdit *edit, QString *) {
        return isUrlValid(edit->text());
    });

    m_username.setLabelText(Tr::tr("Username:"));
    m_username.setDisplayStyle(StringAspect::LineEditDisplay);
    m_username.setPlaceHolderText(Tr::tr("User name"));

    using namespace Layouting;

    Form {
        m_dashboardUrl, br,
        m_username, br,
        noMargin
    }.attachTo(this);

    QTC_ASSERT(ok, return);
    auto checkValidity = [this, ok] {
        m_valid.setValue(isValid());
        ok->setEnabled(m_valid());
    };
    m_dashboardUrl.addOnChanged(this, checkValidity);
    m_username.addOnChanged(this, checkValidity);
}

AxivionServer DashboardSettingsWidget::dashboardServer() const
{
    AxivionServer result;
    if (m_id.isValid())
        result.id = m_id;
    else
        result.id = Id::generate();
    result.dashboard = fixUrl(m_dashboardUrl());
    result.username = m_username();
    return result;
}

void DashboardSettingsWidget::setDashboardServer(const AxivionServer &server)
{
    m_id = server.id;
    m_dashboardUrl.setValue(server.dashboard);
    m_username.setValue(server.username);
}

bool DashboardSettingsWidget::isValid() const
{
    return isUrlValid(m_dashboardUrl());
}

class AxivionSettingsWidget : public IOptionsPageWidget
{
public:
    AxivionSettingsWidget();

    void apply() override;

private:
    void showServerDialog(bool add);
    void removeCurrentServerConfig();
    void updateDashboardServers();
    void updateEnabledStates();

    QComboBox *m_dashboardServers = nullptr;
    QPushButton *m_edit = nullptr;
    QPushButton *m_remove = nullptr;
};

AxivionSettingsWidget::AxivionSettingsWidget()
{
    using namespace Layouting;

    m_dashboardServers = new QComboBox(this);
    m_dashboardServers->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    updateDashboardServers();

    auto addButton = new QPushButton(Tr::tr("Add..."), this);
    m_edit = new QPushButton(Tr::tr("Edit..."), this);
    m_remove = new QPushButton(Tr::tr("Remove"), this);
    Column{
        Row{
            Form{Tr::tr("Default dashboard server:"), m_dashboardServers, br},
            st,
            Column{addButton, m_edit, st, m_remove},
        },
        Space(10),
        br,
        Row{settings().highlightMarks},
        st}
        .attachTo(this);

    connect(addButton, &QPushButton::clicked, this, [this] {
        // add an empty item unconditionally
        m_dashboardServers->addItem(Tr::tr("unset"), QVariant::fromValue(AxivionServer()));
        m_dashboardServers->setCurrentIndex(m_dashboardServers->count() - 1);
        showServerDialog(true);
    });
    connect(m_edit, &QPushButton::clicked, this, [this] { showServerDialog(false); });
    connect(m_remove, &QPushButton::clicked,
            this, &AxivionSettingsWidget::removeCurrentServerConfig);
    updateEnabledStates();
}

void AxivionSettingsWidget::apply()
{
    QList<AxivionServer> servers;
    for (int i = 0, end = m_dashboardServers->count(); i < end; ++i)
        servers.append(m_dashboardServers->itemData(i).value<AxivionServer>());
    settings().updateDashboardServers(servers);
    settings().toSettings();
}

void AxivionSettingsWidget::updateDashboardServers()
{
    m_dashboardServers->clear();
    for (const AxivionServer &server : settings().allAvailableServers())
        m_dashboardServers->addItem(server.displayString(), QVariant::fromValue(server));
}

void AxivionSettingsWidget::updateEnabledStates()
{
    const bool enabled = m_dashboardServers->count();
    m_edit->setEnabled(enabled);
    m_remove->setEnabled(enabled);
}

void AxivionSettingsWidget::removeCurrentServerConfig()
{
    const QString config = m_dashboardServers->currentData().value<AxivionServer>().displayString();
    if (QMessageBox::question(
            ICore::dialogParent(),
            Tr::tr("Remove Server Configuration"),
            Tr::tr("Remove the server configuration \"%1\"?").arg(config))
        != QMessageBox::Yes) {
        return;
    }
    m_dashboardServers->removeItem(m_dashboardServers->currentIndex());
    updateEnabledStates();
}

void AxivionSettingsWidget::showServerDialog(bool add)
{
    const AxivionServer old = m_dashboardServers->currentData().value<AxivionServer>();
    QDialog d;
    d.setWindowTitle(add ? Tr::tr("Add Dashboard Configuration")
                         : Tr::tr("Edit Dashboard Configuration"));
    QVBoxLayout *layout = new QVBoxLayout;
    auto buttons = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Ok, this);
    auto ok = buttons->button(QDialogButtonBox::Ok);
    auto dashboardWidget = new DashboardSettingsWidget(this, ok);
    dashboardWidget->setDashboardServer(old);
    layout->addWidget(dashboardWidget);
    ok->setEnabled(dashboardWidget->isValid());
    connect(buttons->button(QDialogButtonBox::Cancel), &QPushButton::clicked, &d, &QDialog::reject);
    connect(ok, &QPushButton::clicked, &d, &QDialog::accept);
    layout->addWidget(buttons);
    d.setLayout(layout);
    d.resize(500, 200);

    if (d.exec() != QDialog::Accepted) {
        if (add) { // if we canceled an add, remove the canceled item
            m_dashboardServers->removeItem(m_dashboardServers->currentIndex());
            updateEnabledStates();
        }
        return;
    }
    if (dashboardWidget->isValid()) {
        const AxivionServer server = dashboardWidget->dashboardServer();
        if (server != old) {
            m_dashboardServers->setItemData(m_dashboardServers->currentIndex(),
                                            QVariant::fromValue(server));
            m_dashboardServers->setItemData(m_dashboardServers->currentIndex(),
                                            server.displayString(), Qt::DisplayRole);
        }
    }
    updateEnabledStates();
}

// AxivionSettingsPage

class AxivionSettingsPage : public IOptionsPage
{
public:
    AxivionSettingsPage()
    {
        setId("Axivion.Settings.General");
        setDisplayName(Tr::tr("General"));
        setCategory("XY.Axivion");
        setDisplayCategory(Tr::tr("Axivion"));
        setCategoryIconPath(":/axivion/images/axivion.png");
        setWidgetCreator([] { return new AxivionSettingsWidget; });
    }
};

const AxivionSettingsPage settingsPage;

} // Axivion::Internal
