// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
