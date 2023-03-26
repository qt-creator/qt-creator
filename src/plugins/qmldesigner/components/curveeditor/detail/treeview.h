// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "curveitem.h"
#include "selectionmodel.h"

#include <QTreeView>

namespace QmlDesigner {

class AnimationCurve;
class CurveEditorModel;

struct CurveEditorStyle;

class TreeView : public QTreeView
{
    Q_OBJECT

signals:
    void curvesSelected(const std::vector<CurveItem *> &curves);

    void treeItemLocked(TreeItem *item, bool val);

    void treeItemPinned(TreeItem *item, bool val);

public:
    TreeView(CurveEditorModel *model, QWidget *parent = nullptr);

    SelectionModel *selectionModel() const;

    void changeCurve(unsigned int id, const AnimationCurve &curve);

    void setStyle(const CurveEditorStyle &style);

protected:
    QSize sizeHint() const override;

    void mousePressEvent(QMouseEvent *event) override;
};

} // End namespace QmlDesigner.
