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

#include "baremetalrunconfiguration.h"

namespace Utils { class Environment; }

namespace BareMetal {
namespace Internal {

class BareMetalCustomRunConfiguration : public BareMetalRunConfiguration
{
    Q_OBJECT
public:
    BareMetalCustomRunConfiguration(ProjectExplorer::Target *parent);
    BareMetalCustomRunConfiguration(ProjectExplorer::Target *parent,
                                    BareMetalCustomRunConfiguration *source);

    bool isEnabled() const override { return true; }
    bool isConfigured() const override;
    ConfigurationState ensureConfigured(QString *errorMessage) override;
    QWidget *createConfigurationWidget() override;
    Utils::OutputFormatter *createOutputFormatter() const override;

    virtual QString localExecutableFilePath() const override { return m_localExecutable; }

    void setLocalExecutableFilePath(const QString &executable) { m_localExecutable = executable; }

    static Core::Id runConfigId();
    static QString runConfigDefaultDisplayName();

    bool fromMap(const QVariantMap &map) override;
    QVariantMap toMap() const override;

private:
    QString m_localExecutable;
};

} // namespace Internal
} // namespace BareMetal
