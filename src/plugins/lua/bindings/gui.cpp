// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../luaengine.h"

#include "inheritance.h"
#include "utils.h"

#include <utils/aspects.h>
#include <utils/filepath.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcwidgets.h>

#include <QMetaEnum>
#include <QCompleter>

using namespace Layouting;
using namespace Utils;
using namespace std::string_view_literals;

namespace Lua::Internal {

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
        } else if (child.is<Widget *>()) {
            if (Widget *widget = child.get<Widget *>())
                item->addItem(*widget);
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
// clang-format on

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
CREATE_HAS_FUNC(setRole, QtcButton::Role());

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
        const auto text = children.get<sol::optional<QString>>("placeHolderText"sv);
        if (text)
            item->setPlaceHolderText(*text);
    }

    if constexpr (has_setCompleter<T>) {
        const auto completer = children.get<QCompleter *>("completer"sv);
        if (completer) {
            item->setCompleter(completer);
            completer->setParent(item->emerge());
        }
    }

    if constexpr (has_setMinimumHeight<T>) {
        const auto minHeight = children.get<sol::optional<int>>("minimumHeight"sv);
        if (minHeight)
            item->setMinimumHeight(*minHeight);
    }

    if constexpr (has_onReturnPressed<T>) {
        const auto callback = children.get<sol::optional<sol::main_function>>("onReturnPressed"sv);
        if (callback) {
            item->onReturnPressed(guard, [func = *callback]() { void_safe_call(func); });
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
    if constexpr (has_setRole<T>) {
        sol::optional<QtcButton::Role> role = children.get<sol::optional<QtcButton::Role>>(
            "role"sv);
        if (role)
            item->setRole(*role);
    }
}

template<class T>
std::unique_ptr<T> constructWidgetType(const sol::table &children, QObject *guard)
{
    std::unique_ptr<T> item(new T({}));
    constructWidget(item, children);
    setProperties(item, children, guard);
    return item;
}

std::unique_ptr<Tab> constructTabFromTable(const sol::table &children)
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

std::unique_ptr<Tab> constructTab(const QString &tabName, const Layout &layout)
{
    std::unique_ptr<Tab> item = std::make_unique<Tab>(tabName, layout);
    return item;
}

std::unique_ptr<Span> constructSpanFromTable(const sol::table &children)
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

std::unique_ptr<Span> constructSpan(int c, const Layout &layout)
{
    std::unique_ptr<Span> item = std::make_unique<Span>(c, layout);
    return item;
}

std::unique_ptr<Span> constructSpanWithRow(int c, int r, const Layout &layout)
{
    std::unique_ptr<Span> item = std::make_unique<Span>(c, r, layout);
    return item;
}

std::unique_ptr<TabWidget> constructTabWidget(const sol::table &children, QObject *guard)
{
    std::unique_ptr<TabWidget> item(new TabWidget({}));
    setProperties(item, children, guard);
    for (size_t i = 1; i <= children.size(); ++i) {
        const auto &child = children[i];
        if (child.is<Tab *>())
            addToTabWidget(item.get(), *child.get<Tab *>());
    }
    return item;
}

std::unique_ptr<Splitter> constructSplitter(const sol::table &children)
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
        } else if (child.is<Widget *>()) {
            addToSplitter(item.get(), *child.get<Widget *>());
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
    return item;
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

        // Layouts
        gui.new_usertype<Layout>(
            "Layout",
            sol::call_constructor,
            sol::factories(&construct<Layout>),
            "show",
            &Layout::show,
            sol::base_classes,
            sol::bases<Object, Thing>());

        gui.new_usertype<Form>(
            "Form",
            sol::call_constructor,
            sol::factories(&construct<Form>),
            sol::base_classes,
            sol::bases<Layout, Object, Thing>());

        gui.new_usertype<Column>(
            "Column",
            sol::call_constructor,
            sol::factories(&construct<Column>),
            sol::base_classes,
            sol::bases<Layout, Object, Thing>());

        gui.new_usertype<Row>(
            "Row",
            sol::call_constructor,
            sol::factories(&construct<Row>),
            sol::base_classes,
            sol::bases<Layout, Object, Thing>());
        gui.new_usertype<Flow>(
            "Flow",
            sol::call_constructor,
            sol::factories(&construct<Flow>),
            sol::base_classes,
            sol::bases<Layout, Object, Thing>());
        gui.new_usertype<Grid>(
            "Grid",
            sol::call_constructor,
            sol::factories(&construct<Grid>),
            sol::base_classes,
            sol::bases<Layout, Object, Thing>());

        // Widgets
        gui.new_usertype<PushButton>(
            "PushButton",
            sol::call_constructor,
            sol::factories([guard](const sol::table &children) {
                return constructWidgetType<PushButton>(children, guard);
            }),
            "setText",
            &PushButton::setText,
            "setIconPath",
            &PushButton::setIconPath,
            sol::base_classes,
            sol::bases<Widget, Object, Thing>());

        gui.new_usertype<Utils::QtcWidgets::Button>(
            "QtcButton",
            sol::call_constructor,
            sol::factories([](const sol::table &children) {
                return constructWidgetType<Utils::QtcWidgets::Button>(children, nullptr);
            }),
            "setText",
            &Utils::QtcWidgets::Button::setText,
            "setIcon",
            &Utils::QtcWidgets::Button::setIcon,
            "setRole",
            &Utils::QtcWidgets::Button::setRole,
            sol::base_classes,
            sol::bases<Widget, Object, Thing>());

        gui.new_usertype<Utils::QtcWidgets::Switch>(
            "QtcSwitch",
            sol::call_constructor,
            sol::factories([guard](const sol::table &children) {
                return constructWidgetType<Utils::QtcWidgets::Switch>(children, guard);
            }),
            "setText",
            &Utils::QtcWidgets::Switch::setText,
            "setChecked",
            &Utils::QtcWidgets::Switch::setChecked,
            "onClicked",
            &Utils::QtcWidgets::Switch::onClicked,
            sol::base_classes,
            sol::bases<Widget, Object, Thing>());

        gui.new_usertype<Label>(
            "Label",
            sol::call_constructor,
            sol::factories([guard](const sol::table &children) {
                return constructWidgetType<Label>(children, guard);
            }),
            "text",
            sol::property(&Label::text),
            sol::base_classes,
            sol::bases<Widget, Object, Thing>());

        gui.new_usertype<Layouting::MarkdownBrowser>(
            "MarkdownBrowser",
            sol::call_constructor,
            sol::factories([guard](const sol::table &children) {
                return constructWidgetType<Layouting::MarkdownBrowser>(children, guard);
            }),
            "markdown",
            sol::property(
                &Layouting::MarkdownBrowser::toMarkdown, &Layouting::MarkdownBrowser::setMarkdown),
            sol::base_classes,
            sol::bases<Widget, Object, Thing>());

        gui.new_usertype<Widget>(
            "Widget",
            sol::call_constructor,
            sol::factories([guard](const sol::table &children) {
                return constructWidgetType<Widget>(children, guard);
            }),
            "show",
            &Widget::show,
            "activateWindow",
            &Widget::activateWindow,
            "close",
            &Widget::close,
            "visible",
            sol::property(&Widget::isVisible, &Widget::setVisible),
            "enabled",
            sol::property(&Widget::isEnabled, &Widget::setEnabled),
            "focus",
            sol::property([](Widget *self) { return self->emerge()->hasFocus(); }),
            "setFocus",
            [](Widget *self) { self->emerge()->setFocus(); },
            sol::base_classes,
            sol::bases<Object, Thing>());

        mirrorEnum(gui, QMetaEnum::fromType<Qt::WidgetAttribute>());
        mirrorEnum(gui, QMetaEnum::fromType<Qt::WindowType>());
        mirrorEnum(gui, QMetaEnum::fromType<Qt::TextFormat>());
        mirrorEnum(gui, QMetaEnum::fromType<Qt::TextInteractionFlag>());
        mirrorEnum(gui, QMetaEnum::fromType<Qt::CursorShape>());
        mirrorEnum(gui, QMetaEnum::fromType<QtcButton::Role>());

        auto sizePolicy = gui.create_named("QSizePolicy");
        mirrorEnum(sizePolicy, QMetaEnum::fromType<QSizePolicy::Policy>());

        gui.new_usertype<Stack>(
            "Stack",
            sol::call_constructor,
            sol::factories([guard](const sol::table &children) {
                return constructWidgetType<Stack>(children, guard);
            }),
            sol::base_classes,
            sol::bases<Widget, Object, Thing>());

        gui.new_usertype<Tab>(
            "Tab",
            sol::call_constructor,
            sol::factories(&constructTab, &constructTabFromTable),
            sol::base_classes,
            sol::bases<Widget, Object, Thing>());

        gui.new_usertype<ScrollArea>(
            "ScrollArea",
            sol::call_constructor,
            sol::factories(
                [](const Layout &inner) {
                    auto item = std::make_unique<ScrollArea>(inner);
                    return item;
                },
                [guard](const sol::table &children) {
                    return constructWidgetType<ScrollArea>(children, guard);
                }),
            sol::base_classes,
            sol::bases<Widget, Object, Thing>());

        gui.new_usertype<TextEdit>(
            "TextEdit",
            sol::call_constructor,
            sol::factories([guard](const sol::table &children) {
                return constructWidgetType<TextEdit>(children, guard);
            }),
            "markdown",
            sol::property(&TextEdit::markdown),
            sol::base_classes,
            sol::bases<Widget, Object, Thing>());

        gui.new_usertype<LineEdit>(
            "LineEdit",
            sol::call_constructor,
            sol::factories([guard](const sol::table &children) {
                return constructWidgetType<LineEdit>(children, guard);
            }),
            "text",
            sol::property(&LineEdit::text, &LineEdit::setText),
            sol::base_classes,
            sol::bases<Widget, Object, Thing>());

        gui.new_usertype<SpinBox>(
            "SpinBox",
            sol::call_constructor,
            sol::factories([guard](const sol::table &children) {
                return constructWidgetType<SpinBox>(children, guard);
            }),
            sol::base_classes,
            sol::bases<Widget, Object, Thing>());
        gui.new_usertype<Splitter>(
            "Splitter",
            sol::call_constructor,
            sol::factories(&constructSplitter),
            sol::base_classes,
            sol::bases<Widget, Object, Thing>());
        gui.new_usertype<ToolBar>(
            "ToolBar",
            sol::call_constructor,
            sol::factories([guard](const sol::table &children) {
                return constructWidgetType<ToolBar>(children, guard);
            }),
            sol::base_classes,
            sol::bases<Widget, Object, Thing>());
        gui.new_usertype<ToolButton>(
            "ToolButton",
            sol::call_constructor,
            sol::factories([guard](const sol::table &children) {
                return constructWidgetType<ToolButton>(children, guard);
            }),
            sol::base_classes,
            sol::bases<Widget, Object, Thing>());
        gui.new_usertype<TabWidget>(
            "TabWidget",
            sol::call_constructor,
            sol::factories([guard](const sol::table &children) {
                return constructTabWidget(children, guard);
            }),
            sol::base_classes,
            sol::bases<Widget, Object, Thing>());

        gui.new_usertype<Group>(
            "Group",
            sol::call_constructor,
            sol::factories([guard](const sol::table &children) {
                return constructWidgetType<Group>(children, guard);
            }),
            sol::base_classes,
            sol::bases<Widget, Object, Thing>());

        gui.new_usertype<Spinner>(
            "Spinner",
            sol::call_constructor,
            sol::factories([guard](const sol::table &children) {
                return constructWidgetType<Spinner>(children, guard);
            }),
            "running",
            sol::property(&Spinner::setRunning),
            "decorated",
            sol::property(&Spinner::setDecorated),
            sol::base_classes,
            sol::bases<Widget, Object, Thing>());

        gui.new_usertype<Layouting::IconDisplay>(
            "IconDisplay",
            sol::call_constructor,
            sol::factories([guard](const sol::table &children) {
                return constructWidgetType<Layouting::IconDisplay>(children, guard);
            }),
            sol::base_classes,
            sol::bases<Widget, Object, Thing>());

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
