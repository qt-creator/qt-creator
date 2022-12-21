// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QTreeView>

namespace QmlDesigner {

class PreviewToolTip;

class NavigatorTreeView : public QTreeView
{
    Q_OBJECT

public:
    NavigatorTreeView(QWidget *parent = nullptr);
    static void drawSelectionBackground(QPainter *painter, const QStyleOption &option);
    bool viewportEvent(QEvent *event) override;

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void startDrag(Qt::DropActions supportedActions) override;

private:
    PreviewToolTip *m_previewToolTip = nullptr;
    qint32 m_previewToolTipNodeId = -1;
    bool m_dragAllowed = true;
};
}
