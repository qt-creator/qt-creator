/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Design Tooling
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

#include "curveeditor.h"
#include "curveeditormodel.h"
#include "detail/curveitem.h"
#include "detail/graphicsview.h"
#include "detail/treeview.h"

#include <QHBoxLayout>
#include <QSplitter>

namespace DesignTools {

CurveEditor::CurveEditor(CurveEditorModel *model, QWidget *parent)
    : QWidget(parent)
    , m_tree(new TreeView(model, this))
    , m_view(new GraphicsView(model))
{
    QSplitter *splitter = new QSplitter;
    splitter->addWidget(m_tree);
    splitter->addWidget(m_view);
    splitter->setStretchFactor(1, 2);

    QHBoxLayout *box = new QHBoxLayout;
    box->addWidget(splitter);
    setLayout(box);

    connect(m_tree, &TreeView::curvesSelected, m_view, &GraphicsView::reset);
}

void CurveEditor::zoomX(double zoom)
{
    m_view->setZoomX(zoom);
}

void CurveEditor::zoomY(double zoom)
{
    m_view->setZoomY(zoom);
}

} // End namespace DesignTools.
