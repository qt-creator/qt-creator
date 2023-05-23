// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#include "axivionsettingspage.h"

#include "axivionplugin.h"
#include "axivionsettings.h"
#include "axiviontr.h"

#include <coreplugin/icore.h>
#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>

#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QRegularExpression>
#include <QUuid>
#include <QVBoxLayout>

using namespace Utils;

namespace Axivion::Internal {

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

DashboardSettingsWidget::DashboardSettingsWidget(Mode mode, QWidget *parent)
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

    Row {
        Form {
            m_dashboardUrl,
            m_description,
            m_token,
            mode == Edit ? normalMargin : noMargin
        }
    }.attachTo(this);

    auto checkValidity = [this] {
        bool old = m_valid;
        m_valid = isValid();
        if (old != m_valid)
            emit validChanged(m_valid);
    };
    if (mode == Edit) {
        connect(&m_dashboardUrl, &StringAspect::valueChanged,
                this, checkValidity);
        connect(&m_description, &StringAspect::valueChanged,
                this, checkValidity);
        connect(&m_token, &StringAspect::valueChanged,
                this, checkValidity);
    }
}

AxivionServer DashboardSettingsWidget::dashboardServer() const
{
    AxivionServer result;
    if (m_id.isValid())
        result.id = m_id;
    else
        result.id = m_mode == Edit ? Utils::Id::fromName(QUuid::createUuid().toByteArray()) : m_id;
    result.dashboard = m_dashboardUrl.value();
    result.description = m_description.value();
    result.token = m_token.value();
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
    return !m_token.value().isEmpty() && !m_description.value().isEmpty()
            && isUrlValid(m_dashboardUrl.value());
}

class AxivionSettingsWidget : public Core::IOptionsPageWidget
{
public:
    explicit AxivionSettingsWidget(AxivionSettings *settings);

    void apply() override;
private:
    void showEditServerDialog();

    AxivionSettings *m_settings;

    Utils::StringAspect m_curlPC;
    DashboardSettingsWidget *m_dashboardDisplay = nullptr;
    QPushButton *m_edit = nullptr;
};

AxivionSettingsWidget::AxivionSettingsWidget(AxivionSettings *settings)
    : m_settings(settings)
{
    using namespace Layouting;

    m_dashboardDisplay = new DashboardSettingsWidget(DashboardSettingsWidget::Display, this);
    m_dashboardDisplay->setDashboardServer(m_settings->server);
    m_edit = new QPushButton(Tr::tr("Edit..."), this);
    m_curlPC.setLabelText(Tr::tr("curl:"));
    m_curlPC.setDisplayStyle(StringAspect::PathChooserDisplay);
    m_curlPC.setExpectedKind(PathChooser::ExistingCommand);
    m_curlPC.setFilePath(m_settings->curl);
    Grid {
        Form {
            m_dashboardDisplay, br,
            m_curlPC, br,
        }, Column { m_edit, st }
    }.attachTo(this);

    connect(m_edit, &QPushButton::clicked, this, &AxivionSettingsWidget::showEditServerDialog);
}

void AxivionSettingsWidget::apply()
{
    m_settings->server = m_dashboardDisplay->dashboardServer();
    m_settings->curl = m_curlPC.filePath();
    m_settings->toSettings(Core::ICore::settings());
    emit AxivionPlugin::instance()->settingsChanged();
}

void AxivionSettingsWidget::showEditServerDialog()
{
    const AxivionServer old = m_dashboardDisplay->dashboardServer();
    QDialog d;
    d.setWindowTitle(Tr::tr("Edit Dashboard Configuration"));
    QVBoxLayout *layout = new QVBoxLayout;
    DashboardSettingsWidget *dashboardWidget = new DashboardSettingsWidget(DashboardSettingsWidget::Edit, this);
    dashboardWidget->setDashboardServer(old);
    layout->addWidget(dashboardWidget);
    auto buttons = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Ok, this);
    auto ok = buttons->button(QDialogButtonBox::Ok);
    ok->setEnabled(m_dashboardDisplay->isValid());
    connect(buttons->button(QDialogButtonBox::Cancel), &QPushButton::clicked, &d, &QDialog::reject);
    connect(ok, &QPushButton::clicked, &d, &QDialog::accept);
    connect(dashboardWidget, &DashboardSettingsWidget::validChanged,
            ok, &QPushButton::setEnabled);
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

AxivionSettingsPage::AxivionSettingsPage(AxivionSettings *settings)
    : m_settings(settings)
{
    setId("Axivion.Settings.General");
    setDisplayName(Tr::tr("General"));
    setCategory("XY.Axivion");
    setDisplayCategory(Tr::tr("Axivion"));
    setCategoryIconPath(":/axivion/images/axivion.png");
    setWidgetCreator([this] { return new AxivionSettingsWidget(m_settings); });
}

} // Axivion::Internal
