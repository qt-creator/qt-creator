// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QGraphicsItem>

namespace QmlDesigner {
namespace TransitionEditorConstants {

const int sectionWidth = 200;

const int transitionEditorSectionItemUserType = QGraphicsItem::UserType + 6;
const int transitionEditorPropertyItemUserType = QGraphicsItem::UserType + 7;

const char C_QMLTRANSITIONS[] = "QmlDesigner::Transitions";

const char C_SETTINGS[] = "QmlDesigner.Transitions.Settings";
const char C_CURVE_PICKER[] = "QmlDesigner.Transitions.CurvePicker";
const char C_ZOOM_IN[] = "QmlDesigner.Transitions.ZoomIn";
const char C_ZOOM_OUT[] = "QmlDesigner.Transitions.ZoomOut";

} // namespace TransitionEditorConstants
} // namespace QmlDesigner
