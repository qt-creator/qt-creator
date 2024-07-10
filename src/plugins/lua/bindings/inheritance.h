// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <sol/forward.hpp>

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

class Widget;
class Group;
class PushButton;
class Row;
class SpinBox;
class Splitter;
class Stack;
class Tab;
class TabWidget;
class TextEdit;
class ToolBar;
} // namespace Layouting

SOL_BASE_CLASSES(Layouting::Layout, Layouting::Object);
SOL_BASE_CLASSES(Layouting::Column, Layouting::Layout);
SOL_BASE_CLASSES(Layouting::Row, Layouting::Layout);
SOL_BASE_CLASSES(Layouting::Flow, Layouting::Layout);
SOL_BASE_CLASSES(Layouting::Grid, Layouting::Layout);
SOL_BASE_CLASSES(Layouting::Form, Layouting::Layout);
SOL_BASE_CLASSES(Layouting::Widget, Layouting::Object);
SOL_BASE_CLASSES(Layouting::Stack, Layouting::Widget);
SOL_BASE_CLASSES(Layouting::Tab, Layouting::Widget);
SOL_BASE_CLASSES(Layouting::Group, Layouting::Widget);
SOL_BASE_CLASSES(Layouting::TextEdit, Layouting::Widget);
SOL_BASE_CLASSES(Layouting::PushButton, Layouting::Widget);
SOL_BASE_CLASSES(Layouting::SpinBox, Layouting::Widget);
SOL_BASE_CLASSES(Layouting::Splitter, Layouting::Widget);
SOL_BASE_CLASSES(Layouting::ToolBar, Layouting::Widget);
SOL_BASE_CLASSES(Layouting::TabWidget, Layouting::Widget);

SOL_DERIVED_CLASSES(
    Layouting::Object,
    Layouting::Layout,
    Layouting::Column,
    Layouting::Row,
    Layouting::Flow,
    Layouting::Grid,
    Layouting::Form,
    Layouting::Widget,
    Layouting::Stack,
    Layouting::Tab,
    Layouting::Group,
    Layouting::TextEdit,
    Layouting::PushButton,
    Layouting::SpinBox,
    Layouting::Splitter,
    Layouting::ToolBar,
    Layouting::TabWidget);

