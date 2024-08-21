// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../luaengine.h"

#include "inheritance.h"

#include <utils/aspects.h>
#include <utils/layoutbuilder.h>

using namespace Layouting;
using namespace Utils;

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
            QTC_ASSERT_EXPECTED(res, continue);
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
    widget->setWindowTitle(children.get_or<QString>("windowTitle", ""));
    widget->setToolTip(children.get_or<QString>("toolTip", ""));

    for (size_t i = 1; i <= children.size(); ++i) {
        const auto &child = children[i];
        if (child.is<Layout>())
            widget->setLayout(*child.get<Layout *>());
    }
}

#define HAS_MEM_FUNC(func, name) \
    template<typename T, typename Sign> \
    struct name \
    { \
        typedef char yes[1]; \
        typedef char no[2]; \
        template<typename U, U> \
        struct type_check; \
        template<typename _1> \
        static yes &chk(type_check<Sign, &_1::func> *); \
        template<typename> \
        static no &chk(...); \
        static bool const value = sizeof(chk<T>(0)) == sizeof(yes); \
    }

HAS_MEM_FUNC(onTextChanged, hasOnTextChanged);
HAS_MEM_FUNC(onClicked, hasOnClicked);
HAS_MEM_FUNC(setText, hasSetText);
HAS_MEM_FUNC(setTitle, hasSetTitle);
HAS_MEM_FUNC(setValue, hasSetValue);
HAS_MEM_FUNC(setSize, hasSetSize);
HAS_MEM_FUNC(setWindowFlags, hasSetWindowFlags);

template<class T>
void setProperties(std::unique_ptr<T> &item, const sol::table &children, QObject *guard)
{
    if constexpr (hasSetWindowFlags<T, void (T::*)(Qt::WindowFlags)>::value) {
        sol::optional<sol::table> windowFlags = children.get<sol::optional<sol::table>>(
            "windowFlags");
        if (windowFlags) {
            Qt::WindowFlags flags;
            for (const auto &kv : *windowFlags)
                flags.setFlag(static_cast<Qt::WindowType>(kv.second.as<int>()));
            item->setWindowFlags(flags);
        }
    }

    if constexpr (hasSetSize<T, void (T::*)(int, int)>::value) {
        sol::optional<sol::table> size = children.get<sol::optional<sol::table>>("size");
        if (size) {
            if (size->size() == 2)
                item->setSize(size->get<int>(1), size->get<int>(2));
            else
                throw sol::error("size must have exactly two elements");
        }
    }

    if constexpr (hasOnTextChanged<T, void (T::*)(const QString &)>::value) {
        sol::optional<sol::protected_function> onTextChanged
            = children.get<sol::optional<sol::protected_function>>("onTextChanged");
        if (onTextChanged) {
            item->onTextChanged(
                [f = *onTextChanged](const QString &text) {
                    auto res = void_safe_call(f, text);
                    QTC_CHECK_EXPECTED(res);
                },
                guard);
        }
    }
    if constexpr (hasOnClicked<T, void (T::*)(const std::function<void()> &, QObject *guard)>::value) {
        sol::optional<sol::protected_function> onClicked
            = children.get<sol::optional<sol::protected_function>>("onClicked");
        if (onClicked) {
            item->onClicked(
                [f = *onClicked]() {
                    auto res = void_safe_call(f);
                    QTC_CHECK_EXPECTED(res);
                },
                guard);
        }
    }
    if constexpr (hasSetText<T, void (T::*)(const QString &)>::value) {
        item->setText(children.get_or<QString>("text", ""));
    }
    if constexpr (hasSetTitle<T, void (T::*)(const QString &)>::value) {
        item->setTitle(children.get_or<QString>("title", ""));
    }
    if constexpr (hasSetValue<T, void (T::*)(int)>::value) {
        sol::optional<int> value = children.get<sol::optional<int>>("value");
        if (value)
            item->setValue(*value);
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

    if (const auto &orientation = children.get<sol::optional<QString>>("orientation")) {
        if (*orientation == "horizontal")
            item->setOrientation(Qt::Horizontal);
        else if (*orientation == "vertical")
            item->setOrientation(Qt::Vertical);
        else
            throw sol::error(QString("Invalid orientation: %1").arg(*orientation).toStdString());
    }

    if (const auto collapsible = children.get<sol::optional<bool>>("collapsible"))
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

    if (const auto &stretchFactors = children.get<sol::optional<sol::table>>("stretchFactors")) {
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
        const ScriptPluginSpec *pluginSpec = l.get<ScriptPluginSpec *>("PluginSpec");
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
            sol::base_classes,
            sol::bases<Widget, Object, Thing>());

        gui.new_usertype<Label>(
            "Label",
            sol::call_constructor,
            sol::factories([guard](const sol::table &children) {
                return constructWidgetType<Label>(children, guard);
            }),
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
            sol::base_classes,
            sol::bases<Object, Thing>());

        gui["WindowType"] = l.create_table_with(
            "Widget",
            Qt::Widget,
            "Window",
            Qt::Window,
            "Dialog",
            Qt::Dialog,
            "Sheet",
            Qt::Sheet,
            "Drawer",
            Qt::Drawer,
            "Popup",
            Qt::Popup,
            "Tool",
            Qt::Tool,
            "ToolTip",
            Qt::ToolTip,
            "SplashScreen",
            Qt::SplashScreen,
            "Desktop",
            Qt::Desktop,
            "SubWindow",
            Qt::SubWindow,
            "ForeignWindow",
            Qt::ForeignWindow,
            "CoverWindow",
            Qt::CoverWindow,

            "WindowType_Mask",
            Qt::WindowType_Mask,
            "MSWindowsFixedSizeDialogHint",
            Qt::MSWindowsFixedSizeDialogHint,
            "MSWindowsOwnDC",
            Qt::MSWindowsOwnDC,
            "BypassWindowManagerHint",
            Qt::BypassWindowManagerHint,
            "X11BypassWindowManagerHint",
            Qt::X11BypassWindowManagerHint,
            "FramelessWindowHint",
            Qt::FramelessWindowHint,
            "WindowTitleHint",
            Qt::WindowTitleHint,
            "WindowSystemMenuHint",
            Qt::WindowSystemMenuHint,
            "WindowMinimizeButtonHint",
            Qt::WindowMinimizeButtonHint,
            "WindowMaximizeButtonHint",
            Qt::WindowMaximizeButtonHint,
            "WindowMinMaxButtonsHint",
            Qt::WindowMinMaxButtonsHint,
            "WindowContextHelpButtonHint",
            Qt::WindowContextHelpButtonHint,
            "WindowShadeButtonHint",
            Qt::WindowShadeButtonHint,
            "WindowStaysOnTopHint",
            Qt::WindowStaysOnTopHint,
            "WindowTransparentForInput",
            Qt::WindowTransparentForInput,
            "WindowOverridesSystemGestures",
            Qt::WindowOverridesSystemGestures,
            "WindowDoesNotAcceptFocus",
            Qt::WindowDoesNotAcceptFocus,
            "MaximizeUsingFullscreenGeometryHint",
            Qt::MaximizeUsingFullscreenGeometryHint,

            "CustomizeWindowHint",
            Qt::CustomizeWindowHint,
            "WindowStaysOnBottomHint",
            Qt::WindowStaysOnBottomHint,
            "WindowCloseButtonHint",
            Qt::WindowCloseButtonHint,
            "MacWindowToolBarButtonHint",
            Qt::MacWindowToolBarButtonHint,
            "BypassGraphicsProxyWidget",
            Qt::BypassGraphicsProxyWidget,
            "NoDropShadowWindowHint",
            Qt::NoDropShadowWindowHint,
            "WindowFullscreenButtonHint",
            Qt::WindowFullscreenButtonHint);

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

        gui.new_usertype<TextEdit>(
            "TextEdit",
            sol::call_constructor,
            sol::factories([guard](const sol::table &children) {
                return constructWidgetType<TextEdit>(children, guard);
            }),
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

        gui["br"] = &br;
        gui["st"] = &st;
        gui["empty"] = &empty;
        gui["hr"] = &hr;
        gui["noMargin"] = &noMargin;
        gui["normalMargin"] = &normalMargin;
        gui["withFormAlignment"] = &withFormAlignment;
        gui["spacing"] = &spacing;

        return gui;
    });
}

} // namespace Lua::Internal
