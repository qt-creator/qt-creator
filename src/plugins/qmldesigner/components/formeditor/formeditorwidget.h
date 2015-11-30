/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
****************************************************************************/

#ifndef FORMEDITORWIDGET_H
#define FORMEDITORWIDGET_H

#include <QWidget>
#include <QPointer>

QT_BEGIN_NAMESPACE
class QActionGroup;
QT_END_NAMESPACE

namespace QmlDesigner {

class ZoomAction;
class LineEditAction;
class BackgroundAction;
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
    QAction *showBoundingRectAction() const;
    QAction *snappingAction() const;
    QAction *snappingAndAnchoringAction() const;

    void setScene(FormEditorScene *scene);
    ToolBox *toolBox() const;

    double spacing() const;
    double containerPadding() const;

    QString contextHelpId() const;

    void setRootItemRect(const QRectF &rect);
    QRectF rootItemRect() const;

    void updateActions();

    void resetView();
    void centerScene();

    void setFocus();

protected:
    void wheelEvent(QWheelEvent *event);
    QActionGroup *toolActionGroup() const;

private slots:
    void changeTransformTool(bool checked);
    void setZoomLevel(double zoomLevel);
    void changeRootItemWidth(const QString &widthText);
    void changeRootItemHeight(const QString &heightText);
    void changeBackgound(const QColor &color);
    void resetNodeInstanceView();

private:
    QPointer<FormEditorView> m_formEditorView;
    QPointer<FormEditorGraphicsView> m_graphicsView;
    QPointer<ZoomAction> m_zoomAction;
    QPointer<ToolBox> m_toolBox;
    QPointer<QAction> m_transformToolAction;
    QPointer<QActionGroup> m_toolActionGroup;
    QPointer<QAction> m_snappingAction;
    QPointer<QAction> m_snappingAndAnchoringAction;
    QPointer<QAction> m_noSnappingAction;
    QPointer<QAction> m_showBoundingRectAction;
    QPointer<LineEditAction> m_rootWidthAction;
    QPointer<LineEditAction> m_rootHeightAction;
    QPointer<BackgroundAction> m_backgroundAction;
    QPointer<QAction> m_resetAction;
};


}
#endif //FORMEDITORWIDGET_H

