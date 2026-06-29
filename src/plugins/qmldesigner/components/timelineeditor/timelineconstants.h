// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QGraphicsItem>

namespace QmlDesigner::TimelineConstants {
inline constexpr int sectionHeight = 18;
inline constexpr int rulerHeight = sectionHeight + 4;
inline constexpr int sectionWidth = 200;
inline constexpr int moveableAbstractItemUserType = QGraphicsItem::UserType + 1;
inline constexpr int timelineSectionItemUserType = QGraphicsItem::UserType + 2;
inline constexpr int timelinePropertyItemUserType = QGraphicsItem::UserType + 3;
inline constexpr int textIndentationProperties = 54;
inline constexpr int textIndentationSections = 24;
inline constexpr int toolButtonSize = 11;
inline constexpr int timelineBounds = 8;
inline constexpr int timelineLeftOffset = 10;

inline constexpr char timelineCategory[] = "Timeline";
inline constexpr char timelineCategoryDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Timeline");

inline constexpr char timelineCopyKeyframesDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                                  "Copy All Keyframes");
inline constexpr char timelinePasteKeyframesDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                                   "Paste Keyframes");
inline constexpr char timelineInsertKeyframesDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                                    "Add Keyframe");
inline constexpr char timelineDeleteKeyframesDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                                    "Delete All Keyframes");

inline constexpr char statusBarPlayheadFrame[] = QT_TRANSLATE_NOOP("QmlDesignerTimeline", "Playhead frame %1");
inline constexpr char statusBarKeyframe[]      = QT_TRANSLATE_NOOP("QmlDesignerTimeline", "Keyframe %1");

inline constexpr char C_QMLTIMELINE[] = "QmlDesigner::Timeline";
inline constexpr char C_SETTINGS[] = "QmlDesigner.Settings";
inline constexpr char C_ADD_TIMELINE[] = "QmlDesigner.AddTimeline";
inline constexpr char C_TO_START[] = "QmlDesigner.ToStart";
inline constexpr char C_TO_END[] = "QmlDesigner.ToEnd";
inline constexpr char C_PREVIOUS[] = "QmlDesigner.Previous";
inline constexpr char C_PLAY[] = "QmlDesigner.Play";
inline constexpr char C_LOOP_PLAYBACK[] = "QmlDesigner.LoopPlayback";
inline constexpr char C_NEXT[] = "QmlDesigner.Next";
inline constexpr char C_AUTO_KEYFRAME[] = "QmlDesigner.AutoKeyframe";
inline constexpr char C_CURVE_PICKER[] = "QmlDesigner.CurvePicker";
inline constexpr char C_CURVE_EDITOR[] = "QmlDesigner.CurveEditor";
inline constexpr char C_ZOOM_IN[] = "QmlDesigner.ZoomIn";
inline constexpr char C_ZOOM_OUT[] = "QmlDesigner.ZoomOut";

inline constexpr int keyFrameSize = 17;
inline constexpr int keyFrameMargin = 2;
} // namespace QmlDesigner::TimelineConstants
