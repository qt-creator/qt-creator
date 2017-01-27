/****************************************************************************
**
** Copyright (C) 2016 Tim Sander <tim@krieglstein.org>
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

#include <projectexplorer/runconfiguration.h>

namespace BareMetal {
namespace Internal {

class BareMetalRunConfigurationWidget;

class BareMetalRunConfiguration : public ProjectExplorer::RunConfiguration
{
    Q_OBJECT
    Q_DISABLE_COPY(BareMetalRunConfiguration)

    friend class BareMetalRunConfigurationFactory;
    friend class BareMetalRunConfigurationWidget;

public:
    explicit BareMetalRunConfiguration(ProjectExplorer::Target *parent, Core::Id id,
                                       const QString &projectFilePath);

    bool isEnabled() const override;
    QString disabledReason() const override;
    QWidget *createConfigurationWidget() override;
    Utils::OutputFormatter *createOutputFormatter() const override;

    virtual QString localExecutableFilePath() const;
    QString arguments() const;
    QString workingDirectory() const;
    void setWorkingDirectory(const QString &wd);

    QVariantMap toMap() const override;

    QString projectFilePath() const;

    QString buildSystemTarget() const final;

    static const char *IdPrefix;

signals:
    void deploySpecsChanged();
    void targetInformationChanged() const;

protected:
    BareMetalRunConfiguration(ProjectExplorer::Target *parent, BareMetalRunConfiguration *source);
    bool fromMap(const QVariantMap &map) override;
    QString defaultDisplayName();
    void setDisabledReason(const QString &reason) const;

private:
    void handleBuildSystemDataUpdated();
    void init();

    QString m_projectFilePath;
    mutable QString m_disabledReason;
    QString m_workingDirectory;
};

} // namespace Internal
} // namespace BareMetal
