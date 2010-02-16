/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef FORMEDITORWIDGET_H
#define FORMEDITORWIDGET_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QActionGroup;
QT_END_NAMESPACE

namespace QmlDesigner {

class ZoomAction;
class FormEditorView;
class FormEditorScene;
class FormEditorGraphicsView;
class ToolBox;
class NumberSeriesAction;
class QmlItemNode;


class FormEditorWidget : public QWidget
{
    Q_OBJECT
public:
    FormEditorWidget(FormEditorView *view);

    ZoomAction *zoomAction() const;
    QAction *anchorToolAction() const;
    QAction *transformToolAction() const;
    QAction *showBoundingRectAction() const;
    QAction *selectOnlyContentItemsAction() const;
    QAction *snappingAction() const;
    QAction *snappingAndAnchoringAction() const;

    void setScene(FormEditorScene *scene);
    ToolBox *lowerToolBox() const;

    double spacing() const;
    double margins() const;

    void setFeedbackNode(const QmlItemNode &node);


protected:
    void enterEvent(QEvent *event);
    void wheelEvent(QWheelEvent *event);
    QActionGroup *toolActionGroup() const;

private slots:
    void changeTransformTool(bool checked);
    void changeAnchorTool(bool checked);
    void setZoomLevel(double zoomLevel);

private:
    QWeakPointer<FormEditorView> m_formEditorView;
    QWeakPointer<FormEditorGraphicsView> m_graphicsView;
    QWeakPointer<ZoomAction> m_zoomAction;
    QWeakPointer<QAction> m_anchorToolAction;
    QWeakPointer<QAction> m_transformToolAction;
    QWeakPointer<QActionGroup> m_toolActionGroup;
    QWeakPointer<ToolBox> m_lowerToolBox;
    QWeakPointer<QAction> m_snappingAction;
    QWeakPointer<QAction> m_snappingAndAnchoringAction;
    QWeakPointer<QAction> m_noSnappingAction;
    QWeakPointer<NumberSeriesAction> m_snappingMarginAction;
    QWeakPointer<NumberSeriesAction> m_snappingSpacingAction;
    QWeakPointer<QAction> m_showBoundingRectAction;
    QWeakPointer<QAction> m_selectOnlyContentItemsAction;
};


}
#endif //FORMEDITORWIDGET_H

