// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "axivionsettings.h"

#include "axiviontr.h"

#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/icore.h>

#include <utils/id.h>
#include <utils/layoutbuilder.h>
#include <utils/stringutils.h>

#include <QDialog>
#include <QDialogButtonBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPushButton>
#include <QRegularExpression>
#include <QUuid>
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

static void writeTokenFile(const FilePath &filePath, const AxivionServer &server)
{
    QJsonDocument doc;
    doc.setObject(server.toJson());
    // FIXME error handling?
    filePath.writeFileContents(doc.toJson());
    filePath.setPermissions(QFile::ReadUser | QFile::WriteUser);
}

static AxivionServer readTokenFile(const FilePath &filePath)
{
    if (!filePath.exists())
        return {};
    expected_str<QByteArray> contents = filePath.fileContents();
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

    highlightMarks.setSettingsKey("HighlightMarks");
    highlightMarks.setLabelText(Tr::tr("Highlight marks"));
    highlightMarks.setToolTip(Tr::tr("Marks issues on the scroll bar."));
    highlightMarks.setDefaultValue(false);
    AspectContainer::readSettings();

    m_server = readTokenFile(tokensFilePath());
}

void AxivionSettings::toSettings() const
{
    writeTokenFile(tokensFilePath(), m_server);
    AspectContainer::writeSettings();
}

Id AxivionSettings::defaultDashboardId() const
{
    return m_server.id;
}

const AxivionServer AxivionSettings::defaultServer() const
{
    return serverForId(defaultDashboardId());
}

const AxivionServer AxivionSettings::serverForId(const Utils::Id &id) const
{
    if (m_server.id == id)
        return m_server;
    return {};
}

void AxivionSettings::disableCertificateValidation(const Utils::Id &id)
{
    if (m_server.id == id)
        m_server.validateCert = false;
}

void AxivionSettings::modifyDashboardServer(const Utils::Id &id, const AxivionServer &other)
{
    if (m_server.id == id) {
        m_server = other;
        emit changed();
    }
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
    StringAspect m_username;
    BoolAspect m_valid;
};

DashboardSettingsWidget::DashboardSettingsWidget(Mode mode, QWidget *parent, QPushButton *ok)
    : QWidget(parent)
    , m_mode(mode)
{
    auto labelStyle = mode == Display ? StringAspect::LabelDisplay : StringAspect::LineEditDisplay;
    m_dashboardUrl.setLabelText(Tr::tr("Dashboard URL:"));
    m_dashboardUrl.setDisplayStyle(labelStyle);
    m_dashboardUrl.setValidationFunction([](FancyLineEdit *edit, QString *) {
        return isUrlValid(edit->text());
    });

    m_username.setLabelText(Tr::tr("Username:"));
    m_username.setDisplayStyle(labelStyle);
    m_username.setPlaceHolderText(Tr::tr("User name"));

    using namespace Layouting;

    Form {
        m_dashboardUrl, br,
        m_username, br,
        noMargin
    }.attachTo(this);

    if (mode == Edit) {
        QTC_ASSERT(ok, return);
        auto checkValidity = [this, ok] {
            m_valid.setValue(isValid());
            ok->setEnabled(m_valid());
        };
        connect(&m_dashboardUrl, &BaseAspect::changed, this, checkValidity);
        connect(&m_username, &BaseAspect::changed, this, checkValidity);
    }
}

AxivionServer DashboardSettingsWidget::dashboardServer() const
{
    AxivionServer result;
    if (m_id.isValid())
        result.id = m_id;
    else
        result.id = m_mode == Edit ? Id::fromName(QUuid::createUuid().toByteArray()) : m_id;
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
    void showEditServerDialog();

    DashboardSettingsWidget *m_dashboardDisplay = nullptr;
    QPushButton *m_edit = nullptr;
};

AxivionSettingsWidget::AxivionSettingsWidget()
{
    using namespace Layouting;

    m_dashboardDisplay = new DashboardSettingsWidget(DashboardSettingsWidget::Display, this);
    m_dashboardDisplay->setDashboardServer(settings().defaultServer());
    m_edit = new QPushButton(Tr::tr("Edit..."), this);
    Column {
        Row {
            Form {
                m_dashboardDisplay, br
            }, st,
            Column { m_edit },
        },
        Space(10), br,
        Row { settings().highlightMarks }, st
    }.attachTo(this);

    connect(m_edit, &QPushButton::clicked, this, &AxivionSettingsWidget::showEditServerDialog);
}

void AxivionSettingsWidget::apply()
{
    settings().modifyDashboardServer(settings().defaultDashboardId(),
                                     m_dashboardDisplay->dashboardServer());
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
