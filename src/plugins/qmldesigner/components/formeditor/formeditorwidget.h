/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
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
    ToolBox *toolBox() const;

    double spacing() const;
    double margins() const;

    void setFeedbackNode(const QmlItemNode &node);

    QString contextHelpId() const;

    void setRootItemRect(const QRectF &rect);
    QRectF rootItemRect() const;

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
    QWeakPointer<ToolBox> m_toolBox;
    QWeakPointer<QAction> m_anchorToolAction;
    QWeakPointer<QAction> m_transformToolAction;
    QWeakPointer<QActionGroup> m_toolActionGroup;
    QWeakPointer<QAction> m_snappingAction;
    QWeakPointer<QAction> m_snappingAndAnchoringAction;
    QWeakPointer<QAction> m_noSnappingAction;
    QWeakPointer<QAction> m_showBoundingRectAction;
    QWeakPointer<QAction> m_selectOnlyContentItemsAction;
};


}
#endif //FORMEDITORWIDGET_H

