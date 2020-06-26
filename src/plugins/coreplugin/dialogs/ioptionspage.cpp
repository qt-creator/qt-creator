/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2016 Falko Arps
** Copyright (C) 2016 Sven Klein
** Copyright (C) 2016 Giuliano Schneider
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

#include "ioptionspage.h"

#include <utils/stringutils.h>
#include <utils/qtcassert.h>

#include <QCheckBox>
#include <QGroupBox>
#include <QIcon>
#include <QLabel>
#include <QPushButton>
#include <QRegularExpression>

using namespace Utils;

/*!
    \class Core::IOptionsPageProvider
    \inmodule QtCreator
    \internal
*/
/*!
    \class Core::IOptionsPageWidget
    \inmodule QtCreator
    \internal
*/

/*!
    \class Core::IOptionsPage
    \inheaderfile coreplugin/dialogs/ioptionspage.h
    \ingroup mainclasses
    \inmodule QtCreator

    \brief The IOptionsPage class is an interface for providing pages for the
    \uicontrol Options dialog (called \uicontrol Preferences on \macos).

    \image qtcreator-options-dialog.png
*/

/*!

    \fn Utils::Id Core::IOptionsPage::id() const

    Returns a unique identifier for referencing the options page.
*/

/*!
    \fn QString Core::IOptionsPage::displayName() const

    Returns the translated display name of the options page.
*/

/*!
    \fn Utils::Id Core::IOptionsPage::category() const

    Returns the unique id for the category that the options page should be displayed in. This id is
    used for sorting the list on the left side of the \uicontrol Options dialog.
*/

/*!
    \fn QString Core::IOptionsPage::displayCategory() const

    Returns the translated category name of the options page. This name is displayed in the list on
    the left side of the \uicontrol Options dialog.
*/

/*!
    Returns the category icon of the options page. This icon is displayed in the list on the left
    side of the \uicontrol Options dialog.
*/
QIcon Core::IOptionsPage::categoryIcon() const
{
    return m_categoryIcon.icon();
}

/*!
    Sets the \a widgetCreator callback to create page widgets on demand. The
    widget will be destroyed on finish().
 */
void Core::IOptionsPage::setWidgetCreator(const WidgetCreator &widgetCreator)
{
    m_widgetCreator = widgetCreator;
}

/*!
    Returns the widget to show in the \uicontrol Options dialog. You should create a widget lazily here,
    and delete it again in the finish() method. This method can be called multiple times, so you
    should only create a new widget if the old one was deleted.

    Alternatively, use setWidgetCreator() to set a callback function that is used to
    lazily create a widget in time.

    Either override this function in a derived class, or set a widget creator.
*/

QWidget *Core::IOptionsPage::widget()
{
    QTC_ASSERT(m_widgetCreator, return nullptr);
    if (!m_widget)
        m_widget = m_widgetCreator();
    return m_widget;
}

/*!
    Called when selecting the \uicontrol Apply button on the options page dialog.
    Should detect whether any changes were made and store those.

    Either override this function in a derived class, or set a widget creator.

    \sa setWidgetCreator()
*/

void Core::IOptionsPage::apply()
{
    QTC_ASSERT(m_widgetCreator, return);
    if (m_widget)
        m_widget->apply();
}

/*!
    Called directly before the \uicontrol Options dialog closes. Here you should
    delete the widget that was created in widget() to free resources.

    Either override this function in a derived class, or set a widget creator.

    \sa setWidgetCreator()
*/

void Core::IOptionsPage::finish()
{
    QTC_ASSERT(m_widgetCreator, return);
    if (m_widget) {
        m_widget->finish();
        delete m_widget;
    }
}

/*!
    Sets \a categoryIconPath as the path to the category icon of the options
    page.
*/
void Core::IOptionsPage::setCategoryIconPath(const QString &categoryIconPath)
{
    m_categoryIcon = Icon({{categoryIconPath, Theme::PanelTextColorDark}}, Icon::Tint);
}

/*!
    \fn void Core::IOptionsPage::setId(Utils::Id id)

    Sets the \a id of the options page.
*/

/*!
    \fn void Core::IOptionsPage::setDisplayName(const QString &displayName)

    Sets \a displayName as the display name of the options page.
*/

/*!
    \fn void Core::IOptionsPage::setCategory(Utils::Id category)

    Uses \a category to sort the options pages.
*/

/*!
    \fn void Core::IOptionsPage::setDisplayCategory(const QString &displayCategory)

    Sets \a displayCategory as the display category of the options page.
*/

/*!
    \fn void Core::IOptionsPage::setCategoryIcon(const Utils::Icon &categoryIcon)

    Sets \a categoryIcon as the category icon of the options page.
*/

static QList<Core::IOptionsPage *> g_optionsPages;

/*!
    Constructs an options page with the given \a parent and registers it
    at the global options page pool if \a registerGlobally is \c true.
*/
Core::IOptionsPage::IOptionsPage(QObject *parent, bool registerGlobally)
    : QObject(parent)
{
    if (registerGlobally)
        g_optionsPages.append(this);
}

/*!
    \internal
 */
Core::IOptionsPage::~IOptionsPage()
{
    g_optionsPages.removeOne(this);
}

/*!
    Returns a list of all options pages.
 */
const QList<Core::IOptionsPage *> Core::IOptionsPage::allOptionsPages()
{
    return g_optionsPages;
}

/*!
    Is used by the \uicontrol Options dialog search filter to match \a regexp to this options
    page. This defaults to take the widget and then looks for all child labels, check boxes, push
    buttons, and group boxes. Should return \c true when a match is found.
*/
bool Core::IOptionsPage::matches(const QRegularExpression &regexp) const
{
    if (!m_keywordsInitialized) {
        auto that = const_cast<IOptionsPage *>(this);
        QWidget *widget = that->widget();
        if (!widget)
            return false;
        // find common subwidgets
        foreach (const QLabel *label, widget->findChildren<QLabel *>())
            m_keywords << Utils::stripAccelerator(label->text());
        foreach (const QCheckBox *checkbox, widget->findChildren<QCheckBox *>())
            m_keywords << Utils::stripAccelerator(checkbox->text());
        foreach (const QPushButton *pushButton, widget->findChildren<QPushButton *>())
            m_keywords << Utils::stripAccelerator(pushButton->text());
        foreach (const QGroupBox *groupBox, widget->findChildren<QGroupBox *>())
            m_keywords << Utils::stripAccelerator(groupBox->title());

        m_keywordsInitialized = true;
    }
    foreach (const QString &keyword, m_keywords)
        if (keyword.contains(regexp))
            return true;
    return false;
}

static QList<Core::IOptionsPageProvider *> g_optionsPagesProviders;

Core::IOptionsPageProvider::IOptionsPageProvider(QObject *parent)
    : QObject(parent)
{
    g_optionsPagesProviders.append(this);
}

Core::IOptionsPageProvider::~IOptionsPageProvider()
{
    g_optionsPagesProviders.removeOne(this);
}

const QList<Core::IOptionsPageProvider *> Core::IOptionsPageProvider::allOptionsPagesProviders()
{
    return g_optionsPagesProviders;
}

QIcon Core::IOptionsPageProvider::categoryIcon() const
{
    return m_categoryIcon.icon();
}
