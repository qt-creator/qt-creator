// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "modemanager.h"

#include "actionmanager/actionmanager.h"
#include "actionmanager/command.h"
#include "coreplugintr.h"
#include "fancyactionbar.h"
#include "fancytabwidget.h"
#include "icore.h"
#include "imode.h"

#include <extensionsystem/pluginmanager.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QAction>
#include <QActionGroup>
#include <QDebug>
#include <QMap>
#include <QMouseEvent>
#include <QVector>

using namespace Utils;

namespace Core {

/*!
    \class Core::ModeManager
    \inheaderfile coreplugin/modemanager.h
    \ingroup mainclasses
    \inmodule QtCreator

    \brief The ModeManager class manages the activation of modes and the
    actions in the mode selector's tool bar.

    Modes are implemented with the IMode class. Use the ModeManager to
    force activation of a mode, or to be notified when the active mode changed.

    The ModeManager also manages the actions that are visible in the mode
    selector's toolbar. Adding actions to the tool bar should be done very
    sparingly.
*/

/*!
    \enum ModeManager::Style
    \internal
*/

/*!
    \fn void ModeManager::currentModeAboutToChange(Utils::Id mode)

    Emitted before the current mode changes to \a mode.
*/

/*!
    \fn void ModeManager::currentModeChanged(Utils::Id mode, Utils::Id oldMode)

    Emitted after the current mode changed from \a oldMode to \a mode.
*/

struct ModeManagerPrivate
{
    void showMenu(int index, QMouseEvent *event);
    void appendMode(IMode *mode);
    void ensureVisibleEnabledMode();
    void enabledStateChanged(IMode *mode);
    void visibleChanged(IMode *mode);
    void activateModeHelper(Id id);
    void registerModeSelectorStyleActions();
    void updateModeSelectorStyleMenu();
    void extensionsInitializedHelper();

    Internal::FancyTabWidget *m_modeStack;
    Internal::FancyActionBar *m_actionBar;
    QHash<QAction *, int> m_actions;
    QVector<IMode *> m_modes;
    QVector<Command *> m_modeCommands;
    Context m_addedContexts;
    int m_oldCurrent;
    ModeManager::Style m_modeStyle = ModeManager::Style::IconsAndText;
    QAction *m_setModeSelectorStyleIconsAndTextAction = nullptr;
    QAction *m_setModeSelectorStyleHiddenAction = nullptr;
    QAction *m_setModeSelectorStyleIconsOnlyAction = nullptr;

    bool m_startingUp = true;
    Id m_pendingFirstActiveMode; // Valid before extentionsInitialized.
};

static ModeManagerPrivate *d;
static ModeManager *m_instance = nullptr;

static int indexOf(Id id)
{
    for (int i = 0; i < d->m_modes.count(); ++i) {
        if (d->m_modes.at(i)->id() == id)
            return i;
    }
    qDebug() << "Warning, no such mode:" << id.toString();
    return -1;
}

void ModeManagerPrivate::showMenu(int index, QMouseEvent *event)
{
    QTC_ASSERT(m_modes.at(index)->menu(), return);
    m_modes.at(index)->menu()->popup(event->globalPosition().toPoint());
}

ModeManager::ModeManager(Internal::FancyTabWidget *modeStack)
{
    m_instance = this;
    d = new ModeManagerPrivate();
    d->m_modeStack = modeStack;
    d->m_oldCurrent = -1;
    d->m_actionBar = new Internal::FancyActionBar(modeStack);
    d->m_modeStack->addCornerWidget(d->m_actionBar);
    setModeStyle(d->m_modeStyle);

    connect(d->m_modeStack, &Internal::FancyTabWidget::currentAboutToShow,
            this, &ModeManager::currentTabAboutToChange);
    connect(d->m_modeStack, &Internal::FancyTabWidget::currentChanged,
            this, &ModeManager::currentTabChanged);
    connect(d->m_modeStack, &Internal::FancyTabWidget::menuTriggered,
            this, [](int index, QMouseEvent *e) { d->showMenu(index, e); });
}

ModeManager::~ModeManager()
{
    delete d;
    d = nullptr;
    m_instance = nullptr;
}

/*!
    Returns the id of the current mode.

    \sa activateMode()
    \sa currentMode()
*/
Id ModeManager::currentModeId()
{
    int currentIndex = d->m_modeStack->currentIndex();
    if (currentIndex < 0)
        return Id();
    return d->m_modes.at(currentIndex)->id();
}

static IMode *findMode(Id id)
{
    const int index = indexOf(id);
    if (index >= 0)
        return d->m_modes.at(index);
    return nullptr;
}

/*!
    Makes the mode with ID \a id the current mode.

    \sa currentMode()
    \sa currentModeId()
    \sa currentModeAboutToChange()
    \sa currentModeChanged()
*/
void ModeManager::activateMode(Id id)
{
    d->activateModeHelper(id);
}

void ModeManagerPrivate::activateModeHelper(Id id)
{
    if (m_startingUp) {
        m_pendingFirstActiveMode = id;
    } else {
        const int currentIndex = m_modeStack->currentIndex();
        const int newIndex = id.isValid() ? indexOf(id) : -1;
        if (newIndex != currentIndex) {
            if (newIndex >= 0) {
                m_modes.at(newIndex)->setVisible(true);
                m_modeStack->setCurrentIndex(newIndex);
            } else {
                m_modeStack->setCurrentIndex(-1);
            }
        }
    }
}

void ModeManagerPrivate::registerModeSelectorStyleActions()
{
    ActionContainer *mview = ActionManager::actionContainer(Constants::M_VIEW);

    // Cycle Mode Selector Styles
    ActionBuilder(m_instance, Constants::CYCLE_MODE_SELECTOR_STYLE)
        .setText(Tr::tr("Cycle Mode Selector Styles"))
        .addOnTriggered([] { ModeManager::cycleModeStyle(); });

    // Mode Selector Styles
    ActionContainer *mmodeLayouts = ActionManager::createMenu(Constants::M_VIEW_MODESTYLES);
    mview->addMenu(mmodeLayouts, Constants::G_VIEW_MODES);
    QMenu *styleMenu = mmodeLayouts->menu();
    styleMenu->setTitle(Tr::tr("Modes"));
    auto *stylesGroup = new QActionGroup(styleMenu);
    stylesGroup->setExclusive(true);

    mmodeLayouts->addSeparator(Constants::G_DEFAULT_THREE);

    ActionBuilder(m_instance, "QtCreator.Modes.IconsAndText")
        .setText(Tr::tr("Icons and Text"))
        .setCheckable(true)
        .addOnTriggered([] { ModeManager::setModeStyle(ModeManager::Style::IconsAndText); })
        .addToContainer(Constants::M_VIEW_MODESTYLES, Constants::G_DEFAULT_THREE)
        .bindContextAction(&m_setModeSelectorStyleIconsAndTextAction);
    stylesGroup->addAction(m_setModeSelectorStyleIconsAndTextAction);

    ActionBuilder(m_instance, "QtCreator.Modes.IconsOnly")
        .setText(Tr::tr("Icons Only"))
        .setCheckable(true)
        .addOnTriggered([] { ModeManager::setModeStyle(ModeManager::Style::IconsOnly); })
        .addToContainer(Constants::M_VIEW_MODESTYLES, Constants::G_DEFAULT_THREE)
        .bindContextAction(&m_setModeSelectorStyleIconsOnlyAction);
    stylesGroup->addAction(m_setModeSelectorStyleIconsOnlyAction);

    ActionBuilder(m_instance, "QtCreator.Modes.Hidden")
        .setText(Tr::tr("Hidden"))
        .setCheckable(true)
        .addOnTriggered([] { ModeManager::setModeStyle(ModeManager::Style::Hidden); })
        .addToContainer(Constants::M_VIEW_MODESTYLES, Constants::G_DEFAULT_THREE)
        .bindContextAction(&m_setModeSelectorStyleHiddenAction);
    stylesGroup->addAction(m_setModeSelectorStyleHiddenAction);
    updateModeSelectorStyleMenu();
}

void ModeManagerPrivate::updateModeSelectorStyleMenu()
{
    if (!m_setModeSelectorStyleHiddenAction) // actions not yet created
        return;
    switch (m_modeStyle) {
    case ModeManager::Style::IconsAndText:
        m_setModeSelectorStyleIconsAndTextAction->setChecked(true);
        break;
    case ModeManager::Style::IconsOnly:
        m_setModeSelectorStyleIconsOnlyAction->setChecked(true);
        break;
    case ModeManager::Style::Hidden:
        m_setModeSelectorStyleHiddenAction->setChecked(true);
        break;
    }
}

void ModeManager::extensionsInitialized()
{
    d->extensionsInitializedHelper();
}

void ModeManagerPrivate::extensionsInitializedHelper()
{
    m_startingUp = false;
    registerModeSelectorStyleActions();
    Utils::sort(m_modes, &IMode::priority);
    std::reverse(m_modes.begin(), m_modes.end());

    for (IMode *mode : std::as_const(m_modes))
        appendMode(mode);

    if (m_pendingFirstActiveMode.isValid())
        activateModeHelper(m_pendingFirstActiveMode);
}

void ModeManager::addMode(IMode *mode)
{
    QTC_ASSERT(d->m_startingUp, return);
    d->m_modes.append(mode);
}

void ModeManagerPrivate::appendMode(IMode *mode)
{
    const int index = m_modeCommands.count();

    m_modeStack->insertTab(index, mode->widget(), mode->icon(), mode->displayName(),
                           mode->menu() != nullptr);
    m_modeStack->setTabEnabled(index, mode->isEnabled());
    m_modeStack->setTabVisible(index, mode->isVisible());

    // Register mode shortcut
    const Id actionId = mode->id().withPrefix("QtCreator.Mode.");
    QAction *action = new QAction(Tr::tr("Switch to <b>%1</b> mode").arg(mode->displayName()), m_instance);
    Command *cmd = ActionManager::registerAction(action, actionId);
    cmd->setDefaultKeySequence(QKeySequence(useMacShortcuts ? QString("Meta+%1").arg(index + 1)
                                                            : QString("Ctrl+%1").arg(index + 1)));
    m_modeCommands.append(cmd);

    m_modeStack->setTabToolTip(index, cmd->action()->toolTip());
    QObject::connect(cmd, &Command::keySequenceChanged, m_instance, [cmd, index, this] {
        m_modeStack->setTabToolTip(index, cmd->action()->toolTip());
    });

    Id id = mode->id();
    QObject::connect(action, &QAction::triggered, m_instance, [this, id] {
        ModeManager::activateMode(id);
        ICore::raiseWindow(m_modeStack);
    });

    QObject::connect(mode, &IMode::enabledStateChanged,
                     m_instance, [this, mode] { enabledStateChanged(mode); });

    // view action
    QAction *toggleAction;
    ActionBuilder(m_instance, mode->id().withPrefix("QtCreator.Modes.View."))
        //: %1 = name of a mode
        .setText(Tr::tr("Show %1").arg(mode->displayName()))
        .setCheckable(true)
        .setChecked(mode->isVisible())
        .addToContainer(Constants::M_VIEW_MODESTYLES)
        .bindContextAction(&toggleAction)
        .addOnTriggered(mode, [mode](bool checked) { mode->setVisible(checked); });
    QObject::connect(
        mode, &IMode::visibleChanged, m_instance, [this, mode, toggleAction](bool visible) {
            toggleAction->setChecked(visible);
            visibleChanged(mode);
        });
}

void ModeManager::removeMode(IMode *mode)
{
    const int index = d->m_modes.indexOf(mode);
    if (index >= d->m_modes.size() - 1 && d->m_modes.size() > 1)
        d->m_modeStack->setCurrentIndex(d->m_modes.size() - 2);
    d->m_modes.remove(index);
    if (d->m_startingUp)
        return;

    d->m_modeCommands.remove(index);
    d->m_modeStack->removeTab(index);
}

void ModeManagerPrivate::ensureVisibleEnabledMode()
{
    // Make sure we leave any disabled mode to prevent possible crashes:
    IMode *mode = ModeManager::currentMode();
    if (!mode || !mode->isEnabled() || !mode->isVisible()) {
        // This assumes that there is always at least one enabled mode.
        for (int i = 0; i < d->m_modes.count(); ++i) {
            IMode *other = d->m_modes.at(i);
            if (other->isEnabled() && other->isVisible()) {
                ModeManager::activateMode(other->id());
                return;
            }
        }
        ModeManager::activateMode({});
    }
}

void ModeManagerPrivate::enabledStateChanged(IMode *mode)
{
    int index = d->m_modes.indexOf(mode);
    QTC_ASSERT(index >= 0, return);
    d->m_modeStack->setTabEnabled(index, mode->isEnabled());

    ensureVisibleEnabledMode();
}

void ModeManagerPrivate::visibleChanged(IMode *mode)
{
    int index = d->m_modes.indexOf(mode);
    QTC_ASSERT(index >= 0, return);
    d->m_modeStack->setTabVisible(index, mode->isVisible());

    ensureVisibleEnabledMode();
}

/*!
    Adds the \a action to the mode selector's tool bar.
    Actions are sorted by \a priority in descending order.
    Use this functionality very sparingly.
*/
void ModeManager::addAction(QAction *action, int priority)
{
    d->m_actions.insert(action, priority);

    // Count the number of commands with a higher priority
    int index = 0;
    for (int p : std::as_const(d->m_actions)) {
        if (p > priority)
            ++index;
    }

    d->m_actionBar->insertAction(index, action);
}

/*!
    \internal
*/
void ModeManager::addProjectSelector(QAction *action)
{
    d->m_actionBar->addProjectSelector(action);
    d->m_actions.insert(0, INT_MAX);
}

void ModeManager::currentTabAboutToChange(int index)
{
    IMode *mode = d->m_modes.value(index, nullptr);
    emit currentModeAboutToChange(mode ? mode->id() : Id());
}

void ModeManager::currentTabChanged(int index)
{
    // Tab index changes to -1 when there is no tab left.
    if (index < 0)
        return;

    IMode *mode = d->m_modes.at(index);
    if (!mode)
        return;

    // Set the mode's context regardless of focus widget.
    // Whenever a mode is active, it's Context is active.
    ICore::updateAdditionalContexts(d->m_addedContexts, mode->context());
    d->m_addedContexts = mode->context();

    IMode *oldMode = nullptr;
    if (d->m_oldCurrent >= 0)
        oldMode = d->m_modes.at(d->m_oldCurrent);
    d->m_oldCurrent = index;
    emit currentModeChanged(mode->id(), oldMode ? oldMode->id() : Id());
    emit currentMainWindowChanged();
}

/*!
    \internal
*/
void ModeManager::setFocusToCurrentMode()
{
    IMode *mode = findMode(currentModeId());
    QTC_ASSERT(mode, return);
    QWidget *widget = mode->widget();
    if (widget) {
        QWidget *focusWidget = widget->focusWidget();
        if (!focusWidget)
            focusWidget = widget;
        focusWidget->setFocus();
    }
}

/*!
    \internal
*/
void ModeManager::setModeStyle(ModeManager::Style style)
{
    const bool visible = style != Style::Hidden;
    const bool iconsOnly = style == Style::IconsOnly;

    d->m_modeStyle = style;
    d->m_actionBar->setIconsOnly(iconsOnly);
    d->m_modeStack->setIconsOnly(iconsOnly);
    d->m_modeStack->setSelectionWidgetVisible(visible);

    d->updateModeSelectorStyleMenu();
}

/*!
    \internal
*/
void ModeManager::cycleModeStyle()
{
    auto nextStyle = Style((int(modeStyle()) + 1) % 3);
    setModeStyle(nextStyle);
}

/*!
    \internal
*/
ModeManager::Style ModeManager::modeStyle()
{
    return d->m_modeStyle;
}

/*!
    Returns the pointer to the instance. Only use for connecting to signals.
*/
ModeManager *ModeManager::instance()
{
    return m_instance;
}

/*!
    Returns a pointer to the current mode.

    \sa activateMode()
    \sa currentModeId()
*/
IMode *ModeManager::currentMode()
{
    const int currentIndex = d->m_modeStack->currentIndex();
    return currentIndex < 0 ? nullptr : d->m_modes.at(currentIndex);
}

} // namespace Core
