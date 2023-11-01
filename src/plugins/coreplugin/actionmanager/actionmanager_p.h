// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../icontext.h"

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

    void scheduleContainerUpdate(ActionContainerPrivate *actionContainer);
    void updateContainer();
    void containerDestroyed(QObject *sender);

    IdCmdMap m_idCmdMap;

    IdContainerMap m_idContainerMap;
    QSet<ActionContainerPrivate *> m_scheduledContainerUpdates;

    Context m_context;

    std::unique_ptr<PresentationModeHandler> m_presentationModeHandler;
};

} // namespace Internal
} // namespace Core
