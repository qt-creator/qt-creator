/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
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

#include "qmt/diagram_widgets_ui/diagramview.h"
#include "utils/dropsupport.h"

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
    void zoomIn();
    void zoomOut();

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
