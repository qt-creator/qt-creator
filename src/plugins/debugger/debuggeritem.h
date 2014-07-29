/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef DEBUGGER_DEBUGGERITEM_H
#define DEBUGGER_DEBUGGERITEM_H

#include "debugger_global.h"
#include "debuggerconstants.h"

#include <projectexplorer/abi.h>

#include <utils/fileutils.h>

#include <QList>
#include <QVariant>

namespace Debugger {

class DebuggerItemManager;
namespace Internal {
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

    QString displayName() const { return m_displayName; }
    void setDisplayName(const QString &displayName);

    DebuggerEngineType engineType() const { return m_engineType; }
    void setEngineType(const DebuggerEngineType &engineType);

    Utils::FileName command() const { return m_command; }
    void setCommand(const Utils::FileName &command);

    bool isAutoDetected() const { return m_isAutoDetected; }
    void setAutoDetected(bool isAutoDetected);

    QString autoDetectionSource() const { return m_autoDetectionSource; }
    void setAutoDetectionSource(const QString &autoDetectionSource);

    QList<ProjectExplorer::Abi> abis() const { return m_abis; }
    void setAbis(const QList<ProjectExplorer::Abi> &abis);
    void setAbi(const ProjectExplorer::Abi &abi);

    enum MatchLevel { DoesNotMatch, MatchesSomewhat, MatchesWell, MatchesPerfectly };
    MatchLevel matchTarget(const ProjectExplorer::Abi &targetAbi) const;

    QStringList abiNames() const;

    bool operator==(const DebuggerItem &other) const;

private:
    DebuggerItem(const QVariant &id);
    void reinitializeFromFile();

    QVariant m_id;
    QString m_displayName;
    DebuggerEngineType m_engineType;
    Utils::FileName m_command;
    bool m_isAutoDetected;
    QString m_autoDetectionSource;
    QList<ProjectExplorer::Abi> m_abis;

    friend class Internal::DebuggerItemConfigWidget;
    friend class Internal::DebuggerItemModel;
    friend class DebuggerItemManager;
};

} // namespace Debugger

#endif // DEBUGGER_DEBUGGERITEM_H
