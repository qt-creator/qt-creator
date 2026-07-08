// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../luaengine.h"

#include "inheritance.h"
#include "utils.h"

#include <coreplugin/icore.h>

#include <solutions/spinner/spinner.h>

#include <utils/aspects.h>
#include <utils/completingtextedit.h>
#include <utils/fancylineedit.h>
#include <utils/filepath.h>
#include <utils/layoutbuilder.h>
#include <utils/markdownbrowser.h>
#include <utils/qtdesignwidgets.h>

#include <QCompleter>
#include <QLabel>
#include <QListWidget>
#include <QTextEdit>

using namespace Layouting;
using namespace Utils;
using namespace std::string_view_literals;

namespace Lua::Internal {

using QtcTextEdit = Layouting::CompletingTextEdit;

template<class T>
static void processChildren(T *item, const sol::table &children)
{
    for (size_t i = 1; i <= children.size(); ++i) {
        const auto &child = children[i];
        if (child.is<Layout *>()) {
            if (Layout *layout = child.get<Layout *>())
                item->addItem(*layout);
            else
                item->addItem("ERROR");
        } else if (child.is<WidgetHandle *>()) {
            if (QWidget *widget = child.get<WidgetHandle *>()->widget)
                item->addItem(widget);
            else
                item->addItem("ERROR");
        } else if (child.is<BaseAspect>()) {
            child.get<BaseAspect *>()->addToLayout(*item);
        } else if (child.is<QString>()) {
            item->addItem(child.get<QString>());
        } else if (child.is<sol::function>()) {
            const sol::function f = child.get<sol::function>();
            auto res = void_safe_call(f, item);
            QTC_ASSERT_RESULT(res, continue);
        } else if (child.is<Span>()) {
            const Span &span = child.get<Span>();
            item->addItem(span);
        } else if (child.is<Space>()) {
            const Space &space = child.get<Space>();
            item->addItem(space);
        } else if (child.is<Stretch>()) {
            const Stretch &stretch = child.get<Stretch>();
            item->addItem(stretch);
        } else {
            qWarning() << "Incompatible object added to layout item: " << (int) child.get_type()
                       << " (expected Layout, Aspect or function returning Layout)";
        }
    }
}

template<class T>
static std::unique_ptr<T> construct(const sol::table &children)
{
    std::unique_ptr<T> item(new T({}));
    processChildren(item.get(), children);
    item->flush();
    return item;
}

template<class T>
void constructWidget(std::unique_ptr<T> &widget, const sol::table &children)
{
    widget->setWindowTitle(children.get_or<QString>("windowTitle"sv, ""));
    widget->setToolTip(children.get_or<QString>("toolTip"sv, ""));

    for (size_t i = 1; i <= children.size(); ++i) {
        const auto &child = children[i];
        if (child.is<Layout>())
            widget->setLayout(*child.get<Layout *>());
    }
}

/*
 CREATE_HAS_FUNC is a macro that creates a concept that checks if a function exists in a class.

 The arguments must be instances of the type that the function expects.

 If you have a function like this:
    void foo(int, const QString &, QWidget*);

 You can check for it with this macro:
    CREATE_HAS_FUNC(foo, int(), QString(), nullptr)

 You could also specify a value instead of calling the default constructor,
 it would have the same effect but be more verbose:
    CREATE_HAS_FUNC(foo, int(0), QString("hello"), nullptr)

 Both ways will create a concept called has_foo<T> that checks if the function exists, and can
 be called with the specified arguments.
*/
// clang-format off
#define CREATE_HAS_FUNC(name, ...) \
    template<class T> concept has_##name = requires { \
        { std::declval<T>().name(__VA_ARGS__) } -> std::same_as<void>; \
    };

#define CREATE_HAS_FUNC_NAMED(name, hasFuncName, ...) \
    template<class T> concept has_##hasFuncName = requires { \
        { std::declval<T>().name(__VA_ARGS__) } -> std::same_as<void>; \
    };
// clang-format on

CREATE_HAS_FUNC(onDestroyed, nullptr, nullptr)
CREATE_HAS_FUNC(onTextChanged, nullptr, nullptr)
CREATE_HAS_FUNC(onClicked, nullptr, nullptr)
CREATE_HAS_FUNC(setText, QString())
CREATE_HAS_FUNC(setMarkdown, QString())
CREATE_HAS_FUNC(setSizePolicy, QSizePolicy())
CREATE_HAS_FUNC(setReadOnly, bool())
CREATE_HAS_FUNC(setTitle, QString())
CREATE_HAS_FUNC(setValue, int())
CREATE_HAS_FUNC(setSize, int(), int())
CREATE_HAS_FUNC(setWindowFlags, Qt::WindowFlags())
CREATE_HAS_FUNC(setWidgetAttribute, Qt::WidgetAttribute(), bool())
CREATE_HAS_FUNC(setAutoFillBackground, bool())
CREATE_HAS_FUNC(setIconPath, Utils::FilePath())
CREATE_HAS_FUNC(setFlat, bool())
CREATE_HAS_FUNC(setOpenExternalLinks, bool())
CREATE_HAS_FUNC(setIconSize, QSize())
CREATE_HAS_FUNC(setWordWrap, bool())
CREATE_HAS_FUNC(setTextFormat, Qt::TextFormat())
CREATE_HAS_FUNC(setRightSideIconPath, Utils::FilePath())
CREATE_HAS_FUNC(setPlaceHolderText, QString())
CREATE_HAS_FUNC(setPlaceholderText, QString())
CREATE_HAS_FUNC(setCompleter, nullptr)
CREATE_HAS_FUNC(setMinimumHeight, int())
CREATE_HAS_FUNC(onReturnPressed, nullptr, nullptr)
CREATE_HAS_FUNC(onRightSideIconClicked, nullptr, nullptr)
CREATE_HAS_FUNC(setTextInteractionFlags, Qt::TextInteractionFlags())
CREATE_HAS_FUNC(setFixedSize, QSize())
CREATE_HAS_FUNC(setVisible, bool())
CREATE_HAS_FUNC(setIcon, Utils::Icon());
CREATE_HAS_FUNC(setContentsMargins, int(), int(), int(), int());
CREATE_HAS_FUNC(setViewportMargins, int(), int(), int(), int());
CREATE_HAS_FUNC(setCursor, Qt::CursorShape())
CREATE_HAS_FUNC(setMinimumWidth, int());
CREATE_HAS_FUNC(setEnableCodeCopyButton, bool());
CREATE_HAS_FUNC(setDefaultAction, nullptr);
CREATE_HAS_FUNC_NAMED(setRole, setRoleButton, QtcButton::Role());
CREATE_HAS_FUNC_NAMED(setRole, setRoleLabel, QtcLabel::Role());
CREATE_HAS_FUNC(setCompletionBehavior, Utils::CompletingTextEdit::CompletionBehavior());
CREATE_HAS_FUNC(setItems, QStringList());
CREATE_HAS_FUNC(onItemActivated, nullptr, nullptr);
CREATE_HAS_FUNC(onCurrentRowChanged, nullptr, nullptr);

template<class T>
void setProperties(std::unique_ptr<T> &item, const sol::table &children, QObject *guard)
{
    if constexpr (has_setContentsMargins<T>) {
        sol::optional<QMargins> margins = children.get<sol::optional<QMargins>>("contentMargins"sv);
        if (margins)
            item->setContentsMargins(margins->left(), margins->top(), margins->right(), margins->bottom());
    }

    if constexpr (has_setViewportMargins<T>) {
        sol::optional<QMargins> margins = children.get<sol::optional<QMargins>>("viewportMargins"sv);
        if (margins)
            item->setViewportMargins(margins->left(), margins->top(), margins->right(), margins->bottom());
    }

    if constexpr (has_setCursor<T>) {
        const auto cursor = children.get<sol::optional<Qt::CursorShape>>("cursor"sv);
        if (cursor)
            item->setCursor(*cursor);
    }

    if constexpr (has_setMinimumWidth<T>) {
        const auto minw = children.get<sol::optional<int>>("minimumWidth"sv);
        if (minw)
            item->setMinimumWidth(*minw);
    }

    if constexpr (has_setEnableCodeCopyButton<T>) {
        const auto enableCodeCopyButton = children.get<sol::optional<bool>>("enableCodeCopyButton");
        if (enableCodeCopyButton)
            item->setEnableCodeCopyButton(*enableCodeCopyButton);
    }

    if constexpr (has_setDefaultAction<T>) {
        const auto defaultAction = children.get<sol::optional<QAction *>>("defaultAction"sv);
        if (defaultAction)
            item->setDefaultAction(*defaultAction);
    }

    if constexpr (has_setVisible<T>) {
        const auto visible = children.get<sol::optional<bool>>("visible"sv);
        if (visible)
            item->setVisible(*visible);
    }

    if constexpr (has_setIcon<T>) {
        const auto icon = children.get<sol::optional<IconFilePathOrString>>("icon"sv);
        if (icon)
            item->setIcon(*toIcon(*icon));
    }

    if constexpr (has_setTextInteractionFlags<T>) {
        const auto interactionFlags = children.get<sol::optional<sol::table>>("interactionFlags"sv);
        if (interactionFlags) {
            item->setTextInteractionFlags(tableToFlags<Qt::TextInteractionFlag>(*interactionFlags));
        }
    }

    if constexpr (has_setFixedSize<T>) {
        sol::optional<QSize> size = children.get<sol::optional<QSize>>("fixedSize"sv);
        if (size)
            item->setFixedSize(*size);
    }

    if constexpr (has_setWordWrap<T>) {
        const auto wrap = children.get<sol::optional<bool>>("wordWrap"sv);
        if (wrap)
            item->setWordWrap(*wrap);
    }

    if constexpr (has_setTextFormat<T>) {
        const auto format = children.get<sol::optional<Qt::TextFormat>>("textFormat"sv);
        if (format)
            item->setTextFormat(*format);
    }

    if constexpr (has_setRightSideIconPath<T>) {
        const auto path = children.get<sol::optional<Utils::FilePath>>("rightSideIconPath"sv);
        if (path)
            item->setRightSideIconPath(*path);
    }

    if constexpr (has_setPlaceHolderText<T>) {
        const auto compatText = children.get<sol::optional<QString>>("placeHolderText"sv);
        if (compatText)
            qWarning() << "placeHolderText is deprecated, use placeholderText instead.";
        const QString text = children.get<sol::optional<QString>>("placeholderText"sv)
                                 .value_or(compatText.value_or(QString{}));
        if (!text.isEmpty())
            item->setPlaceHolderText(text);
    } else if constexpr (has_setPlaceholderText<T>) {
        const auto text = children.get<sol::optional<QString>>("placeholderText"sv);
        if (text)
            item->setPlaceholderText(*text);
    }

    if constexpr (has_setCompleter<T>) {
        const auto completer = children.get<QCompleter *>("completer"sv);
        if (completer)
            item->setCompleter(completer);
    }

    if constexpr (has_setMinimumHeight<T>) {
        const auto minHeight = children.get<sol::optional<int>>("minimumHeight"sv);
        if (minHeight)
            item->setMinimumHeight(*minHeight);
    }

    if constexpr (has_onReturnPressed<T>) {
        const auto callback = children.get<sol::optional<sol::main_function>>("onReturnPressed"sv);
        if (callback) {
            item->onReturnPressed(guard, [func = *callback](auto &) { void_safe_call(func); });
        }
    }

    if constexpr (has_onRightSideIconClicked<T>) {
        const auto callback = children.get<sol::optional<sol::main_function>>(
            "onRightSideIconClicked");
        if (callback)
            item->onRightSideIconClicked(guard, [func = *callback]() { void_safe_call(func); });
    }

    if constexpr (has_setFlat<T>) {
        const auto flat = children.get<sol::optional<bool>>("flat"sv);
        if (flat)
            item->setFlat(*flat);
    }

    if constexpr (has_setIconPath<T>) {
        const auto iconPath = children.get<sol::optional<FilePath>>("iconPath"sv);
        if (iconPath)
            item->setIconPath(*iconPath);
    }

    if constexpr (has_setIconSize<T>) {
        const auto iconSize = children.get<sol::optional<QSize>>("iconSize"sv);
        if (iconSize)
            item->setIconSize(*iconSize);
    }

    if constexpr (has_setWindowFlags<T>) {
        sol::optional<sol::table> windowFlags = children.get<sol::optional<sol::table>>(
            "windowFlags");
        if (windowFlags) {
            Qt::WindowFlags flags;
            for (const auto &kv : *windowFlags)
                flags.setFlag(static_cast<Qt::WindowType>(kv.second.as<int>()));
            item->setWindowFlags(flags);
        }
    }

    if constexpr (has_setSize<T>) {
        sol::optional<QSize> size = children.get<sol::optional<QSize>>("size"sv);
        if (size)
            item->setSize(size->width(), size->height());
    }

    if constexpr (has_setWidgetAttribute<T>) {
        sol::optional<sol::table> widgetAttributes = children.get<sol::optional<sol::table>>(
            "widgetAttributes");
        if (widgetAttributes) {
            for (const auto &kv : *widgetAttributes)
                item->setWidgetAttribute(
                    static_cast<Qt::WidgetAttribute>(kv.first.as<int>()), kv.second.as<bool>());
        }
    }

    if constexpr (has_setAutoFillBackground<T>) {
        sol::optional<bool> autoFillBackground = children.get<sol::optional<bool>>(
            "autoFillBackground");
        if (autoFillBackground)
            item->setAutoFillBackground(*autoFillBackground);
    }

    if constexpr (has_onTextChanged<T>) {
        sol::optional<sol::main_function> onTextChanged
            = children.get<sol::optional<sol::main_function>>("onTextChanged"sv);
        if (onTextChanged) {
            item->onTextChanged(
                guard,
                [f = *onTextChanged](const QString &text) {
                    auto res = void_safe_call(f, text);
                    QTC_CHECK_RESULT(res);
                });
        }
    }

    if constexpr (has_onDestroyed<T>) {
        sol::optional<sol::main_function> onDestroyed
            = children.get<sol::optional<sol::main_function>>("onDestroyed"sv);
        if (onDestroyed) {
            item->onDestroyed(guard, [f = *onDestroyed]() {
                auto res = void_safe_call(f);
                QTC_CHECK_RESULT(res);
            });
        }
    }

    if constexpr (has_onClicked<T>) {
        sol::optional<sol::main_function> onClicked
            = children.get<sol::optional<sol::main_function>>("onClicked"sv);
        if (onClicked) {
            item->onClicked(
                guard,
                [f = *onClicked]() {
                    auto res = void_safe_call(f);
                    QTC_CHECK_RESULT(res);
                });
        }
    }
    if constexpr (has_setItems<T>) {
        const auto items = children.get<sol::optional<sol::table>>("items"sv);
        if (items) {
            QStringList list;
            for (size_t i = 1; i <= items->size(); ++i)
                list << items->get<QString>(i);
            item->setItems(list);
        }
    }
    if constexpr (has_onItemActivated<T>) {
        sol::optional<sol::main_function> onItemActivated
            = children.get<sol::optional<sol::main_function>>("onItemActivated"sv);
        if (onItemActivated) {
            item->onItemActivated(guard, [f = *onItemActivated](int row) {
                auto res = void_safe_call(f, row);
                QTC_CHECK_RESULT(res);
            });
        }
    }
    if constexpr (has_onCurrentRowChanged<T>) {
        sol::optional<sol::main_function> onCurrentRowChanged
            = children.get<sol::optional<sol::main_function>>("onCurrentRowChanged"sv);
        if (onCurrentRowChanged) {
            item->onCurrentRowChanged(guard, [f = *onCurrentRowChanged](int row) {
                auto res = void_safe_call(f, row);
                QTC_CHECK_RESULT(res);
            });
        }
    }
    if constexpr (has_setText<T>) {
        auto text = children.get<sol::optional<QString>>("text"sv);
        if (text)
            item->setText(*text);
    }
    if constexpr (has_setMarkdown<T>) {
        auto markdown = children.get<sol::optional<QString>>("markdown"sv);
        if (markdown)
            item->setMarkdown(*markdown);
    }
    if constexpr (has_setSizePolicy<T>) {
        auto sizePolicy = children.get<sol::optional<sol::table>>("sizePolicy"sv);
        if (sizePolicy) {
            QTC_ASSERT(
                sizePolicy->size() == 2,
                throw sol::error(
                    "sizePolicy must be array of 2 elements: horizontalPolicy, verticalPolicy.")
                );
            auto horizontalPolicy = sizePolicy->get<QSizePolicy::Policy>(1);
            auto verticalPolicy = sizePolicy->get<QSizePolicy::Policy>(2);
            item->setSizePolicy(QSizePolicy(horizontalPolicy, verticalPolicy));
        }
    }
    if constexpr (has_setTitle<T>) {
        item->setTitle(children.get_or<QString>("title"sv, ""));
    }
    if constexpr (has_setValue<T>) {
        sol::optional<int> value = children.get<sol::optional<int>>("value"sv);
        if (value)
            item->setValue(*value);
    }
    if constexpr (has_setReadOnly<T>) {
        sol::optional<bool> readOnly = children.get<sol::optional<bool>>("readOnly"sv);
        if (readOnly)
            item->setReadOnly(*readOnly);
    }
    if constexpr (has_setOpenExternalLinks<T>) {
        sol::optional<bool> openExternalLinks = children.get<sol::optional<bool>>(
            "openExternalLinks"sv);
        if (openExternalLinks)
            item->setOpenExternalLinks(*openExternalLinks);
    }
    if constexpr (has_setRoleButton<T>) {
        sol::optional<QtcButton::Role> role = children.get<sol::optional<QtcButton::Role>>(
            "role"sv);
        if (role)
            item->setRole(*role);
    }
    if constexpr (has_setRoleLabel<T>) {
        sol::optional<QtcLabel::Role> role = children.get<sol::optional<QtcLabel::Role>>("role"sv);
        if (role)
            item->setRole(*role);
    }
    if constexpr (has_setCompletionBehavior<T>) {
        sol::optional<Utils::CompletingTextEdit::CompletionBehavior> completionBehavior = children.get<sol::optional<Utils::CompletingTextEdit::CompletionBehavior>>(
            "completionBehavior"sv);
        if (completionBehavior)
            item->setCompletionBehavior(*completionBehavior);
    }
}

template<class T>
static std::unique_ptr<Handle<T>> emergeHandle(const T &builder)
{
    auto handle = std::make_unique<Handle<T>>();
    handle->widget = builder.emerge();
    return handle;
}

static QWidget *checkedWidget(const WidgetHandle &handle)
{
    if (!handle.widget)
        throw sol::error("Attempt to access a deleted widget");
    return handle.widget;
}

template<class T>
static typename T::Implementation *implementation(const Handle<T> &handle)
{
    return static_cast<typename T::Implementation *>(checkedWidget(handle));
}

template<class Builder, class... Args>
static auto setter(void (Builder::*set)(Args...))
{
    return [set](const WidgetHandle &handle, Args... args) {
        QWidget *widget = checkedWidget(handle);
        Builder builder({});
        (builder.*set)(args...);
        builder.applyTo(widget);
    };
}

template<class T>
std::unique_ptr<Handle<T>> constructWidgetType(const sol::table &children, QObject *guard)
{
    std::unique_ptr<T> item(new T({}));
    constructWidget(item, children);
    setProperties(item, children, guard);
    std::unique_ptr<Handle<T>> handle = emergeHandle(*item);
    if constexpr (has_setCompleter<T>) {
        if (const auto completer = children.get<QCompleter *>("completer"sv))
            completer->setParent(handle->widget);
    }
    return handle;
}

static std::unique_ptr<Tab> constructTabFromTable(const sol::table &children)
{
    if (children.size() != 2)
        throw sol::error("Tab must have exactly two children");

    auto tabName = children[1];
    if (tabName.get_type() != sol::type::string)
        throw sol::error("Tab name (first argument) must be a string");

    const auto &layout = children[2];
    if (!layout.is<Layout *>())
        throw sol::error("Tab child (second argument) must be a Layout");

    std::unique_ptr<Tab> item = std::make_unique<Tab>(tabName, *layout.get<Layout *>());
    return item;
}

static std::unique_ptr<Tab> constructTab(const QString &tabName, const Layout &layout)
{
    std::unique_ptr<Tab> item = std::make_unique<Tab>(tabName, layout);
    return item;
}

static std::unique_ptr<Span> constructSpanFromTable(const sol::table &children)
{
    if (children.size() != 2 && children.size() != 3)
        throw sol::error("Span must have two or three children");

    auto spanSize = children[1];
    if (spanSize.get_type() != sol::type::number)
        throw sol::error("Span columns (first argument) must be a number");

    const auto &layout_or_row = children[2];
    if (!layout_or_row.is<Layout *>() && layout_or_row.get_type() != sol::type::number)
        throw sol::error("Span child (second argument) must be a Layout or number");

    if (layout_or_row.get_type() == sol::type::number) {
        const auto &layout = children[3];
        if (!layout.is<Layout *>())
            throw sol::error("Span child (third argument) must be a Layout");

        std::unique_ptr<Span> item = std::make_unique<Span>(
            spanSize.get<int>(), layout_or_row.get<int>(), *layout.get<Layout *>());
        return item;
    }

    std::unique_ptr<Span> item = std::make_unique<Span>(spanSize, *layout_or_row.get<Layout *>());
    return item;
}

static std::unique_ptr<Span> constructSpan(int c, const Layout &layout)
{
    std::unique_ptr<Span> item = std::make_unique<Span>(c, layout);
    return item;
}

static std::unique_ptr<Span> constructSpanWithRow(int c, int r, const Layout &layout)
{
    std::unique_ptr<Span> item = std::make_unique<Span>(c, r, layout);
    return item;
}

static std::unique_ptr<Handle<TabWidget>> constructTabWidget(
    const sol::table &children, QObject *guard)
{
    std::unique_ptr<TabWidget> item(new TabWidget({}));
    setProperties(item, children, guard);
    for (size_t i = 1; i <= children.size(); ++i) {
        const auto &child = children[i];
        if (child.is<Tab *>())
            addToTabWidget(item.get(), *child.get<Tab *>());
    }
    return emergeHandle(*item);
}

static std::unique_ptr<Handle<Splitter>> constructSplitter(const sol::table &children)
{
    std::unique_ptr<Splitter> item(new Splitter({}));
    constructWidget(item, children);

    if (const auto &orientation = children.get<sol::optional<QString>>("orientation"sv)) {
        if (*orientation == "horizontal")
            item->setOrientation(Qt::Horizontal);
        else if (*orientation == "vertical")
            item->setOrientation(Qt::Vertical);
        else
            throw sol::error(QString("Invalid orientation: %1").arg(*orientation).toStdString());
    }

    if (const auto collapsible = children.get<sol::optional<bool>>("collapsible"sv))
        item->setChildrenCollapsible(*collapsible);

    for (size_t i = 1; i <= children.size(); ++i) {
        const auto &child = children[i];
        if (child.is<Layout *>()) {
            addToSplitter(item.get(), *child.get<Layout *>());
        } else if (child.is<WidgetHandle *>()) {
            addToSplitter(item.get(), checkedWidget(*child.get<WidgetHandle *>()));
        } else {
            qWarning() << "Incompatible object added to Splitter: " << (int) child.get_type()
                       << " (expected Layout or Widget)";
        }
    }

    if (const auto &stretchFactors = children.get<sol::optional<sol::table>>("stretchFactors"sv)) {
        for (const auto &kv : *stretchFactors) {
            if (kv.second.get_type() != sol::type::number)
                throw sol::error("Stretch factors must be numbers");
            item->setStretchFactor(kv.first.as<int>() - 1, kv.second.as<int>());
        }
    }
    return emergeHandle(*item);
}

static void showWidget(QObject *guard, QWidget *wdgt)
{
    if (!wdgt->parentWidget()) {
        wdgt->setAttribute(Qt::WA_DeleteOnClose, true);
        QObject::connect(guard, &QObject::destroyed, wdgt, &QObject::deleteLater);
        QObject::connect(
            Core::ICore::instance(), &Core::ICore::coreAboutToClose, wdgt, &QObject::deleteLater);
    }
    wdgt->show();
}

void setupGuiModule()
{
    registerProvider("Gui", [](sol::state_view l) -> sol::object {
        const ScriptPluginSpec *pluginSpec = l.get<ScriptPluginSpec *>("PluginSpec"sv);
        QObject *guard = pluginSpec->connectionGuard.get();

        sol::table gui = l.create_table();

        gui.new_usertype<Span>(
            "Span",
            sol::call_constructor,
            sol::factories(&constructSpan, &constructSpanWithRow, &constructSpanFromTable));

        gui.new_usertype<Space>("Space", sol::call_constructor, sol::constructors<Space(int)>());

        gui.new_usertype<Stretch>("Stretch", sol::call_constructor, sol::constructors<Stretch(int)>());

        gui.new_usertype<Object>(
            "Object", sol::no_constructor, "isValid", [](const Object &) { return true; });

        // Layouts
        gui.new_usertype<Layout>(
            "Layout",
            sol::call_constructor,
            sol::factories(&construct<Layout>),
            "show",
            [guard](const Layout &l) { showWidget(guard, l.emerge()); },
            sol::base_classes,
            sol::bases<Object>());

        gui.new_usertype<Form>(
            "Form",
            sol::call_constructor,
            sol::factories(&construct<Form>),
            sol::base_classes,
            sol::bases<Layout, Object>());

        gui.new_usertype<Column>(
            "Column",
            sol::call_constructor,
            sol::factories(&construct<Column>),
            sol::base_classes,
            sol::bases<Layout, Object>());

        gui.new_usertype<Row>(
            "Row",
            sol::call_constructor,
            sol::factories(&construct<Row>),
            sol::base_classes,
            sol::bases<Layout, Object>());
        gui.new_usertype<Flow>(
            "Flow",
            sol::call_constructor,
            sol::factories(&construct<Flow>),
            sol::base_classes,
            sol::bases<Layout, Object>());
        gui.new_usertype<Grid>(
            "Grid",
            sol::call_constructor,
            sol::factories(&construct<Grid>),
            sol::base_classes,
            sol::bases<Layout, Object>());

        // Widgets
        gui.new_usertype<Handle<PushButton>>(
            "PushButton",
            sol::call_constructor,
            sol::factories([guard](const sol::table &children) {
                return constructWidgetType<PushButton>(children, guard);
            }),
            "setText",
            setter(&PushButton::setText),
            "setIconPath",
            setter(&PushButton::setIconPath),
            sol::base_classes,
            sol::bases<WidgetHandle>());

        auto qtcButton = gui.new_usertype<Handle<Utils::QtDesignWidgets::Button>>(
            "QtcButton",
            sol::call_constructor,
            sol::factories([guard](const sol::table &children) {
                return constructWidgetType<Utils::QtDesignWidgets::Button>(children, guard);
            }),
            "setText",
            setter(&Utils::QtDesignWidgets::Button::setText),
            "setIcon",
            setter(&Utils::QtDesignWidgets::Button::setIcon),
            "setRole",
            setter(&Utils::QtDesignWidgets::Button::setRole),
            sol::base_classes,
            sol::bases<WidgetHandle>());

        gui.new_usertype<Handle<Utils::QtDesignWidgets::Switch>>(
            "QtcSwitch",
            sol::call_constructor,
            sol::factories([guard](const sol::table &children) {
                return constructWidgetType<Utils::QtDesignWidgets::Switch>(children, guard);
            }),
            "setText",
            setter(&Utils::QtDesignWidgets::Switch::setText),
            "setChecked",
            setter(&Utils::QtDesignWidgets::Switch::setChecked),
            "onClicked",
            setter(&Utils::QtDesignWidgets::Switch::onClicked),
            sol::base_classes,
            sol::bases<WidgetHandle>());

        auto qtcLabel = gui.new_usertype<Handle<Utils::QtDesignWidgets::Label>>(
            "QtcLabel",
            sol::call_constructor,
            sol::factories([guard](const sol::table &children) {
                return constructWidgetType<Utils::QtDesignWidgets::Label>(children, guard);
            }),
            "setText",
            setter(&Utils::QtDesignWidgets::Label::setText),
            "setRole",
            setter(&Utils::QtDesignWidgets::Label::setRole),
            sol::base_classes,
            sol::bases<WidgetHandle>());

        gui.new_usertype<Handle<Utils::QtDesignWidgets::SearchBox>>(
            "QtcSearchBox",
            sol::call_constructor,
            sol::factories([guard](const sol::table &children) {
                return constructWidgetType<Utils::QtDesignWidgets::SearchBox>(children, guard);
            }),
            "setPlaceholderText",
            setter(&Utils::QtDesignWidgets::SearchBox::setPlaceholderText),
            "setText",
            setter(&Utils::QtDesignWidgets::SearchBox::setText),
            sol::base_classes,
            sol::bases<WidgetHandle>());

        gui.new_usertype<Handle<Label>>(
            "Label",
            sol::call_constructor,
            sol::factories([guard](const sol::table &children) {
                return constructWidgetType<Label>(children, guard);
            }),
            "text",
            sol::property(
                [](const Handle<Label> &h) { return implementation(h)->text(); },
                setter(&Label::setText)),
            sol::base_classes,
            sol::bases<WidgetHandle>());

        gui.new_usertype<Handle<Layouting::MarkdownBrowser>>(
            "MarkdownBrowser",
            sol::call_constructor,
            sol::factories([guard](const sol::table &children) {
                return constructWidgetType<Layouting::MarkdownBrowser>(children, guard);
            }),
            "markdown",
            sol::property(
                [](const Handle<Layouting::MarkdownBrowser> &h) {
                    return implementation(h)->toMarkdown();
                },
                setter(&Layouting::MarkdownBrowser::setMarkdown)),
            sol::base_classes,
            sol::bases<WidgetHandle>());

        gui.new_usertype<Handle<ListWidget>>(
            "ListWidget",
            sol::call_constructor,
            sol::factories([guard](const sol::table &children) {
                return constructWidgetType<ListWidget>(children, guard);
            }),
            "setItems",
            [](Handle<ListWidget> &self, const sol::table &items) {
                QStringList list;
                for (size_t i = 1; i <= items.size(); ++i)
                    list << items.get<QString>(i);
                setter(&ListWidget::setItems)(self, list);
            },
            "addItem",
            setter(&ListWidget::addItem),
            "clear",
            setter(&ListWidget::clear),
            "currentRow",
            [](const Handle<ListWidget> &h) { return implementation(h)->currentRow(); },
            "setCurrentRow",
            setter(&ListWidget::setCurrentRow),
            sol::base_classes,
            sol::bases<WidgetHandle>());

        gui.new_usertype<WidgetHandle>(
            "Widget",
            sol::call_constructor,
            sol::factories([guard](const sol::table &children) -> std::unique_ptr<WidgetHandle> {
                return constructWidgetType<Widget>(children, guard);
            }),
            "isValid",
            [](const WidgetHandle &h) { return !h.widget.isNull(); },
            "show",
            [guard](const WidgetHandle &h) { showWidget(guard, checkedWidget(h)); },
            "activateWindow",
            [](const WidgetHandle &h) { checkedWidget(h)->activateWindow(); },
            "close",
            [](const WidgetHandle &h) { return checkedWidget(h)->close(); },
            "visible",
            sol::property(
                [](const WidgetHandle &h) { return checkedWidget(h)->isVisible(); },
                [](const WidgetHandle &h, bool visible) { checkedWidget(h)->setVisible(visible); }),
            "enabled",
            sol::property(
                [](const WidgetHandle &h) { return checkedWidget(h)->isEnabled(); },
                [](const WidgetHandle &h, bool enabled) { checkedWidget(h)->setEnabled(enabled); }),
            "focus",
            sol::readonly_property(
                [](const WidgetHandle &h) { return checkedWidget(h)->hasFocus(); }),
            "setFocus",
            [](const WidgetHandle &h) { checkedWidget(h)->setFocus(); },
            "toolTip",
            sol::writeonly_property([](const WidgetHandle &h, const QString &toolTip) {
                checkedWidget(h)->setToolTip(toolTip);
            }));

        mirrorEnum(gui, QMetaEnum::fromType<Qt::WidgetAttribute>());
        mirrorEnum(gui, QMetaEnum::fromType<Qt::WindowType>());
        mirrorEnum(gui, QMetaEnum::fromType<Qt::TextFormat>());
        mirrorEnum(gui, QMetaEnum::fromType<Qt::TextInteractionFlag>());
        mirrorEnum(gui, QMetaEnum::fromType<Qt::CursorShape>());
        mirrorEnum(gui.get<sol::table>("QtcButton"), QMetaEnum::fromType<QtcButton::Role>());
        mirrorEnum(gui.get<sol::table>("QtcLabel"), QMetaEnum::fromType<QtcLabel::Role>());

        auto sizePolicy = gui.create_named("QSizePolicy");
        mirrorEnum(sizePolicy, QMetaEnum::fromType<QSizePolicy::Policy>());

        gui.new_usertype<Handle<Stack>>(
            "Stack",
            sol::call_constructor,
            sol::factories([guard](const sol::table &children) {
                return constructWidgetType<Stack>(children, guard);
            }),
            sol::base_classes,
            sol::bases<WidgetHandle>());

        gui.new_usertype<Tab>(
            "Tab", sol::call_constructor, sol::factories(&constructTab, &constructTabFromTable));

        gui.new_usertype<Handle<ScrollArea>>(
            "ScrollArea",
            sol::call_constructor,
            sol::factories(
                [](const Layout &inner) { return emergeHandle(ScrollArea(inner)); },
                [guard](const sol::table &children) {
                    return constructWidgetType<ScrollArea>(children, guard);
                }),
            sol::base_classes,
            sol::bases<WidgetHandle>());

        gui.new_usertype<Handle<TextEdit>>(
            "TextEdit",
            sol::call_constructor,
            sol::factories([guard](const sol::table &children) {
                return constructWidgetType<TextEdit>(children, guard);
            }),
            "markdown",
            sol::readonly_property(
                [](const Handle<TextEdit> &h) { return implementation(h)->toMarkdown(); }),
            sol::base_classes,
            sol::bases<WidgetHandle>());

        gui.new_usertype<Handle<QtcTextEdit>>(
            "QtcTextEdit",
            sol::call_constructor,
            sol::factories([guard](const sol::table &children) {
                return constructWidgetType<QtcTextEdit>(children, guard);
            }),
            "CompletionBehavior",
            gui.create_with(
                "OnKeyPress", Utils::CompletingTextEdit::CompletionBehavior::OnKeyPress,
                "OnTextChange", Utils::CompletingTextEdit::CompletionBehavior::OnTextChange),
            "markdown",
            sol::property(
                [](const Handle<QtcTextEdit> &h) { return implementation(h)->toMarkdown(); },
                setter(&QtcTextEdit::setMarkdown)),
            "text",
            sol::property(
                [](const Handle<QtcTextEdit> &h) {
                    return implementation(h)->toPlainText();
                },
                setter(&QtcTextEdit::setText)),
            "completionBehavior",
            sol::property(
                [](const Handle<QtcTextEdit> &h) {
                    return implementation(h)->completionBehavior();
                },
                setter(&QtcTextEdit::setCompletionBehavior)),
            "placeHolderText",
            sol::writeonly_property(
                setter(&QtcTextEdit::setPlaceHolderText)),
            "rightSideIconPath",
            sol::writeonly_property(
                setter(&QtcTextEdit::setRightSideIconPath)),
            "completer",
            sol::property(
                [](const Handle<QtcTextEdit> &h) { return implementation(h)->completer(); },
                setter(&QtcTextEdit::setCompleter)),
            sol::base_classes,
            sol::bases<WidgetHandle>());

        gui.new_usertype<Handle<LineEdit>>(
            "LineEdit",
            sol::call_constructor,
            sol::factories([guard](const sol::table &children) {
                return constructWidgetType<LineEdit>(children, guard);
            }),
            "text",
            sol::property(
                [](const Handle<LineEdit> &h) { return implementation(h)->text(); },
                setter(&LineEdit::setText)),
            "rightSideIconPath",
            sol::writeonly_property(setter(&LineEdit::setRightSideIconPath)),
            sol::base_classes,
            sol::bases<WidgetHandle>());

        gui.new_usertype<Handle<SpinBox>>(
            "SpinBox",
            sol::call_constructor,
            sol::factories([guard](const sol::table &children) {
                return constructWidgetType<SpinBox>(children, guard);
            }),
            sol::base_classes,
            sol::bases<WidgetHandle>());
        gui.new_usertype<Handle<Splitter>>(
            "Splitter",
            sol::call_constructor,
            sol::factories(&constructSplitter),
            sol::base_classes,
            sol::bases<WidgetHandle>());
        gui.new_usertype<Handle<ToolBar>>(
            "ToolBar",
            sol::call_constructor,
            sol::factories([guard](const sol::table &children) {
                return constructWidgetType<ToolBar>(children, guard);
            }),
            sol::base_classes,
            sol::bases<WidgetHandle>());
        gui.new_usertype<Handle<ToolButton>>(
            "ToolButton",
            sol::call_constructor,
            sol::factories([guard](const sol::table &children) {
                return constructWidgetType<ToolButton>(children, guard);
            }),
            sol::base_classes,
            sol::bases<WidgetHandle>());
        gui.new_usertype<Handle<TabWidget>>(
            "TabWidget",
            sol::call_constructor,
            sol::factories([guard](const sol::table &children) {
                return constructTabWidget(children, guard);
            }),
            sol::base_classes,
            sol::bases<WidgetHandle>());

        gui.new_usertype<Handle<Group>>(
            "Group",
            sol::call_constructor,
            sol::factories([guard](const sol::table &children) {
                return constructWidgetType<Group>(children, guard);
            }),
            sol::base_classes,
            sol::bases<WidgetHandle>());

        gui.new_usertype<Handle<Spinner>>(
            "Spinner",
            sol::call_constructor,
            sol::factories([guard](const sol::table &children) {
                return constructWidgetType<Spinner>(children, guard);
            }),
            "running",
            sol::writeonly_property(setter(&Spinner::setRunning)),
            "decorated",
            sol::writeonly_property(setter(&Spinner::setDecorated)),
            sol::base_classes,
            sol::bases<WidgetHandle>());

        // An overlay spinner that centres itself on an existing widget and dims
        // it while running, unlike the inline Spinner above. Construct with
        // SpinnerOverlay(widget) and toggle its 'running' property.
        gui.new_usertype<SpinnerSolution::Spinner>(
            "SpinnerOverlay",
            sol::call_constructor,
            sol::factories([](WidgetHandle *on) -> SpinnerSolution::Spinner * {
                QWidget *target = checkedWidget(*on);
                auto *spinner = new SpinnerSolution::Spinner(
                    SpinnerSolution::SpinnerSize::Medium, target);
                spinner->hide();
                return spinner; // parented to target; destroyed with it
            }),
            "running",
            sol::property(
                [](SpinnerSolution::Spinner &self) { return self.isVisible(); },
                [](SpinnerSolution::Spinner &self, bool running) {
                    self.setVisible(running);
                }));

        gui.new_usertype<Handle<QtDesignWidgets::IconDisplay>>(
            "IconDisplay",
            sol::call_constructor,
            sol::factories([guard](const sol::table &children) {
                return constructWidgetType<QtDesignWidgets::IconDisplay>(children, guard);
            }),
            sol::base_classes,
            sol::bases<WidgetHandle>());

        gui.new_usertype<Preview>(
            "Preview",
            sol::call_constructor,
            sol::factories(&construct<Preview>),
            "show",
            &Preview::show,
            sol::base_classes,
            sol::bases<Column, Layout, Object>());

        gui["br"] = &br;
        gui["st"] = &st;
        gui["empty"] = &empty;
        gui["hr"] = &hr;
        gui["noMargin"] = &noMargin;
        gui["normalMargin"] = &normalMargin;
        gui["withFormAlignment"] = &withFormAlignment;
        gui["spacing"] = &spacing;
        gui["stretch"] = &stretch;

        return gui;
    });
}

} // namespace Lua::Internal

template<typename T>
concept IsWidgetHandle = std::is_base_of_v<Lua::Internal::WidgetHandle, T>;

template<typename TYPE, typename Handler>
requires IsWidgetHandle<TYPE>
sol::optional<TYPE *> sol_lua_check_get(
    sol::types<TYPE *>, lua_State *L, int index, Handler &&handler, sol::stack::record &tracking)
{
    sol::stack::qualified_check_getter<TYPE *> cg{};
    auto result = cg.get(L, index, std::forward<Handler>(handler), tracking);
    if (!result) {
        return sol::nullopt;
    }
    if (!(*result)->widget)
        throw sol::error(std::string("Attempt to access a deleted ") + typeid(TYPE).name());
    return result;
}
