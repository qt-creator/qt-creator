// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>
#include <utils/aspects.h>
#include <utils/id.h>

#include <QPointer>
#include <QWidget>

namespace Axivion::Internal {

class AxivionServer;
class AxivionSettings;
class AxivionSettingsWidget;

class DashboardSettingsWidget : public QWidget
{
    Q_OBJECT
public:
    enum Mode { Display, Edit };
    explicit DashboardSettingsWidget(Mode m = Display, QWidget *parent = nullptr);

    AxivionServer dashboardServer() const;
    void setDashboardServer(const AxivionServer &server);

    bool isValid() const;

signals:
    void validChanged(bool valid);
private:
    Mode m_mode = Display;
    Utils::Id m_id;
    Utils::StringAspect m_dashboardUrl;
    Utils::StringAspect m_description;
    Utils::StringAspect m_token;
    bool m_valid = false;
};


class AxivionSettingsPage : public Core::IOptionsPage
{
public:
    explicit AxivionSettingsPage(AxivionSettings *settings);

private:
    AxivionSettings *m_settings;
};

} // Axivion::Internal
