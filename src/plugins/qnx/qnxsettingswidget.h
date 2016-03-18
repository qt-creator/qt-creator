/****************************************************************************
**
** Copyright (C) 2016 BlackBerry Limited. All rights reserved.
** Contact: BlackBerry (qt@blackberry.com)
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

#include <QWidget>

namespace Qnx {
namespace Internal {

class Ui_QnxSettingsWidget;
class QnxConfiguration;
class QnxConfigurationManager;

class QnxSettingsWidget : public QWidget
{
    Q_OBJECT

public:
    enum State {
        Activated,
        Deactivated,
        Added,
        Removed
    };

    class ConfigState {
    public:
        ConfigState(QnxConfiguration *config, State state)
        {
            this->config = config;
            this->state = state;
        }

        bool operator ==(const ConfigState& cs)
        {
            return config == cs.config && state == cs.state;
        }

        QnxConfiguration *config;
        State state;
    };

    explicit QnxSettingsWidget(QWidget *parent = 0);
    ~QnxSettingsWidget();
    QList<ConfigState> changedConfigs();

protected slots:
    void addConfiguration();
    void removeConfiguration();
    void generateKits(bool checked);
    void updateInformation();
    void populateConfigsCombo();

private:
    Ui_QnxSettingsWidget *m_ui;
    QnxConfigurationManager *m_qnxConfigManager;
    QList<ConfigState> m_changedConfigs;

    void setConfigState(QnxConfiguration *config, State state);

    void applyChanges();
    friend class QnxSettingsPage;

};

}
}
