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
#include <QHash>
#include <QLabel>
#include <QPointer>
#include <QPushButton>
#include <QRegularExpression>

using namespace Utils;

namespace Core {

namespace Internal {

class IOptionsPageWidgetPrivate
{
public:
    std::function<void()> m_onApply;
    std::function<void()> m_onCancel;
    std::function<void()> m_onFinish;
};

class IOptionsPagePrivate
{
public:
    Id m_id;
    Id m_category;
    QString m_displayName;
    QString m_displayCategory;
    FilePath m_categoryIconPath;
    IOptionsPage::WidgetCreator m_widgetCreator;
    QPointer<QWidget> m_widget; // Used in conjunction with m_widgetCreator

    bool m_keywordsInitialized = false;
    QStringList m_keywords;

    std::function<AspectContainer *()> m_settingsProvider;
};

class IOptionsPageProviderPrivate
{
public:
    Id m_category;
    QString m_displayCategory;
    FilePath m_categoryIconPath;
};

} // namespace Internal

/*!
    \class Core::IOptionsPageProvider
    \inmodule QtCreator
    \internal
*/

/*!
    \class Core::IOptionsPageWidget
    \inheaderfile coreplugin/dialogs/ioptionspage.h
    \ingroup mainclasses
    \inmodule QtCreator

    \brief The IOptionsPageWidget class is used to standardize the interaction
    between an IOptionsPage and its widget.

    Use setOnApply() and setOnFinish() to set functions that are called when
    the IOptionsPage is applied and finished respectively.
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

IOptionsPageWidget::IOptionsPageWidget()
    : d(new Internal::IOptionsPageWidgetPrivate)
{}

IOptionsPageWidget::~IOptionsPageWidget() = default;

/*!
    Sets the function that is called by default on apply to \a func.
*/
void IOptionsPageWidget::setOnApply(const std::function<void()> &func)
{
    d->m_onApply = func;
}

void IOptionsPageWidget::setOnCancel(const std::function<void()> &func)
{
    d->m_onCancel = func;
}

/*!
    Sets the function that is called by default on finish to \a func.
*/
void IOptionsPageWidget::setOnFinish(const std::function<void()> &func)
{
    d->m_onFinish = func;
}

/*!
    Calls the apply function, if set.
    \sa setOnApply
*/
void IOptionsPageWidget::apply()
{
    if (d->m_onApply)
        d->m_onApply();
}

void IOptionsPageWidget::cancel()
{
    if (d->m_onCancel)
        d->m_onCancel();
}

/*!
    Calls the finish function, if set.
    \sa setOnFinish
*/
void IOptionsPageWidget::finish()
{
    if (d->m_onFinish)
        d->m_onFinish();
}

/*!
    Returns the path to the category icon of the options page. This icon will be read from this
    path and displayed in the list on the left side of the \uicontrol Options dialog.
*/
FilePath IOptionsPage::categoryIconPath() const
{
    return d->m_categoryIconPath;
}

/*!
    Sets the \a widgetCreator callback to create page widgets on demand. The
    widget will be destroyed on finish().
 */
void IOptionsPage::setWidgetCreator(const WidgetCreator &widgetCreator)
{
    d->m_widgetCreator = widgetCreator;
}

/*!
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
        d->m_keywords << label->text();
    for (const QCheckBox *checkbox : widget->findChildren<QCheckBox *>())
        d->m_keywords << checkbox->text();
    for (const QPushButton *pushButton : widget->findChildren<QPushButton *>())
        d->m_keywords << pushButton->text();
    for (const QGroupBox *groupBox : widget->findChildren<QGroupBox *>())
        d->m_keywords << groupBox->title();

    return d->m_keywords;
}

/*!
    Sets the \a id of the options page.
*/
void IOptionsPage::setId(Id id)
{
    d->m_id = id;
}

/*!
    Sets \a displayName as the display name of the options page.
*/
void IOptionsPage::setDisplayName(const QString &displayName)
{
    d->m_displayName = displayName;
}

void IOptionsPage::setCategory(Id category)
{
    d->m_category = category;
}

void IOptionsPage::setDisplayCategory(const QString &displayCategory)
{
    d->m_displayCategory = displayCategory;
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
    if (!d->m_widget) {
        if (d->m_widgetCreator) {
            d->m_widget = d->m_widgetCreator();
            QTC_CHECK(d->m_widget);
        } else if (d->m_settingsProvider) {
            d->m_widget = new IOptionsPageWidget;
            AspectContainer *container = d->m_settingsProvider();
            if (auto layouter = container->layouter()) {
                layouter().attachTo(d->m_widget);
            } else {
                QTC_CHECK(false);
            }
        } else {
            QTC_CHECK(false);
        }
    }
    return d->m_widget;
}

/*!
    Called when selecting the \uicontrol Apply button on the options page dialog.
    Should detect whether any changes were made and store those.

    Either override this function in a derived class, or set a widget creator.

    \sa setWidgetCreator()
*/

void IOptionsPage::apply()
{
    if (auto widget = qobject_cast<IOptionsPageWidget *>(d->m_widget))
        widget->apply();

    if (d->m_settingsProvider) {
        AspectContainer *container = d->m_settingsProvider();
        QTC_ASSERT(container, return);
        // Sanity check: Aspects in option pages should not autoapply.
        if (!container->aspects().isEmpty()) {
            BaseAspect *aspect = container->aspects().first();
            QTC_ASSERT(aspect, return);
            QTC_ASSERT(!aspect->isAutoApply(), return);
        }
        if (container->isDirty()) {
            container->apply();
            container->writeSettings();
         }
    }
}

void IOptionsPage::cancel()
{
    if (auto widget = qobject_cast<IOptionsPageWidget *>(d->m_widget))
         widget->cancel();

    if (d->m_settingsProvider) {
         AspectContainer *container = d->m_settingsProvider();
         QTC_ASSERT(container, return);
         if (container->isDirty())
            container->cancel();
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
    if (auto widget = qobject_cast<IOptionsPageWidget *>(d->m_widget))
        widget->finish();

    if (d->m_settingsProvider) {
        AspectContainer *container = d->m_settingsProvider();
        container->finish();
    }

    delete d->m_widget;
}

/*!
    Sets \a categoryIconPath as the path to the category icon of the options
    page.
*/
void IOptionsPage::setCategoryIconPath(const FilePath &categoryIconPath)
{
    d->m_categoryIconPath = categoryIconPath;
}

void IOptionsPage::setSettingsProvider(const std::function<AspectContainer *()> &provider)
{
    d->m_settingsProvider = provider;
}

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
    : d(new Internal::IOptionsPagePrivate)
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
    Returns a unique identifier for referencing the options page.
*/
Id IOptionsPage::id() const
{
    return d->m_id;
}

/*!
    Returns the translated display name of the options page.
*/
QString IOptionsPage::displayName() const
{
    return d->m_displayName;
}

/*!
    Returns the unique id for the category that the options page should be displayed in. This id is
    used for sorting the list on the left side of the \uicontrol Options dialog.
*/
Id IOptionsPage::category() const
{
    return d->m_category;
}

/*!
    Returns the translated category name of the options page. This name is displayed in the list on
    the left side of the \uicontrol Options dialog.
*/
QString IOptionsPage::displayCategory() const
{
    return d->m_displayCategory;
}

/*!
    Is used by the \uicontrol Options dialog search filter to match \a regexp to this options
    page. This defaults to take the widget and then looks for all child labels, check boxes, push
    buttons, and group boxes. Should return \c true when a match is found.
*/
bool IOptionsPage::matches(const QRegularExpression &regexp) const
{
    if (!d->m_keywordsInitialized) {
        d->m_keywords = transform(keywords(), stripAccelerator);
        d->m_keywordsInitialized = true;
    }

    for (const QString &keyword : d->m_keywords)
        if (keyword.contains(regexp))
            return true;
    return false;
}

static QList<IOptionsPageProvider *> g_optionsPagesProviders;

IOptionsPageProvider::IOptionsPageProvider()
    : d(new Internal::IOptionsPageProviderPrivate)
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

Id IOptionsPageProvider::category() const
{
    return d->m_category;
}

QString IOptionsPageProvider::displayCategory() const
{
    return d->m_displayCategory;
}

FilePath IOptionsPageProvider::categoryIconPath() const
{
    return d->m_categoryIconPath;
}

/*!
    Uses \a category to sort the options pages.
*/
void IOptionsPageProvider::setCategory(Id category)
{
    d->m_category = category;
}

/*!
    Sets \a displayCategory as the display category of the options page.
*/
void IOptionsPageProvider::setDisplayCategory(const QString &displayCategory)
{
    d->m_displayCategory = displayCategory;
}

void IOptionsPageProvider::setCategoryIconPath(const FilePath &iconPath)
{
    d->m_categoryIconPath = iconPath;
}

static QHash<Id, Id> g_preselectedOptionPageItems;

void setPreselectedOptionsPageItem(Id page, Id item)
{
    g_preselectedOptionPageItems.insert(page, item);
}

Id preselectedOptionsPageItem(Id page)
{
    return g_preselectedOptionPageItems.value(page);
}

} // Core
