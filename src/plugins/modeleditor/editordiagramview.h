// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmt/diagram_widgets_ui/diagramview.h"
#include <utils/dropsupport.h>

namespace ModelEditor {
namespace Internal {

class PxNodeController;

class EditorDiagramView :
        public qmt::DiagramView
{
    Q_OBJECT
    class EditorDiagramViewPrivate;

public:
    explicit EditorDiagramView(QWidget *parent = nullptr);
    ~EditorDiagramView() override;

signals:
    void zoomIn(const QPoint &zoomOrigin);
    void zoomOut(const QPoint &zoomOrigin);

public:
    void setPxNodeController(PxNodeController *pxNodeController);

protected:
    void wheelEvent(QWheelEvent *wheelEvent) override;

private:
    void dropProjectExplorerNodes(const QList<QVariant> &values, const QPoint &pos);
    void dropFiles(const QList<Utils::DropSupport::FileSpec> &files, const QPoint &pos);

    EditorDiagramViewPrivate *d;
};

} // namespace Internal
} // namespace ModelEditor
