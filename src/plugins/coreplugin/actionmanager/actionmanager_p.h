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

#include <coreplugin/icontext.h>

#include <QMap>
#include <QHash>
#include <QMultiHash>
#include <QTimer>

#include <memory>

namespace Core {

class Command;

namespace Internal {

class ActionContainerPrivate;
class PresentationModeHandler;

class ActionManagerPrivate : public QObject
{
    Q_OBJECT

public:
    using IdCmdMap = QHash<Utils::Id, Command *>;
    using IdContainerMap = QHash<Utils::Id, ActionContainerPrivate *>;

    ActionManagerPrivate();
    ~ActionManagerPrivate() override;

    void setContext(const Context &context);
    bool hasContext(int context) const;

    void saveSettings();
    static void saveSettings(Command *cmd);

    bool hasContext(const Context &context) const;
    Command *overridableAction(Utils::Id id);

    static void readUserSettings(Utils::Id id, Command *cmd);

    void containerDestroyed(QObject *sender);

    IdCmdMap m_idCmdMap;

    IdContainerMap m_idContainerMap;

    Context m_context;

    std::unique_ptr<PresentationModeHandler> m_presentationModeHandler;
};

} // namespace Internal
} // namespace Core
