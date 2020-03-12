/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "itemlibraryinfo.h"

#include <QtWidgets/qwidget.h>
#include <QtGui/qimage.h>
#include <QtGui/qevent.h>
#include <QtCore/qpointer.h>

namespace QmlDesigner {

class Edit3DWidget;

class Edit3DCanvas : public QWidget
{
    Q_OBJECT

public:
    Edit3DCanvas(Edit3DWidget *parent);

    void updateRenderImage(const QImage &img);
    void updateActiveScene(qint32 activeScene);

    qint32 activeScene() const { return m_activeScene; }

protected:
    void mousePressEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void mouseDoubleClickEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void wheelEvent(QWheelEvent *e) override;
    void paintEvent(QPaintEvent *e) override;
    void resizeEvent(QResizeEvent *e) override;
    void dragEnterEvent(QDragEnterEvent *e) override;
    void dropEvent(QDropEvent *e) override;

private:
    QPointer<Edit3DWidget> m_parent;
    QImage m_image;
    qint32 m_activeScene = -1;
    ItemLibraryEntry m_itemLibraryEntry;
};

} // namespace QmlDesigner
