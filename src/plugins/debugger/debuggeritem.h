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

#include "debugger_global.h"
#include "debuggerconstants.h"

#include <projectexplorer/abi.h>

#include <utils/fileutils.h>

#include <QDateTime>
#include <QList>
#include <QVariant>

namespace Debugger {

class DebuggerItemManager;

namespace Internal {
class DebuggerConfigWidget;
class DebuggerItemConfigWidget;
class DebuggerItemModel;
} // namespace Internal

// -----------------------------------------------------------------------
// DebuggerItem
// -----------------------------------------------------------------------

class DEBUGGER_EXPORT DebuggerItem
{
public:
    DebuggerItem();
    DebuggerItem(const QVariantMap &data);

    void createId();
    bool canClone() const { return true; }
    bool isValid() const;
    QString engineTypeName() const;

    QVariantMap toMap() const;

    QVariant id() const { return m_id; }

    QString displayName() const;
    QString unexpandedDisplayName() const { return m_unexpandedDisplayName; }
    void setUnexpandedDisplayName(const QString &unexpandedDisplayName);

    DebuggerEngineType engineType() const { return m_engineType; }
    void setEngineType(const DebuggerEngineType &engineType);

    Utils::FileName command() const { return m_command; }
    void setCommand(const Utils::FileName &command);

    bool isAutoDetected() const { return m_isAutoDetected; }
    void setAutoDetected(bool isAutoDetected);

    QString version() const;
    void setVersion(const QString &version);

    QString autoDetectionSource() const { return m_autoDetectionSource; }
    void setAutoDetectionSource(const QString &autoDetectionSource);

    QList<ProjectExplorer::Abi> abis() const { return m_abis; }
    void setAbis(const QList<ProjectExplorer::Abi> &abis);
    void setAbi(const ProjectExplorer::Abi &abi);

    enum MatchLevel { DoesNotMatch, MatchesSomewhat, MatchesWell, MatchesPerfectly };
    MatchLevel matchTarget(const ProjectExplorer::Abi &targetAbi) const;

    QStringList abiNames() const;
    QDateTime lastModified() const;

    QIcon decoration() const;
    QString validityMessage() const;

    bool operator==(const DebuggerItem &other) const;
    bool operator!=(const DebuggerItem &other) const { return !operator==(other); }

    void reinitializeFromFile();

    Utils::FileName workingDirectory() const { return m_workingDirectory; }
    void setWorkingDirectory(const Utils::FileName &workingPath) { m_workingDirectory = workingPath; }

private:
    DebuggerItem(const QVariant &id);
    void initMacroExpander();

    QVariant m_id;
    QString m_unexpandedDisplayName;
    DebuggerEngineType m_engineType;
    Utils::FileName m_command;
    Utils::FileName m_workingDirectory;
    bool m_isAutoDetected;
    QString m_autoDetectionSource;
    QString m_version;
    QList<ProjectExplorer::Abi> m_abis;
    QDateTime m_lastModified;

    friend class Internal::DebuggerConfigWidget;
    friend class Internal::DebuggerItemConfigWidget;
    friend class Internal::DebuggerItemModel;
    friend class DebuggerItemManager;
};

} // namespace Debugger
