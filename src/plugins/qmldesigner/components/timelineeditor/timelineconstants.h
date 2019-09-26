/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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
const int priorityTimelineCategory = 110;
const char timelineCategoryDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Timeline");

const char timelineCopyKeyframesDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                                  "Copy All Keyframes");
const char timelinePasteKeyframesDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                                   "Paste Keyframes");
const char timelineInsertKeyframesDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                                    "Add Keyframes at Current Frame");
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
const char C_NEXT[] = "QmlDesigner.Next";
const char C_AUTO_KEYFRAME[] = "QmlDesigner.AutoKeyframe";
const char C_CURVE_PICKER[] = "QmlDesigner.CurvePicker";
const char C_CURVE_EDITOR[] = "QmlDesigner.CurveEditor";
const char C_ZOOM_IN[] = "QmlDesigner.ZoomIn";
const char C_ZOOM_OUT[] = "QmlDesigner.ZoomOut";

const char C_BAR_ITEM_OVERRIDE[] = "Timeline.OverrideColor";

const int keyFrameSize = 17;
const int keyFrameMargin = 2;
} // namespace TimelineConstants
} // namespace QmlDesigner
