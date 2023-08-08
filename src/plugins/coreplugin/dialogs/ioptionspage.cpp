// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Falko Arps
// Copyright (C) 2016 Sven Klein
// Copyright (C) 2016 Giuliano Schneider
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "ioptionspage.h"

#include <utils/algorithm.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>

#include <QCheckBox>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QRegularExpression>

using namespace Utils;

namespace Core {

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
    Returns the path to the category icon of the options page. This icon will be read from this
    path and displayed in the list on the left side of the \uicontrol Options dialog.
*/
FilePath IOptionsPage::categoryIconPath() const
{
    return m_categoryIconPath;
}

/*!
    Sets the \a widgetCreator callback to create page widgets on demand. The
    widget will be destroyed on finish().
 */
void IOptionsPage::setWidgetCreator(const WidgetCreator &widgetCreator)
{
    m_widgetCreator = widgetCreator;
}

/*!
    \fn QStringList Core::IOptionsPage::keywords() const

    Returns a list of ui strings that are used inside the widget.
*/

QStringList IOptionsPage::keywords() const
{
    auto that = const_cast<IOptionsPage *>(this);
    QWidget *widget = that->widget();
    if (!widget)
        return {};
    // find common subwidgets
    for (const QLabel *label : widget->findChildren<QLabel *>())
        m_keywords << label->text();
    for (const QCheckBox *checkbox : widget->findChildren<QCheckBox *>())
        m_keywords << checkbox->text();
    for (const QPushButton *pushButton : widget->findChildren<QPushButton *>())
        m_keywords << pushButton->text();
    for (const QGroupBox *groupBox : widget->findChildren<QGroupBox *>())
        m_keywords << groupBox->title();

    return m_keywords;
}

/*!
    Returns the widget to show in the \uicontrol Options dialog. You should create a widget lazily here,
    and delete it again in the finish() method. This method can be called multiple times, so you
    should only create a new widget if the old one was deleted.

    Alternatively, use setWidgetCreator() to set a callback function that is used to
    lazily create a widget in time.

    Either override this function in a derived class, or set a widget creator.
*/

QWidget *IOptionsPage::widget()
{
    if (!m_widget) {
        if (m_widgetCreator) {
            m_widget = m_widgetCreator();
            QTC_CHECK(m_widget);
        } else if (m_settingsProvider) {
            m_widget = new IOptionsPageWidget;
            AspectContainer *container = m_settingsProvider();
            if (auto layouter = container->layouter()) {
                layouter().attachTo(m_widget);
            } else {
                QTC_CHECK(false);
            }
        } else {
            QTC_CHECK(false);
        }
    }
    return m_widget;
}

/*!
    Called when selecting the \uicontrol Apply button on the options page dialog.
    Should detect whether any changes were made and store those.

    Either override this function in a derived class, or set a widget creator.

    \sa setWidgetCreator()
*/

void IOptionsPage::apply()
{
    if (auto widget = qobject_cast<IOptionsPageWidget *>(m_widget))
        widget->apply();

    if (m_settingsProvider) {
        AspectContainer *container = m_settingsProvider();
        QTC_ASSERT(container, return);
        // Sanity check: Aspects in option pages should not autoapply.
        if (!container->aspects().isEmpty()) {
            BaseAspect *aspect = container->aspects().first();
            QTC_ASSERT(aspect, return);
            QTC_ASSERT(!aspect->isAutoApply(), container->setAutoApply(false));
        }
        if (container->isDirty()) {
            container->apply();
            container->writeSettings();
         }
    }
}

/*!
    Called directly before the \uicontrol Options dialog closes. Here you should
    delete the widget that was created in widget() to free resources.

    Either override this function in a derived class, or set a widget creator.

    \sa setWidgetCreator()
*/

void IOptionsPage::finish()
{
    if (auto widget = qobject_cast<IOptionsPageWidget *>(m_widget))
        widget->finish();

    if (m_settingsProvider) {
        AspectContainer *container = m_settingsProvider();
        container->finish();
    }

    delete m_widget;
}

/*!
    Sets \a categoryIconPath as the path to the category icon of the options
    page.
*/
void IOptionsPage::setCategoryIconPath(const FilePath &categoryIconPath)
{
    m_categoryIconPath = categoryIconPath;
}

void IOptionsPage::setSettingsProvider(const std::function<AspectContainer *()> &provider)
{
    m_settingsProvider = provider;
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

static QList<IOptionsPage *> &optionsPages()
{
    static QList<IOptionsPage *> thePages;
    return thePages;
}

/*!
    Constructs an options page and registers it
    at the global options page pool if \a registerGlobally is \c true.
*/
IOptionsPage::IOptionsPage(bool registerGlobally)
{
    if (registerGlobally)
        optionsPages().append(this);
}

/*!
    \internal
 */
IOptionsPage::~IOptionsPage()
{
    optionsPages().removeOne(this);
}

/*!
    Returns a list of all options pages.
 */
const QList<IOptionsPage *> IOptionsPage::allOptionsPages()
{
    return optionsPages();
}

/*!
    Is used by the \uicontrol Options dialog search filter to match \a regexp to this options
    page. This defaults to take the widget and then looks for all child labels, check boxes, push
    buttons, and group boxes. Should return \c true when a match is found.
*/
bool IOptionsPage::matches(const QRegularExpression &regexp) const
{
    if (!m_keywordsInitialized) {
        m_keywords = Utils::transform(keywords(), Utils::stripAccelerator);
        m_keywordsInitialized = true;
    }

    for (const QString &keyword : m_keywords)
        if (keyword.contains(regexp))
            return true;
    return false;
}

static QList<IOptionsPageProvider *> g_optionsPagesProviders;

IOptionsPageProvider::IOptionsPageProvider()
{
    g_optionsPagesProviders.append(this);
}

IOptionsPageProvider::~IOptionsPageProvider()
{
    g_optionsPagesProviders.removeOne(this);
}

const QList<IOptionsPageProvider *> IOptionsPageProvider::allOptionsPagesProviders()
{
    return g_optionsPagesProviders;
}

} // Core
