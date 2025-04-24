// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace QmlDesigner {

enum DesignerWidgetFlags { DisableOnError, IgnoreErrors };

class WidgetInfo
{
public:
    enum PlacementHint {
        NoPane,
        LeftPane,
        RightPane,
        BottomPane,
        TopPane, // not used
        CentralPane
    };

    QString uniqueId;
    QString tabName;
    QString feedbackDisplayName;
    QWidget *widget = nullptr;
    PlacementHint placementHint = PlacementHint::NoPane;
    DesignerWidgetFlags widgetFlags = DesignerWidgetFlags::DisableOnError;
};

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::WidgetInfo)
