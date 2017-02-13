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

#include <QStringList>
#include <QLabel>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QLineEdit;
class QRadioButton;
class QComboBox;
QT_END_NAMESPACE

namespace qbs { class InstallOptions; }

namespace Utils { class PathChooser; }

namespace ProjectExplorer { class BuildStepList; }

namespace QbsProjectManager {

class QbsProject;

namespace Internal {

class QbsRunConfigurationFactory;

class QbsRunConfiguration : public ProjectExplorer::RunConfiguration
{
    Q_OBJECT

    // to change the display name and arguments and set the userenvironmentchanges
    friend class QbsRunConfigurationWidget;
    friend class QbsRunConfigurationFactory;

public:
    QbsRunConfiguration(ProjectExplorer::Target *parent, Core::Id id);

    bool isEnabled() const override;
    QString disabledReason() const override;
    QWidget *createConfigurationWidget() override;

    ProjectExplorer::Runnable runnable() const override;

    QString executable() const;
    Utils::OutputFormatter *createOutputFormatter() const override;

    void addToBaseEnvironment(Utils::Environment &env) const;

    QString buildSystemTarget() const final;
    bool isConsoleApplication() const;

signals:
    void targetInformationChanged();
    void usingDyldImageSuffixChanged(bool);

protected:
    QbsRunConfiguration(ProjectExplorer::Target *parent, QbsRunConfiguration *source);

private:
    void installStepChanged();
    QString baseWorkingDirectory() const;
    QString defaultDisplayName();

    void ctor();

    void updateTarget();

    QString m_uniqueProductName;

    // Cached startup sub project information

    ProjectExplorer::BuildStepList *m_currentBuildStepList; // We do not take ownership!
};

class QbsRunConfigurationWidget : public QWidget
{
    Q_OBJECT

public:
    QbsRunConfigurationWidget(QbsRunConfiguration *rc);

private:
    void runConfigurationEnabledChange();
    void targetInformationHasChanged();
    void setExecutableLineText(const QString &text = QString());

    QbsRunConfiguration *m_rc;
    bool m_ignoreChange = false;
    QLabel *m_disabledIcon;
    QLabel *m_disabledReason;
    QLabel *m_executableLineLabel;
    bool m_isShown = false;
};

class QbsRunConfigurationFactory : public ProjectExplorer::IRunConfigurationFactory
{
    Q_OBJECT

public:
    explicit QbsRunConfigurationFactory(QObject *parent = 0);

    bool canCreate(ProjectExplorer::Target *parent, Core::Id id) const override;
    bool canRestore(ProjectExplorer::Target *parent, const QVariantMap &map) const override;
    bool canClone(ProjectExplorer::Target *parent, ProjectExplorer::RunConfiguration *source) const override;
    ProjectExplorer::RunConfiguration *clone(ProjectExplorer::Target *parent, ProjectExplorer::RunConfiguration *source) override;

    QList<Core::Id> availableCreationIds(ProjectExplorer::Target *parent, CreationMode mode) const override;
    QString displayNameForId(Core::Id id) const override;

private:
    bool canHandle(ProjectExplorer::Target *t) const;

    ProjectExplorer::RunConfiguration *doCreate(ProjectExplorer::Target *parent, Core::Id id) override;
    ProjectExplorer::RunConfiguration *doRestore(ProjectExplorer::Target *parent,
                                                 const QVariantMap &map) override;
};

} // namespace Internal
} // namespace QbsProjectManager
