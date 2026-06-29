// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QGraphicsItem>

namespace QmlDesigner::TransitionEditorConstants {

inline constexpr int sectionWidth = 200;

inline constexpr int transitionEditorSectionItemUserType = QGraphicsItem::UserType + 6;
inline constexpr int transitionEditorPropertyItemUserType = QGraphicsItem::UserType + 7;

inline constexpr char C_QMLTRANSITIONS[] = "QmlDesigner::Transitions";

inline constexpr char C_SETTINGS[] = "QmlDesigner.Transitions.Settings";
inline constexpr char C_CURVE_PICKER[] = "QmlDesigner.Transitions.CurvePicker";
inline constexpr char C_ZOOM_IN[] = "QmlDesigner.Transitions.ZoomIn";
inline constexpr char C_ZOOM_OUT[] = "QmlDesigner.Transitions.ZoomOut";

} // namespace QmlDesigner::TransitionEditorConstants
