// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "imode.h"

#include "modemanager.h"

#include <utils/aspects.h>
#include <utils/fancymainwindow.h>
#include <utils/qtcassert.h>

#include <aggregation/aggregate.h>

using namespace Utils;

namespace Core {

namespace Internal {

class IModePrivate
{
public:
    QString m_displayName;
    QIcon m_icon;
    std::function<void(QMenu *)> m_menuFunction;
    Utils::FancyMainWindow *m_mainWindow = nullptr;
    int m_priority = -1;
    Utils::Id m_id;
    Context m_context;
    QPointer<QWidget> m_widget;
    std::function<QWidget *()> m_widgetCreator;
    bool m_isEnabled = true;
    BoolAspect m_isVisible;
};

} // namespace Internal

/*!
    \class Core::IMode
    \inheaderfile coreplugin/imode.h
    \ingroup mainclasses
    \inmodule QtCreator

    \brief The IMode class represents a mode in \QC.

    This class defines a mode and its representation as a tool button in the
    mode selector on the left side of \QC's main window.

    Modes are used to show a different UI for different development tasks.
    Therefore modes control the layout of most of Qt Creator's main window,
    except for the tool bar on the left side and the status bar. For example
    Edit mode, the most commonly used mode for coding, shows the code editor
    and various navigation and output panes. Debug mode enhances that view with
    a configurable layout of debugging related information. Design mode
    reserves all the main window's space for the graphical editor.

    When a mode gets selected, a context is added and a widget is shown.
    Use \l{IMode::setContext()} to set the context, and either
    \l{IMode::setWidget()} to set the widget directly or
    \l{IMode::setWidgetCreator()} to define a functor that is used when the
    widget is needed.

    Adding a mode should be done sparingly, only as a last reserve. Consider if
    your feature can instead be implemented as a INavigationWidgetFactory,
    IOutputPane, \c{Debugger::Utils::Perspective}, separate dialog, or
    specialized IEditor first.

    If you add a mode, consider adding a NavigationWidgetPlaceHolder
    on the left side and a OutputPanePlaceHolder on the bottom of your
    mode's layout.

    Modes automatically register themselves with \QC when they are created and
    unregister themselves when they are destructed.
*/

/*!
    \property Core::IMode::enabled

    This property holds whether the mode is enabled.

    By default, this property is \c true.
*/

/*!
    \property Core::IMode::displayName

    This property holds the display name of the mode.

    The display name is shown under the mode icon in the mode selector.
*/

/*!
    \property Core::IMode::icon

    This property holds the icon of the mode.

    The icon is shown for the mode in the mode selector. Mode icons should
    support the sizes 34x34 pixels and 68x68 pixels for HiDPI.
*/

/*!
    \property Core::IMode::priority

    This property holds the priority of the mode.

    The priority defines the order in which the modes are shown in the mode
    selector. Higher priority moves to mode towards the top. Welcome mode,
    which should stay at the top, has the priority 100. The default priority is
    -1.
*/

/*!
    \property Core::IMode::id

    This property holds the ID of the mode.
*/

/*!
    Creates an IMode with an optional \a parent.

    Registers the mode in \QC.
*/
IMode::IMode(QObject *parent)
    : QObject(parent)
    , m_d(new Internal::IModePrivate)
{
    m_d->m_isVisible.setDefaultValue(true);
    connect(&m_d->m_isVisible, &BoolAspect::changed, this, [this] {
        emit visibleChanged(m_d->m_isVisible.value());
        m_d->m_isVisible.writeSettings();
    });
    ModeManager::addMode(this);
}

IMode::~IMode()
{
    if (m_d->m_widgetCreator)
        delete m_d->m_widget;
}

QString IMode::displayName() const
{
    return m_d->m_displayName;
}

QIcon IMode::icon() const
{
    return m_d->m_icon;
}

int IMode::priority() const
{
    return m_d->m_priority;
}

Utils::Id IMode::id() const
{
    return m_d->m_id;
}

void IMode::setEnabled(bool enabled)
{
    if (m_d->m_isEnabled == enabled)
        return;
    m_d->m_isEnabled = enabled;
    emit enabledStateChanged(m_d->m_isEnabled);
}

void IMode::setVisible(bool visible)
{
    m_d->m_isVisible.setValue(visible);
}

void IMode::setDisplayName(const QString &displayName)
{
    m_d->m_displayName = displayName;
}

void IMode::setIcon(const QIcon &icon)
{
    m_d->m_icon = icon;
}

void IMode::setPriority(int priority)
{
    m_d->m_priority = priority;
}

void IMode::setId(Utils::Id id)
{
    m_d->m_id = id;
    m_d->m_isVisible
        .setSettingsKey("MainWindow", id.withPrefix("Mode.").withSuffix(".Visible").toKey());
    m_d->m_isVisible.readSettings();
}

/*!
    Sets a \a menuFunction that is used to add the mode specific items
    to the mode's context menu. This is called every time the context
    menu is requested with a new QMenu instance.
    The menu is destroyed after the it closes.
*/
void IMode::setMenu(std::function<void(QMenu *)> menuFunction)
{
    m_d->m_menuFunction = menuFunction;
}

/*!
    Sets the \a context associated with this IMode.

    \sa context()
*/
void IMode::setContext(const Context &context)
{
    m_d->m_context = context;
}

/*!
    Sets the \a widget associated with this IMode.

    \note The ownership of the passed widget is not changed.

    \sa widget(), setWidgetCreator()
*/
void IMode::setWidget(QWidget *widget)
{
    QTC_ASSERT(!m_d->m_widgetCreator,
               qWarning("A mode widget should not be set if there is already a widget creator"));
    m_d->m_widget = widget;
}

/*!
    Sets the \a widgetCreator to create the widget associated with this IMode
    when needed.

    \note The mode itself takes the ownership of the created widget.

    \sa widget(), setWidget()
*/
void IMode::setWidgetCreator(const std::function<QWidget *()> &widgetCreator)
{
    QTC_ASSERT(!m_d->m_widget,
               qWarning("A mode widget widgetCreator should not be set if there is already a widget"));
    m_d->m_widgetCreator = widgetCreator;
}

Utils::FancyMainWindow *IMode::mainWindow()
{
    if (m_d->m_mainWindow)
        return m_d->m_mainWindow;
    return qobject_cast<Utils::FancyMainWindow *>(widget());
}

void IMode::setMainWindow(Utils::FancyMainWindow *mw)
{
    m_d->m_mainWindow = mw;
    emit ModeManager::instance()->currentMainWindowChanged();
}

bool IMode::isEnabled() const
{
    return m_d->m_isEnabled;
}

bool IMode::isVisible() const
{
    return m_d->m_isVisible.value();
}

/*!
    Returns if the mode provides mode specific context menu items.

    \sa setMenu()
*/
bool IMode::hasMenu() const
{
    return bool(m_d->m_menuFunction);
}

/*!
    Adds the mode specific items to the \a menu, if any.

    \sa setMenu()
*/
void IMode::addToMenu(QMenu *menu) const
{
    if (m_d->m_menuFunction)
        m_d->m_menuFunction(menu);
}

/*!
    Returns the context list associated with this IMode.

    \sa setContext()
*/
Context IMode::context() const
{
    return m_d->m_context;
}

/*!
    Returns the widget associated with this IMode.

    \sa setWidget()
*/
QWidget *IMode::widget() const
{
    if (!m_d->m_widget && m_d->m_widgetCreator)
        m_d->m_widget = m_d->m_widgetCreator();
    return m_d->m_widget;
}

} // namespace Core
