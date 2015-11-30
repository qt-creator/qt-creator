/**************************************************************************
**
** Copyright (C) 2015 BlackBerry Limited. All rights reserved.
**
** Contact: BlackBerry (qt@blackberry.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QNXSETTINGSWIDGET_H
#define QNXSETTINGSWIDGET_H

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

#endif // QNXSETTINGSWIDGET_H
