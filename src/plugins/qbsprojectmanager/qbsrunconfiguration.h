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

#include <projectexplorer/runnables.h>

#include <QCheckBox>
#include <QLabel>
#include <QStringList>
#include <QWidget>

namespace QbsProjectManager {
namespace Internal {

class QbsRunConfiguration : public ProjectExplorer::RunConfiguration
{
    Q_OBJECT

    // to change the display name and arguments and set the userenvironmentchanges
    friend class QbsRunConfigurationWidget;

public:
    explicit QbsRunConfiguration(ProjectExplorer::Target *target);
    ~QbsRunConfiguration();

    QWidget *createConfigurationWidget() final;

    ProjectExplorer::Runnable runnable() const final;

    QString executable() const;
    Utils::OutputFormatter *createOutputFormatter() const final;

    void addToBaseEnvironment(Utils::Environment &env) const;

    QString buildSystemTarget() const final;
    bool isConsoleApplication() const;
    bool usingLibraryPaths() const { return m_usingLibraryPaths; }
    void setUsingLibraryPaths(bool useLibPaths);

signals:
    void targetInformationChanged();
    void usingDyldImageSuffixChanged(bool);

private:
    QVariantMap toMap() const final;
    bool fromMap(const QVariantMap &map) final;
    QString extraId() const final;
    void doAdditionalSetup(const ProjectExplorer::RunConfigurationCreationInfo &rci) final;
    bool canRunForNode(const ProjectExplorer::Node *node) const final;

    QString baseWorkingDirectory() const;
    QString defaultDisplayName();
    void handleBuildSystemDataUpdated();

    bool m_usingLibraryPaths = true;

    QString m_buildKey;
};

class QbsRunConfigurationFactory : public ProjectExplorer::RunConfigurationFactory
{
public:
    QbsRunConfigurationFactory();
};

} // namespace Internal
} // namespace QbsProjectManager
