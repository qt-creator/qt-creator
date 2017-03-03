/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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
#pragma once

#include "coreplugin/id.h"
#include "projectexplorer/namedwidget.h"

#include <QWidget>

namespace Utils {
class DetailsWidget;
}


namespace Ios {
namespace Internal {
namespace Ui {
    class IosBuildSettingsWidget;
}

class IosBuildSettingsWidget : public ProjectExplorer::NamedWidget
{
    Q_OBJECT

public:
    explicit IosBuildSettingsWidget(const Core::Id &deviceType, const QString &signingIdentifier,
                                    bool isSigningAutoManaged, QWidget *parent = 0);
    ~IosBuildSettingsWidget();

public:
    bool isSigningAutomaticallyManaged() const;

private slots:
    void onSigningEntityComboIndexChanged();
    void onReset();

signals:
    void signingSettingsChanged(bool isAutoManaged, QString identifier);

private:
    void setDefaultSigningIdentfier(const QString &identifier) const;
    void configureSigningUi(bool autoManageSigning);
    void populateDevelopmentTeams();
    void populateProvisioningProfiles();
    QString selectedIdentifier() const;
    void updateInfoText();
    void updateWarningText();

private:
    Ui::IosBuildSettingsWidget *ui;
    Utils::DetailsWidget *m_detailsWidget;
    QString m_lastProfileSelection;
    QString m_lastTeamSelection;
    const Core::Id m_deviceType;
};

} // namespace Internal
} // namespace Ios
