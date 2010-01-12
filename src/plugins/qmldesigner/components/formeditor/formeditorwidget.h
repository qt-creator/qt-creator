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

class QActionGroup;

namespace QmlDesigner {

class ZoomAction;
class FormEditorView;
class FormEditorScene;
class FormEditorGraphicsView;
class ToolBox;
class NumberSeriesAction;

class FormEditorWidget : public QWidget
{
    Q_OBJECT
public:
    FormEditorWidget(FormEditorView *view);
    bool isSnapButtonChecked() const;

    ZoomAction *zoomAction() const;
    QAction *anchorToolAction() const;
    QAction *transformToolAction() const;

    void setScene(FormEditorScene *scene);
    ToolBox *toolBox() const;

    double spacing() const;
    double margins() const;


protected:
    void enterEvent(QEvent *event);
    void wheelEvent(QWheelEvent *event);
    QActionGroup *toolActionGroup() const;

private slots:
    void changeTransformTool(bool checked);
    void changeAnchorTool(bool checked);
    void setZoomLevel(double zoomLevel);
    void changeSnappingTool(bool checked);

private:
    QWeakPointer<FormEditorView> m_formEditorView;
    QWeakPointer<FormEditorGraphicsView> m_graphicsView;
    QWeakPointer<ZoomAction> m_zoomAction;
    QWeakPointer<QAction> m_anchorToolAction;
    QWeakPointer<QAction> m_transformToolAction;
    QWeakPointer<QActionGroup> m_toolActionGroup;
    QWeakPointer<ToolBox> m_toolBox;
    QWeakPointer<QAction> m_snappingToolAction;
    QWeakPointer<NumberSeriesAction> m_snappingMarginAction;
    QWeakPointer<NumberSeriesAction> m_snappingSpacingAction;
};


}
#endif //FORMEDITORWIDGET_H

