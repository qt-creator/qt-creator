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

#include <coreplugin/id.h>

#include <projectexplorer/namedwidget.h>

#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>

namespace Ios {
namespace Internal {

class IosBuildSettingsWidget : public ProjectExplorer::NamedWidget
{
    Q_OBJECT

public:
    explicit IosBuildSettingsWidget(const Core::Id &deviceType, const QString &signingIdentifier,
                                    bool isSigningAutoManaged, QWidget *parent = nullptr);

    bool isSigningAutomaticallyManaged() const;

signals:
    void signingSettingsChanged(bool isAutoManaged, QString identifier);

private:
    void onSigningEntityComboIndexChanged();
    void onReset();

    void setDefaultSigningIdentfier(const QString &identifier) const;
    void configureSigningUi(bool autoManageSigning);
    void populateDevelopmentTeams();
    void populateProvisioningProfiles();
    QString selectedIdentifier() const;
    void updateInfoText();
    void updateWarningText();

private:
    QString m_lastProfileSelection;
    QString m_lastTeamSelection;
    const Core::Id m_deviceType;

    QPushButton *m_qmakeDefaults;
    QComboBox *m_signEntityCombo;
    QCheckBox *m_autoSignCheckbox;
    QLabel *m_signEntityLabel;
    QLabel *m_infoIconLabel;
    QLabel *m_infoLabel;
    QLabel *m_warningIconLabel;
    QLabel *m_warningLabel;
};

} // namespace Internal
} // namespace Ios
