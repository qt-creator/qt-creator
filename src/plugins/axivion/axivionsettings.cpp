// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "axivionsettings.h"

#include "axiviontr.h"

#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/icore.h>

#include <utils/hostosinfo.h>
#include <utils/id.h>
#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>

#include <QDialog>
#include <QDialogButtonBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QPushButton>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QUuid>
#include <QVBoxLayout>

using namespace Utils;

namespace Axivion::Internal {

AxivionServer::AxivionServer(const Utils::Id &id, const QString &dashboard,
                             const QString &description, const QString &token)
    : id(id)
    , dashboard(dashboard)
    , description(description)
    , token(token)
{
}

bool AxivionServer::operator==(const AxivionServer &other) const
{
    return id == other.id && dashboard == other.dashboard
            && description == other.description && token == other.token;
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
    result.insert("description", description);
    result.insert("token", token);
    return result;
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
    const QJsonValue description = json.value("description");
    if (description == QJsonValue::Undefined)
        return invalidServer;
    const QJsonValue token = json.value("token");
    if (token == QJsonValue::Undefined)
        return invalidServer;
    return { Utils::Id::fromString(id.toString()), dashboard.toString(),
                description.toString(), token.toString() };
}

QStringList AxivionServer::curlArguments() const
{
    QStringList args { "-sS" }; // silent, but show error
    if (dashboard.startsWith("https://") && !validateCert)
        args << "-k";
    return args;
}

static Utils::FilePath tokensFilePath()
{
    return Utils::FilePath::fromString(Core::ICore::settings()->fileName()).parentDir()
            .pathAppended("qtcreator/axivion.json");
}

static void writeTokenFile(const Utils::FilePath &filePath, const AxivionServer &server)
{
    QJsonDocument doc;
    doc.setObject(server.toJson());
    // FIXME error handling?
    filePath.writeFileContents(doc.toJson());
    filePath.setPermissions(QFile::ReadUser | QFile::WriteUser);
}

static AxivionServer readTokenFile(const Utils::FilePath &filePath)
{
    if (!filePath.exists())
        return {};
    Utils::expected_str<QByteArray> contents = filePath.fileContents();
    if (!contents)
        return {};
    const QJsonDocument doc = QJsonDocument::fromJson(*contents);
    if (!doc.isObject())
        return {};
    return AxivionServer::fromJson(doc.object());
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

    curl.setSettingsKey("Curl");
    curl.setLabelText(Tr::tr("curl:"));
    curl.setExpectedKind(Utils::PathChooser::ExistingCommand);

    AspectContainer::readSettings();

    server = readTokenFile(tokensFilePath());

    if (curl().isEmpty() || !curl().exists()) {
        const QString curlPath = QStandardPaths::findExecutable(
            HostOsInfo::withExecutableSuffix("curl"));
        if (!curlPath.isEmpty())
            curl.setValue(FilePath::fromString(curlPath));
    }
}

void AxivionSettings::toSettings() const
{
    writeTokenFile(tokensFilePath(), server);
    Utils::AspectContainer::writeSettings();
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
    enum Mode { Display, Edit };
    explicit DashboardSettingsWidget(Mode m, QWidget *parent, QPushButton *ok = nullptr);

    AxivionServer dashboardServer() const;
    void setDashboardServer(const AxivionServer &server);

    bool isValid() const;

private:
    Mode m_mode = Display;
    Id m_id;
    StringAspect m_dashboardUrl;
    StringAspect m_description;
    StringAspect m_token;
    BoolAspect m_valid;
};

DashboardSettingsWidget::DashboardSettingsWidget(Mode mode, QWidget *parent, QPushButton *ok)
    : QWidget(parent)
    , m_mode(mode)
{
    auto labelStyle = mode == Display ? StringAspect::LabelDisplay : StringAspect::LineEditDisplay;
    m_dashboardUrl.setLabelText(Tr::tr("Dashboard URL:"));
    m_dashboardUrl.setDisplayStyle(labelStyle);
    m_dashboardUrl.setValidationFunction([](FancyLineEdit *edit, QString *){
        return isUrlValid(edit->text());
    });
    m_description.setLabelText(Tr::tr("Description:"));
    m_description.setDisplayStyle(labelStyle);
    m_description.setPlaceHolderText(Tr::tr("Non-empty description"));

    m_token.setLabelText(Tr::tr("Access token:"));
    m_token.setDisplayStyle(labelStyle);
    m_token.setPlaceHolderText(Tr::tr("IDE Access Token"));
    m_token.setVisible(mode == Edit);

    using namespace Layouting;

    Form {
        m_dashboardUrl, br,
        m_description, br,
        m_token, br,
        mode == Edit ? normalMargin : noMargin
    }.attachTo(this);

    if (mode == Edit) {
        QTC_ASSERT(ok, return);
        auto checkValidity = [this, ok] {
            m_valid.setValue(isValid());
            ok->setEnabled(m_valid());
        };
        connect(&m_dashboardUrl, &BaseAspect::changed, this, checkValidity);
        connect(&m_description, &BaseAspect::changed, this, checkValidity);
        connect(&m_token, &BaseAspect::changed, this, checkValidity);
    }
}

AxivionServer DashboardSettingsWidget::dashboardServer() const
{
    AxivionServer result;
    if (m_id.isValid())
        result.id = m_id;
    else
        result.id = m_mode == Edit ? Utils::Id::fromName(QUuid::createUuid().toByteArray()) : m_id;
    result.dashboard = m_dashboardUrl();
    result.description = m_description();
    result.token = m_token();
    return result;
}

void DashboardSettingsWidget::setDashboardServer(const AxivionServer &server)
{
    m_id = server.id;
    m_dashboardUrl.setValue(server.dashboard);
    m_description.setValue(server.description);
    m_token.setValue(server.token);
}

bool DashboardSettingsWidget::isValid() const
{
    return !m_token().isEmpty() && !m_description().isEmpty() && isUrlValid(m_dashboardUrl());
}

class AxivionSettingsWidget : public Core::IOptionsPageWidget
{
public:
    AxivionSettingsWidget();

    void apply() override;

private:
    void showEditServerDialog();

    DashboardSettingsWidget *m_dashboardDisplay = nullptr;
    QPushButton *m_edit = nullptr;
};

AxivionSettingsWidget::AxivionSettingsWidget()
{
    using namespace Layouting;

    m_dashboardDisplay = new DashboardSettingsWidget(DashboardSettingsWidget::Display, this);
    m_dashboardDisplay->setDashboardServer(settings().server);
    m_edit = new QPushButton(Tr::tr("Edit..."), this);
    Row {
        Form {
            m_dashboardDisplay, br,
            settings().curl, br,
        }, Column { m_edit, st }
    }.attachTo(this);

    connect(m_edit, &QPushButton::clicked, this, &AxivionSettingsWidget::showEditServerDialog);
}

void AxivionSettingsWidget::apply()
{
    settings().server = m_dashboardDisplay->dashboardServer();
    settings().toSettings();
}

void AxivionSettingsWidget::showEditServerDialog()
{
    const AxivionServer old = m_dashboardDisplay->dashboardServer();
    QDialog d;
    d.setWindowTitle(Tr::tr("Edit Dashboard Configuration"));
    QVBoxLayout *layout = new QVBoxLayout;
    auto buttons = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Ok, this);
    auto ok = buttons->button(QDialogButtonBox::Ok);
    auto dashboardWidget = new DashboardSettingsWidget(DashboardSettingsWidget::Edit, this, ok);
    dashboardWidget->setDashboardServer(old);
    layout->addWidget(dashboardWidget);
    ok->setEnabled(m_dashboardDisplay->isValid());
    connect(buttons->button(QDialogButtonBox::Cancel), &QPushButton::clicked, &d, &QDialog::reject);
    connect(ok, &QPushButton::clicked, &d, &QDialog::accept);
    layout->addWidget(buttons);
    d.setLayout(layout);
    d.resize(500, 200);

    if (d.exec() != QDialog::Accepted)
        return;
    if (dashboardWidget->isValid()) {
        const AxivionServer server = dashboardWidget->dashboardServer();
        if (server != old)
            m_dashboardDisplay->setDashboardServer(server);
    }
}

// AxivionSettingsPage

class AxivionSettingsPage : public Core::IOptionsPage
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
