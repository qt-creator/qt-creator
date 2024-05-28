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
            item->addItem(*child.get<Layout *>());
        } else if (child.is<Widget *>()) {
            item->addItem(*child.get<Widget *>());
        } else if (child.is<BaseAspect>()) {
            child.get<BaseAspect *>()->addToLayout(*item);
        } else if (child.is<QString>()) {
            item->addItem(child.get<QString>());
        } else if (child.is<sol::function>()) {
            const sol::function f = child.get<sol::function>();
            auto res = LuaEngine::void_safe_call(f, item);
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
                       << " (expected LayoutItem, Aspect or function returning LayoutItem)";
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

    for (size_t i = 1; i < children.size(); ++i) {
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

template<class T>
void setProperties(std::unique_ptr<T> &item, const sol::table &children, QObject *guard)
{
    if constexpr (hasOnTextChanged<T, void (T::*)(const QString &)>::value) {
        sol::optional<sol::protected_function> onTextChanged
            = children.get<sol::optional<sol::protected_function>>("onTextChanged");
        if (onTextChanged) {
            item->onTextChanged(
                [f = *onTextChanged](const QString &text) {
                    auto res = LuaEngine::void_safe_call(f, text);
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
                    auto res = LuaEngine::void_safe_call(f);
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

std::unique_ptr<Tab> constructTab(const QString &tabName, const Layout &layout)
{
    std::unique_ptr<Tab> item = std::make_unique<Tab>(tabName, layout);
    return item;
}

std::unique_ptr<TabWidget> constructTabWidget(const sol::table &children, QObject *guard)
{
    std::unique_ptr<TabWidget> item(new TabWidget({}));
    setProperties(item, children, guard);
    for (size_t i = 1; i < children.size(); ++i) {
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

    for (size_t i = 1; i < children.size(); ++i) {
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
    return item;
}

void addLayoutModule()
{
    LuaEngine::registerProvider("Layout", [](sol::state_view l) -> sol::object {
        const ScriptPluginSpec *pluginSpec = l.get<ScriptPluginSpec *>("PluginSpec");
        QObject *guard = pluginSpec->connectionGuard.get();

        sol::table layout = l.create_table();

        layout.new_usertype<Span>(
            "Span", sol::call_constructor, sol::constructors<Span(int n, const Layout::I &item)>());

        layout.new_usertype<Space>("Space", sol::call_constructor, sol::constructors<Space(int)>());

        layout.new_usertype<Stretch>(
            "Stretch", sol::call_constructor, sol::constructors<Stretch(int)>());

        // Layouts
        layout.new_usertype<Form>(
            "Form",
            sol::call_constructor,
            sol::factories(&construct<Form>),
            sol::base_classes,
            sol::bases<Layout, Object, Thing>());

        layout.new_usertype<Column>(
            "Column",
            sol::call_constructor,
            sol::factories(&construct<Column>),
            sol::base_classes,
            sol::bases<Layout, Object, Thing>());

        layout.new_usertype<Row>(
            "Row",
            sol::call_constructor,
            sol::factories(&construct<Row>),
            sol::base_classes,
            sol::bases<Layout, Object, Thing>());
        layout.new_usertype<Flow>(
            "Flow",
            sol::call_constructor,
            sol::factories(&construct<Flow>),
            sol::base_classes,
            sol::bases<Layout, Object, Thing>());
        layout.new_usertype<Grid>(
            "Grid",
            sol::call_constructor,
            sol::factories(&construct<Grid>),
            sol::base_classes,
            sol::bases<Layout, Object, Thing>());

        // Widgets
        layout.new_usertype<PushButton>(
            "PushButton",
            sol::call_constructor,
            sol::factories([guard](const sol::table &children) {
                return constructWidgetType<PushButton>(children, guard);
            }),
            sol::base_classes,
            sol::bases<Widget, Object, Thing>());

        layout.new_usertype<Widget>(
            "Widget",
            sol::call_constructor,
            sol::factories([guard](const sol::table &children) {
                return constructWidgetType<Widget>(children, guard);
            }),
            "show",
            &Widget::show,
            "resize",
            &Widget::resize,
            sol::base_classes,
            sol::bases<Object, Thing>());

        layout.new_usertype<Stack>(
            "Stack",
            sol::call_constructor,
            sol::factories([guard](const sol::table &children) {
                return constructWidgetType<Stack>(children, guard);
            }),
            sol::base_classes,
            sol::bases<Widget, Object, Thing>());

        layout.new_usertype<Tab>(
            "Tab",
            sol::call_constructor,
            sol::factories(&constructTab),
            sol::base_classes,
            sol::bases<Widget, Object, Thing>());

        layout.new_usertype<TextEdit>(
            "TextEdit",
            sol::call_constructor,
            sol::factories([guard](const sol::table &children) {
                return constructWidgetType<TextEdit>(children, guard);
            }),
            sol::base_classes,
            sol::bases<Widget, Object, Thing>());

        layout.new_usertype<SpinBox>(
            "SpinBox",
            sol::call_constructor,
            sol::factories([guard](const sol::table &children) {
                return constructWidgetType<SpinBox>(children, guard);
            }),
            sol::base_classes,
            sol::bases<Widget, Object, Thing>());
        layout.new_usertype<Splitter>(
            "Splitter",
            sol::call_constructor,
            sol::factories(&constructSplitter),
            sol::base_classes,
            sol::bases<Widget, Object, Thing>());
        layout.new_usertype<ToolBar>(
            "ToolBar",
            sol::call_constructor,
            sol::factories([guard](const sol::table &children) {
                return constructWidgetType<ToolBar>(children, guard);
            }),
            sol::base_classes,
            sol::bases<Widget, Object, Thing>());
        layout.new_usertype<TabWidget>(
            "TabWidget",
            sol::call_constructor,
            sol::factories([guard](const sol::table &children) {
                return constructTabWidget(children, guard);
            }),
            sol::base_classes,
            sol::bases<Widget, Object, Thing>());

        layout.new_usertype<Group>(
            "Group",
            sol::call_constructor,
            sol::factories([guard](const sol::table &children) {
                return constructWidgetType<Group>(children, guard);
            }),
            sol::base_classes,
            sol::bases<Widget, Object, Thing>());

        layout["br"] = &br;
        layout["st"] = &st;
        layout["empty"] = &empty;
        layout["hr"] = &hr;
        layout["noMargin"] = &noMargin;
        layout["normalMargin"] = &normalMargin;

        return layout;
    });
}

} // namespace Lua::Internal
