/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef FORMEDITORWIDGET_H
#define FORMEDITORWIDGET_H

#include <QWidget>

#include "formeditorcrumblebar.h"

QT_BEGIN_NAMESPACE
class QActionGroup;
QT_END_NAMESPACE

namespace QmlDesigner {

class ZoomAction;
class LineEditAction;
class FormEditorView;
class FormEditorScene;
class FormEditorGraphicsView;
class ToolBox;
class QmlItemNode;


class FormEditorWidget : public QWidget
{
    Q_OBJECT
public:
    FormEditorWidget(FormEditorView *view);

    ZoomAction *zoomAction() const;
    QAction *transformToolAction() const;
    QAction *showBoundingRectAction() const;
    QAction *selectOnlyContentItemsAction() const;
    QAction *snappingAction() const;
    QAction *snappingAndAnchoringAction() const;

    void setScene(FormEditorScene *scene);
    ToolBox *toolBox() const;

    double spacing() const;
    double margins() const;

    QString contextHelpId() const;

    void setRootItemRect(const QRectF &rect);
    QRectF rootItemRect() const;

    void updateActions();

    void resetView();
    void centerScene();

    void setFocus();

    FormEditorCrumbleBar *formEditorCrumbleBar() const;

protected:
    void wheelEvent(QWheelEvent *event);
    QActionGroup *toolActionGroup() const;

private slots:
    void changeTransformTool(bool checked);
    void setZoomLevel(double zoomLevel);
    void changeRootItemWidth(const QString &widthText);
    void changeRootItemHeight(const QString &heightText);
    void resetNodeInstanceView();

private:
    QWeakPointer<FormEditorView> m_formEditorView;
    QWeakPointer<FormEditorGraphicsView> m_graphicsView;
    QWeakPointer<ZoomAction> m_zoomAction;
    QWeakPointer<ToolBox> m_toolBox;
    QWeakPointer<QAction> m_transformToolAction;
    QWeakPointer<QActionGroup> m_toolActionGroup;
    QWeakPointer<QAction> m_snappingAction;
    QWeakPointer<QAction> m_snappingAndAnchoringAction;
    QWeakPointer<QAction> m_noSnappingAction;
    QWeakPointer<QAction> m_showBoundingRectAction;
    QWeakPointer<QAction> m_selectOnlyContentItemsAction;
    QWeakPointer<LineEditAction> m_rootWidthAction;
    QWeakPointer<LineEditAction> m_rootHeightAction;
    QWeakPointer<QAction> m_resetAction;
};


}
#endif //FORMEDITORWIDGET_H

