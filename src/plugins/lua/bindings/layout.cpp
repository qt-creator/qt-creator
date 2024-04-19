// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../luaengine.h"

#include "inheritance.h"

#include <utils/aspects.h>
#include <utils/layoutbuilder.h>

using namespace Layouting;
using namespace Utils;

namespace Lua::Internal {

static void processChildren(LayoutItem *item, const sol::table &children)
{
    for (size_t i = 1; i <= children.size(); ++i) {
        const sol::object v = children[i];

        if (v.is<LayoutItem *>()) {
            item->addItem(*v.as<LayoutItem *>());
        } else if (v.is<BaseAspect>()) {
            v.as<BaseAspect *>()->addToLayout(*item);
        } else if (v.is<QString>()) {
            item->addItem(v.as<QString>());
        } else if (v.is<sol::function>()) {
            const sol::function f = v.as<sol::function>();
            auto res = LuaEngine::safe_call<LayoutItem *>(f);
            QTC_ASSERT_EXPECTED(res, continue);
            item->addItem(**res);
        } else {
            qWarning() << "Incompatible object added to layout item: " << (int) v.get_type()
                       << " (expected LayoutItem, Aspect or function returning LayoutItem)";
        }
    }
}

template<class T, typename... Args>
static std::unique_ptr<T> construct(Args &&...args, const sol::table &children)
{
    std::unique_ptr<T> item(new T(std::forward<Args>(args)..., {}));

    processChildren(item.get(), children);

    return item;
}

void addLayoutModule()
{
    LuaEngine::registerProvider("Layout", [](sol::state_view l) -> sol::object {
        sol::table layout = l.create_table();

        layout.new_usertype<LayoutItem>("LayoutItem", "attachTo", &LayoutItem::attachTo);

        layout["Span"] = [](int span, LayoutItem *item) {
            return createItem(item, Span(span, *item));
        };
        layout["Space"] = [](int space) { return createItem(nullptr, Space(space)); };
        layout["Stretch"] = [](int stretch) { return createItem(nullptr, Stretch(stretch)); };

        layout.new_usertype<Column>("Column",
                                    sol::call_constructor,
                                    sol::factories(&construct<Column>),
                                    sol::base_classes,
                                    sol::bases<LayoutItem>());
        layout.new_usertype<Row>("Row",
                                 sol::call_constructor,
                                 sol::factories(&construct<Row>),
                                 sol::base_classes,
                                 sol::bases<LayoutItem>());
        layout.new_usertype<Flow>("Flow",
                                  sol::call_constructor,
                                  sol::factories(&construct<Flow>),
                                  sol::base_classes,
                                  sol::bases<LayoutItem>());
        layout.new_usertype<Grid>("Grid",
                                  sol::call_constructor,
                                  sol::factories(&construct<Grid>),
                                  sol::base_classes,
                                  sol::bases<LayoutItem>());
        layout.new_usertype<Form>("Form",
                                  sol::call_constructor,
                                  sol::factories(&construct<Form>),
                                  sol::base_classes,
                                  sol::bases<LayoutItem>());
        layout.new_usertype<Widget>("Widget",
                                    sol::call_constructor,
                                    sol::factories(&construct<Widget>),
                                    sol::base_classes,
                                    sol::bases<LayoutItem>());
        layout.new_usertype<Stack>("Stack",
                                   sol::call_constructor,
                                   sol::factories(&construct<Stack>),
                                   sol::base_classes,
                                   sol::bases<LayoutItem>());
        layout.new_usertype<Tab>(
            "Tab",
            sol::call_constructor,
            sol::factories(&construct<Tab, QString>),
            sol::base_classes,
            sol::bases<LayoutItem>());
        layout.new_usertype<TextEdit>("TextEdit",
                                      sol::call_constructor,
                                      sol::factories(&construct<TextEdit>),
                                      sol::base_classes,
                                      sol::bases<LayoutItem>());
        layout.new_usertype<PushButton>("PushButton",
                                        sol::call_constructor,
                                        sol::factories(&construct<PushButton>),
                                        sol::base_classes,
                                        sol::bases<LayoutItem>());
        layout.new_usertype<SpinBox>("SpinBox",
                                     sol::call_constructor,
                                     sol::factories(&construct<SpinBox>),
                                     sol::base_classes,
                                     sol::bases<LayoutItem>());
        layout.new_usertype<Splitter>("Splitter",
                                      sol::call_constructor,
                                      sol::factories(&construct<Splitter>),
                                      sol::base_classes,
                                      sol::bases<LayoutItem>());
        layout.new_usertype<ToolBar>("ToolBar",
                                     sol::call_constructor,
                                     sol::factories(&construct<ToolBar>),
                                     sol::base_classes,
                                     sol::bases<LayoutItem>());
        layout.new_usertype<TabWidget>("TabWidget",
                                       sol::call_constructor,
                                       sol::factories(&construct<TabWidget>),
                                       sol::base_classes,
                                       sol::bases<LayoutItem>());

        layout.new_usertype<Group>("Group",
                                   sol::call_constructor,
                                   sol::factories(&construct<Group>),
                                   sol::base_classes,
                                   sol::bases<LayoutItem>());

        layout["br"] = &br;
        layout["st"] = &st;
        layout["empty"] = &empty;
        layout["hr"] = &hr;
        layout["noMargin"] = &noMargin;
        layout["normalMargin"] = &normalMargin;
        layout["customMargin"] = [](int left, int top, int right, int bottom) {
            return customMargin(QMargins(left, top, right, bottom));
        };
        layout["withFormAlignment"] = &withFormAlignment;
        layout["title"] = &title;
        layout["text"] = &text;
        layout["tooltip"] = &tooltip;
        layout["resize"] = &resize;
        layout["columnStretch"] = &columnStretch;
        layout["spacing"] = &spacing;
        layout["windowTitle"] = &windowTitle;
        layout["fieldGrowthPolicy"] = &fieldGrowthPolicy;
        layout["id"] = &id;
        layout["setText"] = &setText;
        layout["onClicked"] = [](const sol::function &f) {
            return onClicked([f]() {
                auto res = LuaEngine::void_safe_call(f);
                QTC_CHECK_EXPECTED(res);
            });
        };
        layout["onTextChanged"] = [](const sol::function &f) {
            return onTextChanged([f](const QString &text) {
                auto res = LuaEngine::void_safe_call(f, text);
                QTC_CHECK_EXPECTED(res);
            });
        };

        return layout;
    });
}

} // namespace Lua::Internal
