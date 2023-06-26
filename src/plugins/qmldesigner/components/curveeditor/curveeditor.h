// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QToolBar>
#include <QWidget>
#include <QLabel>

namespace QmlDesigner {

class CurveEditorModel;
class CurveEditorToolBar;
class GraphicsView;
class TreeView;

class CurveEditor : public QWidget
{
    Q_OBJECT

public:
    CurveEditor(CurveEditorModel *model, QWidget *parent = nullptr);

    bool dragging() const;

    void zoomX(double zoom);

    void zoomY(double zoom);

    void clearCanvas();

signals:
    void viewEnabledChanged(const bool);

protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

private:
    void updateStatusLine();
    void updateMcuState();

    QLabel *m_infoText;

    QLabel *m_statusLine;

    CurveEditorToolBar *m_toolbar;

    TreeView *m_tree;

    GraphicsView *m_view;
};

} // End namespace QmlDesigner.
