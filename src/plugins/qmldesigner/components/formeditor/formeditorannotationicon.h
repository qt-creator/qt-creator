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

#include <annotation.h>
#include <modelnode.h>

#include <QGraphicsItem>
#include <QGraphicsObject>
#include <QIcon>
#include <QObject>

namespace QmlDesigner {

class AnnotationEditorDialog;

class FormEditorAnnotationIcon : public QGraphicsObject
{
    Q_OBJECT

public:
    explicit FormEditorAnnotationIcon(const ModelNode &modelNode, QGraphicsItem *parent = nullptr);
    ~FormEditorAnnotationIcon() override;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    QRectF boundingRect() const override;

    qreal iconWidth();
    qreal iconHeight();

    bool isReaderActive();
    void setActive(bool readerStatus);

    void quickResetReader();
    void resetReader();

protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;

private:
    void drawReader();
    void hideReader();

    void createReader();
    void removeReader();
    QGraphicsItem *createCommentBubble(QRectF rect, const QString &title,
                                       const QString &author, const QString  &text,
                                       const QString &date, QGraphicsItem *parent);
    QGraphicsItem *createTitleBubble(const QRectF &rect, const QString &text, QGraphicsItem *parent);

    void createAnnotationEditor();
    void removeAnnotationDialog();

    void annotationDialogAccepted();
    void annotationDialogRejected();

private:
    ModelNode m_modelNode;
    bool m_readerIsActive;
    QString m_customId;
    Annotation m_annotation;
    AnnotationEditorDialog *m_annotationEditor;

    QString m_normalIconStr;
    QString m_activeIconStr;

    qreal m_iconW;
    qreal m_iconH;
};

} //QmlDesigner
