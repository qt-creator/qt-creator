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

#include "qmlprojectmanager_global.h"

#include <projectexplorer/runconfiguration.h>

namespace Core { class IEditor; }

namespace QmlProjectManager {
class QmlProject;

namespace Internal { class QmlProjectRunConfigurationWidget; }

class QMLPROJECTMANAGER_EXPORT QmlProjectRunConfiguration : public ProjectExplorer::RunConfiguration
{
    Q_OBJECT
    friend class Internal::QmlProjectRunConfigurationWidget;
    friend class QmlProject; // to call updateEnabled()

public:
    QmlProjectRunConfiguration(ProjectExplorer::Target *target, Core::Id id);

    ProjectExplorer::Runnable runnable() const override;

    enum MainScriptSource {
        FileInEditor,
        FileInProjectFile,
        FileInSettings
    };
    MainScriptSource mainScriptSource() const;
    void setScriptSource(MainScriptSource source, const QString &settingsPath = QString());

    QString mainScript() const;

    QString disabledReason() const override;
    QWidget *createConfigurationWidget() override;
    QVariantMap toMap() const override;

    ProjectExplorer::Abi abi() const override;

signals:
    void scriptSourceChanged();

private:
    bool fromMap(const QVariantMap &map) override;

    void changeCurrentFile(Core::IEditor* = 0);
    void updateEnabledState() final;

    QString executable() const;
    QString commandLineArguments() const;

    // absolute path to current file (if being used)
    QString m_currentFileFilename;
    // absolute path to selected main script (if being used)
    QString m_mainScriptFilename;

    QString m_scriptFile;
    QString m_qmlViewerArgs;
};

namespace Internal {

class QmlProjectRunConfigurationFactory : public ProjectExplorer::FixedRunConfigurationFactory
{
public:
    QmlProjectRunConfigurationFactory();
};

} // namespace Internal
} // namespace QmlProjectManager
