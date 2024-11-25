// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../core_global.h"
#include "../coreconstants.h"
#include "../icontext.h"
#include "actioncontainer.h" // For enum ActionContainer::OnAllDisabledBehavior
#include "command.h"

#include <utils/action.h>

#include <functional>

namespace Core {

class ICore;

namespace Internal {
class CorePlugin;
class ICorePrivate;
} // Internal

class CORE_EXPORT ActionBuilder
{
public:
    ActionBuilder(QObject *contextActionParent, const Utils::Id actionId);
    ~ActionBuilder();

    ActionBuilder &adopt(Utils::Action *action);

    ActionBuilder &setContext(const Utils::Id id);
    ActionBuilder &setContext(const Core::Context &context);
    ActionBuilder &setText(const QString &text);
    ActionBuilder &setIconText(const QString &iconText);
    ActionBuilder &setToolTip(const QString &toolTip);
    ActionBuilder &setCommandAttribute(Core::Command::CommandAttribute attr);
    ActionBuilder &setCommandAttributes(Core::Command::CommandAttributes attr);
    ActionBuilder &setCommandDescription(const QString &desc);
    ActionBuilder &addToContainer(Utils::Id containerId, Utils::Id groupId = {}, bool needsToExist = true);
    ActionBuilder &addToContainers(QList<Utils::Id> containerIds, Utils::Id groupId = {},
                         bool needsToExist = true);
    ActionBuilder &addOnTriggered(const std::function<void()> &func);

    template<class T, typename F>
    ActionBuilder &addOnTriggered(T *guard,
                        F &&function,
                        Qt::ConnectionType connectionType = Qt::AutoConnection)
    {
        QObject::connect(contextAction(),
                         &QAction::triggered,
                         guard,
                         std::forward<F>(function),
                         connectionType);
        return *this;
    }

    template<class T, typename F>
    ActionBuilder &addOnToggled(T *guard,
                        F &&function,
                        Qt::ConnectionType connectionType = Qt::AutoConnection)
    {
        QObject::connect(contextAction(),
                         &QAction::toggled,
                         guard,
                         std::forward<F>(function),
                         connectionType);
        return *this;
    }

    ActionBuilder &setDefaultKeySequence(const QKeySequence &seq);
    ActionBuilder &setDefaultKeySequences(const QList<QKeySequence> &seqs);
    ActionBuilder &setDefaultKeySequence(const QString &mac, const QString &nonMac);
    ActionBuilder &setIcon(const QIcon &icon);
    ActionBuilder &setIconVisibleInMenu(bool on);
    ActionBuilder &setTouchBarIcon(const QIcon &icon);
    ActionBuilder &setTouchBarText(const QString &text);
    ActionBuilder &setEnabled(bool on);
    ActionBuilder &setChecked(bool on);
    ActionBuilder &setVisible(bool on);
    ActionBuilder &setCheckable(bool on);
    ActionBuilder &setSeperator(bool on);
    ActionBuilder &setScriptable(bool on);
    ActionBuilder &setMenuRole(QAction::MenuRole role);

    enum EnablingMode { AlwaysEnabled, EnabledWithParameter };
    ActionBuilder &setParameterText(const QString &parametrizedText,
                          const QString &emptyText,
                          EnablingMode mode = EnabledWithParameter);

    ActionBuilder &bindContextAction(QAction **dest);
    ActionBuilder &bindContextAction(Utils::Action **dest);
    ActionBuilder &bindCommand(Command **dest);
    ActionBuilder &augmentActionWithShortcutToolTip();

    Utils::Id id() const;
    Command *command() const;
    QAction *commandAction() const;
    Utils::Action *contextAction() const;

private:
    class ActionBuilderPrivate *d = nullptr;
};

class CORE_EXPORT MenuBuilder
{
public:
    MenuBuilder(Utils::Id menuId);
    ~MenuBuilder();

    MenuBuilder &setTitle(const QString &title);
    MenuBuilder &setIcon(const QIcon &icon);
    MenuBuilder &setOnAllDisabledBehavior(ActionContainer::OnAllDisabledBehavior behavior);
    MenuBuilder &addToContainer(Utils::Id containerId, Utils::Id groupId = {});
    MenuBuilder &addSeparator();

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
