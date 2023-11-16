// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../core_global.h"
#include "../coreconstants.h"
#include "../icontext.h"
#include "command.h"

#include <QAction>
#include <QList>

#include <functional>

namespace Utils { class ParameterAction; }

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

class CORE_EXPORT ActionBuilder
{
public:
    ActionBuilder(QObject *contextActionParent, const Utils::Id actionId = {});
    ~ActionBuilder();

    void setId(Utils::Id id);
    void setContext(const Utils::Id id);
    void setContext(const Core::Context &context);
    void setText(const QString &text);
    void setIconText(const QString &text);
    void setToolTip(const QString &toolTip);
    void setCommandAttribute(Core::Command::CommandAttribute attr);
    void setCommandDescription(const QString &desc);
    void setContainer(Utils::Id containerId, Utils::Id groupId = {}, bool needsToExist = true);
    void setOnTriggered(const std::function<void()> &func);
    void setOnTriggered(QObject *guard, const std::function<void()> &func);
    void setOnTriggered(QObject *guard, const std::function<void(bool)> &func);
    void setOnToggled(QObject *guard, const std::function<void(bool)> &func);
    void setDefaultKeySequence(const QKeySequence &seq);
    void setDefaultKeySequences(const QList<QKeySequence> &seqs);
    void setDefaultKeySequence(const QString &mac, const QString &nonMac);
    void setIcon(const QIcon &icon);
    void setIconVisibleInMenu(bool on);
    void setTouchBarIcon(const QIcon &icon);
    void setEnabled(bool on);
    void setChecked(bool on);
    void setVisible(bool on);
    void setCheckable(bool on);
    void setScriptable(bool on);
    void setMenuRole(QAction::MenuRole role);

    enum EnablingMode { AlwaysEnabled, EnabledWithParameter };
    void setParameterText(const QString &parametrizedText,
                          const QString &emptyText,
                          EnablingMode mode = EnabledWithParameter);


    Command *command() const;
    QAction *commandAction() const;
    QAction *contextAction() const;
    Utils::ParameterAction *contextParameterAction() const;
    void bindContextAction(QAction **dest);
    void bindContextAction(Utils::ParameterAction **dest);
    void augmentActionWithShortcutToolTip();

private:
    class ActionBuilderPrivate *d = nullptr;
};

class CORE_EXPORT Menu
{
public:
    Menu();

    void setId(Utils::Id id);
    void setTitle(const QString &title);
    void setContainer(Utils::Id containerId, Utils::Id groupId = {});
    void addSeparator();

private:
    ActionContainer *m_menu = nullptr;
};

class CORE_EXPORT ActionSeparator
{
public:
    ActionSeparator(Utils::Id id);
};

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

    static Command *createCommand(Utils::Id id);
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
