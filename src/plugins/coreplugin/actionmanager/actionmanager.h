// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../core_global.h"
#include "../coreconstants.h"
#include "../icontext.h"
#include "command.h"

#include <QObject>
#include <QList>

QT_BEGIN_NAMESPACE
class QAction;
class QString;
QT_END_NAMESPACE

namespace Core {

class ActionContainer;
class Command;
class Context;
class ICore;

namespace Internal {
class CorePlugin;
class ICorePrivate;
class MainWindow;
} // Internal

class CORE_EXPORT ActionManager : public QObject
{
    Q_OBJECT
public:
    static ActionManager *instance();

    static ActionContainer *createMenu(Utils::Id id);
    static ActionContainer *createMenuBar(Utils::Id id);
    static ActionContainer *createTouchBar(Utils::Id id,
                                           const QIcon &icon,
                                           const QString &text = QString());

    static Command *registerAction(QAction *action, Utils::Id id,
                                   const Context &context = Context(Constants::C_GLOBAL),
                                   bool scriptable = false);

    static Command *command(Utils::Id id);
    static ActionContainer *actionContainer(Utils::Id id);

    static QList<Command *> commands();

    static void unregisterAction(QAction *action, Utils::Id id);

    static void setPresentationModeEnabled(bool enabled);
    static bool isPresentationModeEnabled();

    static QString withNumberAccelerator(const QString &text, const int number);

signals:
    void commandListChanged();
    void commandAdded(Utils::Id id);

private:
    ActionManager(QObject *parent = nullptr);
    ~ActionManager() override;
    static void saveSettings();
    static void setContext(const Context &context);

    friend class Core::Internal::CorePlugin; // initialization
    friend class Core::ICore; // saving settings and setting context
    friend class Core::Internal::ICorePrivate; // saving settings and setting context
};

} // namespace Core
