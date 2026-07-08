// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <sol/forward.hpp>

#include <QPointer>
#include <QWidget>

namespace Lua::Internal {
class LuaAspectContainer;
}

namespace Utils {
class AspectContainer;
class BoolAspect;
class ColorAspect;
class SelectionAspect;
class MultiSelectionAspect;
class StringAspect;
class FilePathAspect;
class IntegerAspect;
class DoubleAspect;
class StringListAspect;
class FilePathListAspect;
class IntegersAspect;
class StringSelectionAspect;
class ToggleAspect;
class TriStateAspect;
class TextDisplay;
class AspectList;
class BaseAspect;
} // namespace Utils

SOL_BASE_CLASSES(::Lua::Internal::LuaAspectContainer, Utils::AspectContainer, Utils::BaseAspect);

SOL_BASE_CLASSES(Utils::BoolAspect, Utils::BaseAspect);
SOL_BASE_CLASSES(Utils::ColorAspect, Utils::BaseAspect);
SOL_BASE_CLASSES(Utils::SelectionAspect, Utils::BaseAspect);
SOL_BASE_CLASSES(Utils::MultiSelectionAspect, Utils::BaseAspect);
SOL_BASE_CLASSES(Utils::StringAspect, Utils::BaseAspect);
SOL_BASE_CLASSES(Utils::FilePathAspect, Utils::BaseAspect);
SOL_BASE_CLASSES(Utils::IntegerAspect, Utils::BaseAspect);
SOL_BASE_CLASSES(Utils::DoubleAspect, Utils::BaseAspect);
SOL_BASE_CLASSES(Utils::StringListAspect, Utils::BaseAspect);
SOL_BASE_CLASSES(Utils::FilePathListAspect, Utils::BaseAspect);
SOL_BASE_CLASSES(Utils::IntegersAspect, Utils::BaseAspect);
SOL_BASE_CLASSES(Utils::StringSelectionAspect, Utils::BaseAspect);
SOL_BASE_CLASSES(Utils::ToggleAspect, Utils::BoolAspect, Utils::BaseAspect);
SOL_BASE_CLASSES(Utils::TriStateAspect, Utils::SelectionAspect, Utils::BaseAspect);
SOL_BASE_CLASSES(Utils::TextDisplay, Utils::BaseAspect);
SOL_BASE_CLASSES(Utils::AspectList, Utils::BaseAspect);

SOL_DERIVED_CLASSES(Utils::AspectContainer, Lua::Internal::LuaAspectContainer);

SOL_DERIVED_CLASSES(
    Utils::BaseAspect,
    Utils::AspectContainer,
    Utils::BoolAspect,
    Utils::ColorAspect,
    Utils::SelectionAspect,
    Utils::MultiSelectionAspect,
    Utils::StringAspect,
    Utils::FilePathAspect,
    Utils::IntegerAspect,
    Utils::DoubleAspect,
    Utils::StringListAspect,
    Utils::FilePathListAspect,
    Utils::IntegersAspect,
    Utils::StringSelectionAspect,
    Utils::ToggleAspect,
    Utils::TriStateAspect,
    Utils::TextDisplay,
    Utils::AspectList);

namespace Layouting {
class Object;

class Layout;
class Column;
class Flow;
class Form;
class Grid;
class Preview;
class Row;
} // namespace Layouting

namespace Lua::Internal {

class WidgetHandle
{
public:
    virtual ~WidgetHandle() = default;

    QPointer<QWidget> widget;
};

template<class Builder>
class Handle : public WidgetHandle
{};

} // namespace Lua::Internal

namespace sol {
template<>
struct derive<Lua::Internal::WidgetHandle> : std::true_type
{
    typedef types<> type;
};
template<class Builder>
struct base<Lua::Internal::Handle<Builder>> : std::true_type
{
    typedef types<Lua::Internal::WidgetHandle> type;
};
} // namespace sol

SOL_BASE_CLASSES(Layouting::Layout, Layouting::Object);
SOL_BASE_CLASSES(Layouting::Column, Layouting::Layout);
SOL_BASE_CLASSES(Layouting::Row, Layouting::Layout);
SOL_BASE_CLASSES(Layouting::Flow, Layouting::Layout);
SOL_BASE_CLASSES(Layouting::Grid, Layouting::Layout);
SOL_BASE_CLASSES(Layouting::Form, Layouting::Layout);
SOL_BASE_CLASSES(Layouting::Preview, Layouting::Column, Layouting::Layout, Layouting::Object);

SOL_DERIVED_CLASSES(
    Layouting::Object,
    Layouting::Layout,
    Layouting::Column,
    Layouting::Row,
    Layouting::Flow,
    Layouting::Grid,
    Layouting::Form,
    Layouting::Preview);
