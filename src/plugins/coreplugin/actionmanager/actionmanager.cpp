// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "actionmanager.h"
#include "actionmanager_p.h"
#include "actioncontainer_p.h"
#include "command_p.h"
#include "../icore.h"

#include <utils/action.h>
#include <utils/algorithm.h>
#include <utils/fadingindicator.h>
#include <utils/qtcassert.h>

#include <nanotrace/nanotrace.h>

#include <QApplication>
#include <QDebug>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>

namespace {
    enum { warnAboutFindFailures = 0 };
}

static const char kKeyboardSettingsKeyV2[] = "KeyboardShortcutsV2";

using namespace Core;
using namespace Core::Internal;
using namespace Utils;

namespace Core::Internal {

class PresentationModeHandler : public QObject
{
public:
    void connectCommand(Command *command);

private:
    void showShortcutPopup(const QString &shortcut);
};

void PresentationModeHandler::connectCommand(Command *command)
{
    QAction *action = command->action();
    if (action) {
        connect(action, &QAction::triggered,
                this, [this, action] { showShortcutPopup(action->shortcut().toString()); });
    }
}

void PresentationModeHandler::showShortcutPopup(const QString &shortcut)
{
    if (shortcut.isEmpty())
        return;

    QWidget *window = QApplication::activeWindow();
    if (!window) {
        if (!QApplication::topLevelWidgets().isEmpty()) {
            window = QApplication::topLevelWidgets().first();
        } else {
            window = ICore::mainWindow();
        }
    }
    Utils::FadingIndicator::showText(window, shortcut);
}

} // Core::Internal


namespace Core {

class ActionBuilderPrivate
{
public:
    ActionBuilderPrivate(QObject *contextActionParent, const Id actionId)
        : actionId(actionId)
        , m_parent(contextActionParent)
    {
        command = ActionManager::createCommand(actionId);
    }

    void registerAction()
    {
        QTC_ASSERT(actionId.isValid(), return);
        ActionManager::registerAction(contextAction(), actionId, context, scriptable);
    }

    Action *contextAction()
    {
        if (!m_contextAction) {
            QTC_CHECK(m_parent);
            m_contextAction = new Action(m_parent);
        }
        return m_contextAction;
    }

    void adopt(Action *action)
    {
        QTC_ASSERT(!m_contextAction,
                   qWarning() << QLatin1String("Cannot adopt context action for \"%1\"after it "
                                               "already has been created.")
                                     .arg(actionId.toString());
                   return);
        QTC_ASSERT(action,
                   qWarning() << QLatin1String("Adopt called with nullptr action for \"%1\".")
                                     .arg(actionId.toString()));
        m_contextAction = action;
    }

    Command *command = nullptr;

    Id actionId;
    Context context{Constants::C_GLOBAL};
    bool scriptable = false;

private:
    QObject *m_parent = nullptr;
    Action *m_contextAction = nullptr;
};

/*!
    \class Core::ActionBuilder
    \inheaderfile coreplugin/actionmanager/actionmanager.h
    \inmodule QtCreator
    \ingroup mainclasses

    \brief The ActionBuilder class is convienience class to set up
    \l{Core::Command}s.

    An action builder specifies properties of a \c{Core::Command} and
    a context action and uses \l{ActionManager::registerAction()} in its
    destructor to actually register the action for a set \l{Core::Context}
    for the Command.
*/

/*!
    Constructs an action builder for an action with the Id \a actionId.

    The \a contextActionParent is used to provide a QObject parent for the
    internally constructed QAction object to control its life time.

    This is typically the \c this pointer of the entity using \c ActionBuilder.
 */
ActionBuilder::ActionBuilder(QObject *contextActionParent, const Id actionId)
    : d(new ActionBuilderPrivate(contextActionParent, actionId))
{}

/*!
    Registers the created action with the set properties.

    \sa ActionManager::registerAction()
*/
ActionBuilder::~ActionBuilder()
{
    d->registerAction();
    delete d;
}

/*!
    Uses the given \a action is the contextAction for this builder.
    \a action must not be nullptr, and adopt() must be called before
    setting any actual properties like setText() or setIcon().

    Usually you should prefer passing a \c contextActionParent to the
    ActionBuilder constructor, and binding a QAction to the automatically
    created context action with bindContextAction().

    This method is sometimes useful if the caller manages the action's
    lifetime itself, and for example there is no QObject that can be
    the parent of an automatically created context action.
*/
ActionBuilder &ActionBuilder::adopt(Utils::Action *action)
{
    d->adopt(action);
    return *this;
}

/*!
    Sets the \c text property of the action under construction to \a text.

    \sa QAction::setText()
*/
ActionBuilder &ActionBuilder::setText(const QString &text)
{
    d->contextAction()->setText(text);
    return *this;
}

/*!
    Sets the \c iconText property of the action under construction to \a iconText.

    \sa QAction::setIconText()
*/
ActionBuilder &ActionBuilder::setIconText(const QString &iconText)
{
    d->contextAction()->setIconText(iconText);
    return *this;
}

ActionBuilder &ActionBuilder::setToolTip(const QString &toolTip)
{
    d->contextAction()->setToolTip(toolTip);
    return *this;
}

ActionBuilder &ActionBuilder::setCommandAttribute(Command::CommandAttribute attr)
{
    d->command->setAttribute(attr);
    return *this;
}

ActionBuilder &ActionBuilder::setCommandAttributes(Command::CommandAttributes attr)
{
    d->command->setAttributes(attr);
    return *this;
}

ActionBuilder &ActionBuilder::setCommandDescription(const QString &desc)
{
    d->command->setDescription(desc);
    return *this;
}

ActionBuilder &ActionBuilder::addToContainer(Id containerId, Id groupId, bool needsToExist)
{
    QTC_ASSERT(containerId.isValid(), return *this);
    if (ActionContainer *container = ActionManager::actionContainer(containerId)) {
        container->addAction(d->command, groupId);
        return *this;
    }
    QTC_CHECK(!needsToExist);
    return *this;
}

ActionBuilder &ActionBuilder::addToContainers(QList<Id> containerIds, Id groupId, bool needsToExist)
{
    for (const Id &containerId : containerIds)
        addToContainer(containerId, groupId, needsToExist);
    return *this;
}

ActionBuilder &ActionBuilder::addOnTriggered(const std::function<void()> &func)
{
    QObject::connect(d->contextAction(), &QAction::triggered, d->contextAction(), func);
    return *this;
}

ActionBuilder &ActionBuilder::setDefaultKeySequence(const QKeySequence &seq)
{
    d->command->setDefaultKeySequence(seq);
    return *this;
}

ActionBuilder &ActionBuilder::setDefaultKeySequences(const QList<QKeySequence> &seqs)
{
    d->command->setDefaultKeySequences(seqs);
    return *this;
}

ActionBuilder &ActionBuilder::setDefaultKeySequence(const QString &mac, const QString &nonMac)
{
    d->command->setDefaultKeySequence(QKeySequence(useMacShortcuts ? mac : nonMac));
    return *this;
}

ActionBuilder &ActionBuilder::setIcon(const QIcon &icon)
{
    d->contextAction()->setIcon(icon);
    return *this;
}

ActionBuilder &ActionBuilder::setIconVisibleInMenu(bool on)
{
    d->contextAction()->setIconVisibleInMenu(on);
    return *this;
}

ActionBuilder &ActionBuilder::setTouchBarIcon(const QIcon &icon)
{
    d->command->setTouchBarIcon(icon);
    return *this;
}

ActionBuilder &ActionBuilder::setTouchBarText(const QString &text)
{
    d->command->setTouchBarText(text);
    return *this;
}

ActionBuilder &ActionBuilder::setEnabled(bool on)
{
    d->contextAction()->setEnabled(on);
    return *this;
}

ActionBuilder &ActionBuilder::setChecked(bool on)
{
    d->contextAction()->setChecked(on);
    return *this;
}

ActionBuilder &ActionBuilder::setVisible(bool on)
{
    d->contextAction()->setVisible(on);
    return *this;
}

ActionBuilder &ActionBuilder::setCheckable(bool on)
{
    d->contextAction()->setCheckable(on);
    return *this;
}

ActionBuilder &ActionBuilder::setSeperator(bool on)
{
    d->contextAction()->setSeparator(on);
    return *this;
}

ActionBuilder &ActionBuilder::setScriptable(bool on)
{
    d->scriptable = on;
    return *this;
}

ActionBuilder &ActionBuilder::setMenuRole(QAction::MenuRole role)
{
    d->contextAction()->setMenuRole(role);
    return *this;
}

ActionBuilder &ActionBuilder::setParameterText(const QString &parameterText,
                                     const QString &emptyText,
                                     EnablingMode mode)
{
    QTC_CHECK(parameterText.contains("%1"));
    QTC_CHECK(!emptyText.contains("%1"));

    d->contextAction()->setEmptyText(emptyText);
    d->contextAction()->setParameterText(parameterText);
    d->contextAction()->setEnablingMode(mode == AlwaysEnabled
                                            ? Action::AlwaysEnabled
                                            : Action::EnabledWithParameter);
    d->contextAction()->setText(emptyText);
    return *this;
}

Id ActionBuilder::id() const
{
    return d->actionId;
}

Command *ActionBuilder::command() const
{
    return d->command;
}

QAction *ActionBuilder::commandAction() const
{
    return d->command->action();
}

Action *ActionBuilder::contextAction() const
{
    return d->contextAction();
}

ActionBuilder &ActionBuilder::bindContextAction(QAction **dest)
{
    QTC_ASSERT(dest, return *this);
    *dest = d->contextAction();
    return *this;
}

ActionBuilder &ActionBuilder::bindContextAction(Utils::Action **dest)
{
    QTC_ASSERT(dest, return *this);
    *dest = d->contextAction();
    return *this;
}

ActionBuilder &ActionBuilder::bindCommand(Command **dest)
{
    QTC_ASSERT(dest, return *this);
    *dest = d->command;
    return *this;
}

ActionBuilder &ActionBuilder::augmentActionWithShortcutToolTip()
{
    d->command->augmentActionWithShortcutToolTip(d->contextAction());
    return *this;
}

ActionBuilder &ActionBuilder::setContext(Id id)
{
    d->context = Context(id);
    return *this;
}

ActionBuilder &ActionBuilder::setContext(const Context &context)
{
    QTC_ASSERT(!context.isEmpty(), return *this);
    d->context = context;
    return *this;
}

// Separator

ActionSeparator::ActionSeparator(Id id)
{
    ActionContainer *container = ActionManager::actionContainer(id);
    QTC_ASSERT(container, return);
    container->addSeparator();
}

// MenuBuilder

MenuBuilder::MenuBuilder(Id id)
{
    m_menu = ActionManager::createMenu(id);
}

MenuBuilder::~MenuBuilder() = default;

MenuBuilder &MenuBuilder::setTitle(const QString &title)
{
    m_menu->menu()->setTitle(title);
    return *this;
}

MenuBuilder &MenuBuilder::setIcon(const QIcon &icon)
{
    m_menu->menu()->setIcon(icon);
    return *this;
}

MenuBuilder &MenuBuilder::setOnAllDisabledBehavior(ActionContainer::OnAllDisabledBehavior behavior)
{
    m_menu->setOnAllDisabledBehavior(behavior);
    return *this;
}

MenuBuilder &MenuBuilder::addToContainer(Id containerId, Id groupId)
{
    ActionContainer *container = ActionManager::actionContainer(containerId);
    if (QTC_GUARD(container))
        container->addMenu(m_menu, groupId);
    return *this;
}

MenuBuilder &MenuBuilder::addSeparator()
{
    m_menu->addSeparator();
    return *this;
}
/*!
    \class Core::ActionManager
    \inheaderfile coreplugin/actionmanager/actionmanager.h
    \ingroup mainclasses
    \inmodule QtCreator

    \brief The ActionManager class is responsible for registration of menus and
    menu items and keyboard shortcuts.

    The action manager is the central bookkeeper of actions and their shortcuts
    and layout. It is a singleton containing mostly static functions. If you
    need access to the instance, for example for connecting to signals, call
    its ActionManager::instance() function.

    The action manager makes it possible to provide a central place where the
    users can specify all their keyboard shortcuts, and provides a solution for
    actions that should behave differently in different contexts (like the
    copy/replace/undo/redo actions).

    See \l{The Action Manager and Commands} for an overview of the interaction
    between Core::ActionManager, Core::Command, and Core::Context.

    Register a globally active action "My Action" by putting the following in
    your plugin's ExtensionSystem::IPlugin::initialize() function.

    \code
        QAction *myAction = new QAction(Tr::tr("My Action"), this);
        Command *cmd = ActionManager::registerAction(myAction, "myplugin.myaction", Context(C_GLOBAL));
        cmd->setDefaultKeySequence(QKeySequence(Tr::tr("Ctrl+Alt+u")));
        connect(myAction, &QAction::triggered, this, &MyPlugin::performMyAction);
    \endcode

    The \c connect is done to your own QAction instance. If you create for
    example a tool button that should represent the action, add the action from
    Command::action() to it.

    \code
        QToolButton *myButton = new QToolButton(someParentWidget);
        myButton->setDefaultAction(cmd->action());
    \endcode

    Also use the action manager to add items to registered action containers
    like the application's menu bar or menus in that menu bar. Register your
    action via the Core::ActionManager::registerAction() function, get the
    action container for a specific ID (as specified for example in the
    Core::Constants namespace) with Core::ActionManager::actionContainer(), and
    add your command to this container.

    Building on the example, adding "My Action" to the "Tools" menu would be
    done with

    \code
        ActionManager::actionContainer(Core::Constants::M_TOOLS)->addAction(cmd);
    \endcode

    \sa Core::ICore
    \sa Core::Command
    \sa Core::ActionContainer
    \sa Core::IContext
    \sa {The Action Manager and Commands}
*/

/*!
    \fn void Core::ActionManager::commandListChanged()

    Emitted when the command list has changed.
*/

/*!
    \fn void Core::ActionManager::commandAdded(Utils::Id id)

    Emitted when a command (with the \a id) is added.
*/

static ActionManager *m_instance = nullptr;
static ActionManagerPrivate *d;

/*!
    \internal
*/
ActionManager::ActionManager(QObject *parent)
    : QObject(parent)
{
    m_instance = this;
    d = new ActionManagerPrivate;
    if (Utils::HostOsInfo::isMacHost())
        QCoreApplication::setAttribute(Qt::AA_DontShowIconsInMenus);
}

/*!
    \internal
*/
ActionManager::~ActionManager()
{
    delete d;
}

/*!
    Returns the pointer to the instance. Only use for connecting to signals.
*/
ActionManager *ActionManager::instance()
{
    return m_instance;
}

/*!
    Creates a new menu action container or returns an existing container with
    the specified \a id. The ActionManager owns the returned ActionContainer.
    Add your menu to some other menu or a menu bar via the actionContainer()
    and ActionContainer::addMenu() functions.

    \sa actionContainer()
    \sa ActionContainer::addMenu()
*/
ActionContainer *ActionManager::createMenu(Id id)
{
    const ActionManagerPrivate::IdContainerMap::const_iterator it = d->m_idContainerMap.constFind(id);
    if (it !=  d->m_idContainerMap.constEnd())
        return it.value();

    auto mc = new MenuActionContainer(id, d);

    d->m_idContainerMap.insert(id, mc);
    connect(mc, &QObject::destroyed, d, &ActionManagerPrivate::containerDestroyed);

    return mc;
}

/*!
    Creates a new menu bar action container or returns an existing container
    with the specified \a id. The ActionManager owns the returned
    ActionContainer.

    \sa createMenu()
    \sa ActionContainer::addMenu()
*/
ActionContainer *ActionManager::createMenuBar(Id id)
{
    const ActionManagerPrivate::IdContainerMap::const_iterator it = d->m_idContainerMap.constFind(id);
    if (it !=  d->m_idContainerMap.constEnd())
        return it.value();

    auto mb = new QMenuBar; // No parent (System menu bar on macOS)
    mb->setObjectName(id.toString());

    auto mbc = new MenuBarActionContainer(id, d);
    mbc->setMenuBar(mb);

    d->m_idContainerMap.insert(id, mbc);
    connect(mbc, &QObject::destroyed, d, &ActionManagerPrivate::containerDestroyed);

    return mbc;
}

/*!
    Creates a new (sub) touch bar action container or returns an existing
    container with the specified \a id. The ActionManager owns the returned
    ActionContainer.

    Note that it is only possible to create a single level of sub touch bars.
    The sub touch bar will be represented as a button with \a icon and \a text
    (either of which can be left empty), which opens the sub touch bar when
    touched.

    \sa actionContainer()
    \sa ActionContainer::addMenu()
*/
ActionContainer *ActionManager::createTouchBar(Id id, const QIcon &icon, const QString &text)
{
    QTC_CHECK(!icon.isNull() || !text.isEmpty());
    ActionContainer * const c = d->m_idContainerMap.value(id);
    if (c)
        return c;
    auto ac = new TouchBarActionContainer(id, d, icon, text);
    d->m_idContainerMap.insert(id, ac);
    connect(ac, &QObject::destroyed, d, &ActionManagerPrivate::containerDestroyed);
    return ac;
}

/*!
    Makes an \a action known to the system under the specified \a id.

    Returns a Command instance that represents the action in the application
    and is owned by the ActionManager. You can register several actions with
    the same \a id as long as the \a context is different. In this case
    triggering the action is forwarded to the registered QAction for the
    currently active context. If the optional \a context argument is not
    specified, the global context will be assumed. A \a scriptable action can
    be called from a script without the need for the user to interact with it.
*/
Command *ActionManager::registerAction(QAction *action, Id id, const Context &context, bool scriptable)
{
    Command *cmd = d->overridableAction(id);
    if (cmd) {
        cmd->d->addOverrideAction(action, context, scriptable);
        emit m_instance->commandListChanged();
        emit m_instance->commandAdded(id);
    }
    return cmd;
}

/*!
    Creates a Command or returns an existing Command with the specified \a id.

    The created command doesn't have any actions associated with it yet, so
    it cannot actually be triggered.
    But the system is aware of it, it appears in the keyboard shortcut
    settings, and QActions can later be registered for it.
    If you already have a QAction, ID and Context that you want to register,
    there is no need to call this. Just directly call registerAction().
*/
Command *ActionManager::createCommand(Utils::Id id)
{
    return d->overridableAction(id);
}

/*!
    Returns the Command instance that has been created with registerAction()
    for the specified \a id.

    \sa registerAction()
*/
Command *ActionManager::command(Id id)
{
    const ActionManagerPrivate::IdCmdMap::const_iterator it = d->m_idCmdMap.constFind(id);
    if (it == d->m_idCmdMap.constEnd()) {
        if (warnAboutFindFailures)
            qWarning() << "ActionManagerPrivate::command(): failed to find :"
                       << id.name();
        return nullptr;
    }
    return it.value();
}

/*!
    Returns the ActionContainter instance that has been created with
    createMenu(), createMenuBar(), createTouchBar() for the specified \a id.

    Use the ID \c{Core::Constants::MENU_BAR} to retrieve the main menu bar.

    Use the IDs \c{Core::Constants::M_FILE}, \c{Core::Constants::M_EDIT}, and
    similar constants to retrieve the various default menus.

    Use the ID \c{Core::Constants::TOUCH_BAR} to retrieve the main touch bar.

    \sa ActionManager::createMenu()
    \sa ActionManager::createMenuBar()
*/
ActionContainer *ActionManager::actionContainer(Id id)
{
    const ActionManagerPrivate::IdContainerMap::const_iterator it = d->m_idContainerMap.constFind(id);
    if (it == d->m_idContainerMap.constEnd()) {
        if (warnAboutFindFailures)
            qWarning() << "ActionManagerPrivate::actionContainer(): failed to find :"
                       << id.name();
        return nullptr;
    }
    return it.value();
}

/*!
    Returns all registered commands.
*/
QList<Command *> ActionManager::commands()
{
    return d->m_idCmdMap.values();
}

/*!
    Removes the knowledge about an \a action under the specified \a id.

    Usually you do not need to unregister actions. The only valid use case for unregistering
    actions, is for actions that represent user definable actions, like for the custom Locator
    filters. If the user removes such an action, it also has to be unregistered from the action manager,
    to make it disappear from shortcut settings etc.
*/
void ActionManager::unregisterAction(QAction *action, Id id)
{
    Command *cmd = d->m_idCmdMap.value(id, nullptr);
    if (!cmd) {
        qWarning() << "unregisterAction: id" << id.name()
                   << "is registered with a different command type.";
        return;
    }
    cmd->d->removeOverrideAction(action);
    if (cmd->d->isEmpty()) {
        // clean up
        ActionManagerPrivate::saveSettings(cmd);
        ICore::mainWindow()->removeAction(cmd->action());
        // ActionContainers listen to the commands' destroyed signals
        delete cmd->action();
        d->m_idCmdMap.remove(id);
        delete cmd;
    }
    emit m_instance->commandListChanged();
}

/*!
    \internal
*/
void ActionManager::setPresentationModeEnabled(bool enabled)
{
    if (enabled == isPresentationModeEnabled())
        return;

    if (!enabled) {
        d->m_presentationModeHandler.reset();
        return;
    }

    d->m_presentationModeHandler.reset(new PresentationModeHandler);
    const auto commandList = commands();
    for (Command *command : commandList)
        d->m_presentationModeHandler->connectCommand(command);
}

/*!
    Returns whether presentation mode is enabled.

    The presentation mode is enabled when starting \QC with the command line
    argument \c{-presentationMode}. In presentation mode, \QC displays any
    pressed shortcut in an overlay box.
*/
bool ActionManager::isPresentationModeEnabled()
{
    return bool(d->m_presentationModeHandler);
}

/*!
    Decorates the specified \a text with a numbered accelerator key \a number,
    in the style of the \uicontrol {Recent Files} menu.
*/
QString ActionManager::withNumberAccelerator(const QString &text, const int number)
{
    if (Utils::HostOsInfo::isMacHost() || number > 9)
        return text;
    return QString("&%1 | %2").arg(number).arg(text);
}

/*!
    \internal
*/
void ActionManager::saveSettings()
{
    d->saveSettings();
}

/*!
    \internal
*/
void ActionManager::setContext(const Context &context)
{
    d->setContext(context);
}

/*!
    \class ActionManagerPrivate
    \inheaderfile actionmanager_p.h
    \internal
*/

ActionManagerPrivate::ActionManagerPrivate() = default;

ActionManagerPrivate::~ActionManagerPrivate()
{
    // first delete containers to avoid them reacting to command deletion
    for (const ActionContainerPrivate *container : std::as_const(m_idContainerMap))
        disconnect(container, &QObject::destroyed, this, &ActionManagerPrivate::containerDestroyed);
    qDeleteAll(m_idContainerMap);
    qDeleteAll(m_idCmdMap);
}

void ActionManagerPrivate::setContext(const Context &context)
{
    // here are possibilities for speed optimization if necessary:
    // let commands (de-)register themselves for contexts
    // and only update commands that are either in old or new contexts
    m_context = context;
    const IdCmdMap::const_iterator cmdcend = m_idCmdMap.constEnd();
    for (IdCmdMap::const_iterator it = m_idCmdMap.constBegin(); it != cmdcend; ++it)
        it.value()->d->setCurrentContext(m_context);
}

bool ActionManagerPrivate::hasContext(const Context &context) const
{
    for (int i = 0; i < m_context.size(); ++i) {
        if (context.contains(m_context.at(i)))
            return true;
    }
    return false;
}

void ActionManagerPrivate::containerDestroyed(QObject *sender)
{
    auto container = static_cast<ActionContainerPrivate *>(sender);
    m_idContainerMap.remove(m_idContainerMap.key(container));
    m_scheduledContainerUpdates.remove(container);
}

Command *ActionManagerPrivate::overridableAction(Id id)
{
    Command *cmd = m_idCmdMap.value(id, nullptr);
    if (!cmd) {
        cmd = new Command(id);
        m_idCmdMap.insert(id, cmd);
        readUserSettings(id, cmd);
        QAction *action = cmd->action();
        ICore::mainWindow()->addAction(action);
        action->setObjectName(id.toString());
        action->setShortcutContext(Qt::ApplicationShortcut);
        cmd->d->setCurrentContext(m_context);

        if (d->m_presentationModeHandler)
            d->m_presentationModeHandler->connectCommand(cmd);
    }

    return cmd;
}

void ActionManagerPrivate::readUserSettings(Id id, Command *cmd)
{
    QtcSettings *settings = ICore::settings();
    settings->beginGroup(kKeyboardSettingsKeyV2);
    if (settings->contains(id.toKey())) {
        const QVariant v = settings->value(id.toKey());
        if (v.typeId() == QMetaType::QStringList) {
            cmd->setKeySequences(Utils::transform<QList>(v.toStringList(), [](const QString &s) {
                return QKeySequence::fromString(s);
            }));
        } else {
            cmd->setKeySequences({QKeySequence::fromString(v.toString())});
        }
    }
    settings->endGroup();
}

void ActionManagerPrivate::scheduleContainerUpdate(ActionContainerPrivate *actionContainer)
{
    const bool needsSchedule = m_scheduledContainerUpdates.isEmpty();
    m_scheduledContainerUpdates.insert(actionContainer);
    if (needsSchedule)
        QMetaObject::invokeMethod(this,
                                  &ActionManagerPrivate::updateContainer,
                                  Qt::QueuedConnection);
}

void ActionManagerPrivate::updateContainer()
{
    for (ActionContainerPrivate *c : std::as_const(m_scheduledContainerUpdates))
        c->update();
    m_scheduledContainerUpdates.clear();
}

void ActionManagerPrivate::saveSettings(Command *cmd)
{
    const Key id = cmd->id().toKey();
    const Key settingsKey = Key(kKeyboardSettingsKeyV2) + '/' + id;
    const QList<QKeySequence> keys = cmd->keySequences();
    const QList<QKeySequence> defaultKeys = cmd->defaultKeySequences();
    if (keys != defaultKeys) {
        if (keys.isEmpty()) {
            ICore::settings()->setValue(settingsKey, QString());
        } else if (keys.size() == 1) {
            ICore::settings()->setValue(settingsKey, keys.first().toString());
        } else {
            ICore::settings()->setValue(settingsKey,
                                        Utils::transform<QStringList>(keys,
                                                                      [](const QKeySequence &k) {
                                                                          return k.toString();
                                                                      }));
        }
    } else {
        ICore::settings()->remove(settingsKey);
    }
}

void ActionManagerPrivate::saveSettings()
{
    const IdCmdMap::const_iterator cmdcend = m_idCmdMap.constEnd();
    for (IdCmdMap::const_iterator j = m_idCmdMap.constBegin(); j != cmdcend; ++j) {
        saveSettings(j.value());
    }
}

} // Core
