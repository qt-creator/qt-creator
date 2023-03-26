// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QGraphicsItem>

namespace QmlDesigner {
namespace TimelineConstants {
const int sectionHeight = 18;
const int rulerHeight = sectionHeight + 4;
const int sectionWidth = 200;
const int moveableAbstractItemUserType = QGraphicsItem::UserType + 1;
const int timelineSectionItemUserType = QGraphicsItem::UserType + 2;
const int timelinePropertyItemUserType = QGraphicsItem::UserType + 3;
const int textIndentationProperties = 54;
const int textIndentationSections = 24;
const int toolButtonSize = 11;
const int timelineBounds = 8;
const int timelineLeftOffset = 10;

const char timelineCategory[] = "Timeline";
const char timelineCategoryDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Timeline");

const char timelineCopyKeyframesDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                                  "Copy All Keyframes");
const char timelinePasteKeyframesDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                                   "Paste Keyframes");
const char timelineInsertKeyframesDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                                    "Add Keyframe");
const char timelineDeleteKeyframesDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                                    "Delete All Keyframes");

const char statusBarPlayheadFrame[] = QT_TRANSLATE_NOOP("QmlDesignerTimeline", "Playhead frame %1");
const char statusBarKeyframe[]      = QT_TRANSLATE_NOOP("QmlDesignerTimeline", "Keyframe %1");

const char C_QMLTIMELINE[] = "QmlDesigner::Timeline";
const char C_SETTINGS[] = "QmlDesigner.Settings";
const char C_ADD_TIMELINE[] = "QmlDesigner.AddTimeline";
const char C_TO_START[] = "QmlDesigner.ToStart";
const char C_TO_END[] = "QmlDesigner.ToEnd";
const char C_PREVIOUS[] = "QmlDesigner.Previous";
const char C_PLAY[] = "QmlDesigner.Play";
const char C_LOOP_PLAYBACK[] = "QmlDesigner.LoopPlayback";
const char C_NEXT[] = "QmlDesigner.Next";
const char C_AUTO_KEYFRAME[] = "QmlDesigner.AutoKeyframe";
const char C_CURVE_PICKER[] = "QmlDesigner.CurvePicker";
const char C_CURVE_EDITOR[] = "QmlDesigner.CurveEditor";
const char C_ZOOM_IN[] = "QmlDesigner.ZoomIn";
const char C_ZOOM_OUT[] = "QmlDesigner.ZoomOut";

const int keyFrameSize = 17;
const int keyFrameMargin = 2;
} // namespace TimelineConstants
} // namespace QmlDesigner
