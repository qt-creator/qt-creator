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

#include <QAbstractItemView>
#include <QCheckBox>
#include <QEvent>
#include <QGroupBox>
#include <QHash>
#include <QMenu>
#include <QPointer>
#include <QPushButton>
#include <QScrollBar>
#include <QSpinBox>

#include <utility>

using namespace Utils;
using namespace Core::Internal;

static QHash<Id, std::pair<QString, FilePath>> g_categories;

namespace Core {
namespace Internal {

using WidgetCreator = std::function<IOptionsPageWidget *()>;
using SettingsProvider = std::function<Utils::AspectContainer *()>;

class IOptionsPageWidgetPrivate final : public QObject
{
public:
    explicit IOptionsPageWidgetPrivate(IOptionsPageWidget *q) : q(q) {}

    void setAspects(Utils::AspectContainer *aspects);

    bool eventFilter(QObject *watched, QEvent *event);

    IOptionsPageWidget *q;

    std::function<void()> m_onApply;
    std::function<void()> m_onCancel;
    std::function<void()> m_onFinish;

    AspectContainer *m_aspects = nullptr;
    bool m_useDirtyHook = true;
};

class IOptionsPagePrivate
{
public:
    QStringList keywords();
    IOptionsPageWidget *createWidget();

    Id m_id;
    Id m_category;
    QString m_displayName;
    WidgetCreator m_widgetCreator;
    bool m_keywordsInitialized = false;
    QStringList m_keywords;
    std::function<AspectContainer *()> m_settingsProvider;
    bool m_recreateOnCancel = false;
    bool m_autoApply = false;
    QPointer<IOptionsPageWidget> m_widget; // Cache.
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
    : d(new Internal::IOptionsPageWidgetPrivate(this))
{}

IOptionsPageWidget::~IOptionsPageWidget()
{
    d.reset();
}

bool IOptionsPageWidget::useDirtyHook() const
{
    return d->m_useDirtyHook;
}

void IOptionsPageWidget::setUseDirtyHook(bool on)
{
    d->m_useDirtyHook = on;
}

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
    Calls the apply function, if set.
    \sa setOnApply
*/
void IOptionsPageWidget::apply()
{
    if (d->m_onApply) {
        d->m_onApply();
        return;
    }

    if (d->m_aspects) {
        AspectContainer *container = d->m_aspects;
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
        return;
    }
}

void IOptionsPageWidget::cancel()
{
    if (d->m_onCancel) {
        d->m_onCancel();
        return;
    }

    if (d->m_aspects) {
        AspectContainer *container = d->m_aspects;
        if (container->isDirty())
            container->cancel();
        return;
    }
}

bool IOptionsPageWidget::isDirty() const
{
    if (d->m_aspects)
        return d->m_aspects->isDirty();

    return true;
}

const char DirtyOnMouseButtonRelease[] = "DirtyOnMouseButtonRelease";

static bool makesDirty(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonRelease
            && !isIgnoredForDirtyHook(watched)
            && watched->property(DirtyOnMouseButtonRelease).isValid()) {
        return true;
    }

    return false;
}

void IOptionsPageWidget::setupDirtyHook(QWidget *widget)
{
    QTC_ASSERT(!d->m_aspects, return);
    QTC_ASSERT(widget, return);

    if (isIgnoredForDirtyHook(widget))
        return;

    QList<QWidget *> children = {widget};

    while (!children.isEmpty()) {
        QWidget *child = children.takeLast();
        if (isIgnoredForDirtyHook(child))
            continue;

        children += child->findChildren<QWidget *>(Qt::FindDirectChildrenOnly);

        if (child->metaObject() == &QWidget::staticMetaObject)
            continue;

        if (child->metaObject() == &QLabel::staticMetaObject)
            continue;

        if (child->metaObject() == &QScrollBar::staticMetaObject)
            continue;

        if (child->metaObject() == &QMenu::staticMetaObject)
            continue;

        auto markDirty = [child] {
            if (isIgnoredForDirtyHook(child))
                return;
            markSettingsDirty();
        };

        if (auto ob = qobject_cast<QLineEdit *>(child)) {
            connect(ob, &QLineEdit::textEdited, this, markDirty);
            continue;
        }
        if (auto ob = qobject_cast<QComboBox *>(child)) {
            connect(ob, &QComboBox::currentIndexChanged, markDirty);
            continue;
        }
        if (auto ob = qobject_cast<QSpinBox *>(child)) {
            connect(ob, &QSpinBox::valueChanged, this, markDirty);
            continue;
        }
        if (auto ob = qobject_cast<QGroupBox *>(child)) {
            connect(ob, &QGroupBox::toggled, this, markDirty);
            continue;
        }
        if (auto ob = qobject_cast<QCheckBox *>(child)) {
            connect(ob, &QCheckBox::toggled, this, markDirty);
            continue;
        }
    }

    if (auto ob = qobject_cast<QAbstractItemView *>(widget)) {
        ob->viewport()->setProperty(DirtyOnMouseButtonRelease, true);
        ob->viewport()->installEventFilter(d.get());
    }
}

void IOptionsPageWidgetPrivate::setAspects(AspectContainer *aspects)
{
    m_aspects = aspects;
    connect(m_aspects, &AspectContainer::volatileValueChanged, [] {
        checkSettingsDirty();
    });
}

bool IOptionsPageWidgetPrivate::eventFilter(QObject *watched, QEvent *event)
{
    if (makesDirty(watched, event))
        markSettingsDirty();

    return false;
}

/*!
    Returns the path to the category icon of the options page. This icon will be read from this
    path and displayed in the list on the left side of the \uicontrol Options dialog.
*/
FilePath IOptionsPage::categoryIconPath() const
{
    return g_categories.value(category()).second;
}

std::optional<AspectContainer *> IOptionsPage::aspects() const
{
    return d->m_settingsProvider ? std::make_optional(d->m_settingsProvider()) : std::nullopt;
}

/*!
    Sets the \a widgetCreator callback to create page widgets on demand. The
    widget will be owned by the settings mode.
 */
void IOptionsPage::setWidgetCreator(const WidgetCreator &widgetCreator)
{
    d->m_widgetCreator = widgetCreator;
    d->m_recreateOnCancel = true;
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

void IOptionsPage::setSettingsProvider(const std::function<AspectContainer *()> &provider)
{
    d->m_settingsProvider = provider;
    d->m_recreateOnCancel = false;
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
    Registers a category with the ID \a id, user-visible name \a displayName, and
    icon specified by \a iconPath.
 */
void IOptionsPage::registerCategory(
    Utils::Id id, const QString &displayName, const Utils::FilePath &iconPath)
{
    g_categories.insert(id, std::make_pair(displayName, iconPath));
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
    return g_categories.value(category()).first;
}

bool IOptionsPage::recreateOnCancel() const
{
    return d->m_recreateOnCancel;
}

void IOptionsPage::setAutoApply()
{
    d->m_autoApply = true;
}

/*!
    Is used by the \uicontrol Options dialog search filter to match \a regexp to this options
    page. If not set,  it takes the widget and then looks for all child labels, check boxes, push
    buttons, and group boxes. Should return \c true when a match is found.
*/

static QStringList scrapeKeywords(QWidget *widget)
{
    QStringList keywords;

    QTC_ASSERT(widget, return keywords);

    // find common subwidgets
    for (const QLabel *label : widget->findChildren<QLabel *>())
        keywords << label->text();
    for (const QCheckBox *checkbox : widget->findChildren<QCheckBox *>())
        keywords << checkbox->text();
    for (const QPushButton *pushButton : widget->findChildren<QPushButton *>())
        keywords << pushButton->text();
    for (const QGroupBox *groupBox : widget->findChildren<QGroupBox *>())
        keywords << groupBox->title();

    return transform(keywords, stripAccelerator);
}

QStringList Internal::IOptionsPagePrivate::keywords()
{
    if (m_keywordsInitialized)
        return m_keywords;

    m_keywordsInitialized = true;

    createWidget();
    QTC_ASSERT(m_widget, return {});

    m_keywords = scrapeKeywords(m_widget);
    return m_keywords;
}

void IOptionsPage::setFixedKeywords(const QStringList &keywords)
{
    d->m_keywords = keywords;
    d->m_keywordsInitialized = true;
}

void IOptionsPage::setRecreateOnCancel(bool on)
{
    d->m_recreateOnCancel = on;
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

IOptionsPageWidget *IOptionsPage::createWidget()
{
    return d->createWidget();
}

IOptionsPageWidget *IOptionsPagePrivate::createWidget()
{
    if (m_widget)
        return m_widget;

    if (m_widgetCreator) {
        m_widget = m_widgetCreator();
        QTC_ASSERT(m_widget, return nullptr);
        return m_widget;
    }

    if (m_settingsProvider) {
        AspectContainer *container = m_settingsProvider();
        QTC_ASSERT(container, return nullptr);
        std::function<Layouting::Layout()> layouter = container->layouter();
        QTC_ASSERT(layouter, return nullptr);
        m_widget = new IOptionsPageWidget;
        m_widget->d->setAspects(container);
        layouter().attachTo(m_widget);
        return m_widget;
    }

    QTC_CHECK(false);
    return nullptr;
}

bool IOptionsPage::matches(const QRegularExpression &regexp) const
{
    const QStringList keywords = d->keywords();

    for (const QString &keyword : keywords) {
        if (keyword.contains(regexp))
            return true;
    }

    return false;
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
