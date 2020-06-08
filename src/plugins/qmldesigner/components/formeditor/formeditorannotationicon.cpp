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

#include "formeditorannotationicon.h"

#include <QGraphicsSceneMouseEvent>
#include <QGraphicsLinearLayout>
#include <QPainter>
#include <QAction>
#include <QMenu>
#include <QMessageBox>
#include <QTextDocument>

#include <coreplugin/icore.h>
#include <utils/theme/theme.h>
#include <utils/stylehelper.h>
#include <annotationeditor/annotationeditordialog.h>
#include <formeditorscene.h>


namespace QmlDesigner {

const int penWidth = 2;

FormEditorAnnotationIcon::FormEditorAnnotationIcon(const ModelNode &modelNode, QGraphicsItem *parent)
    : QGraphicsObject(parent)
    , m_modelNode(modelNode)
    , m_readerIsActive(false)
    , m_customId(modelNode.customId())
    , m_annotation(modelNode.annotation())
    , m_annotationEditor(nullptr)
    , m_normalIconStr(":icon/layout/annotationsIcon.png")
    , m_activeIconStr(":icon/layout/annotationsIconActive.png")
    , m_iconW(40)
    , m_iconH(32)
{
    setAcceptHoverEvents(true);

    bool hasAuxData = modelNode.hasAnnotation() || modelNode.hasCustomId();

    setEnabled(hasAuxData);
    setVisible(hasAuxData);

    FormEditorScene *scene = qobject_cast<FormEditorScene*>(parentItem()->scene());
    if (scene) {
        m_readerIsActive = scene->annotationVisibility();
        if (m_readerIsActive) {
            createReader();
        }
    }

    setToolTip(tr("Annotation"));
    setCursor(Qt::ArrowCursor);
}

FormEditorAnnotationIcon::~FormEditorAnnotationIcon()
{
    if (m_annotationEditor) {
        m_annotationEditor->deleteLater();
    }
}

void FormEditorAnnotationIcon::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    painter->setPen(Qt::NoPen);

    if (!isEnabled())
        setOpacity(0.5);

    bool hasAuxData = m_modelNode.hasAnnotation() || m_modelNode.hasCustomId();

    if (hasAuxData) {
        FormEditorScene *scene = qobject_cast<FormEditorScene*>(parentItem()->scene());
        if (scene)
            m_readerIsActive = scene->annotationVisibility();

        QPixmap icon(m_readerIsActive ? m_activeIconStr : m_normalIconStr);

        painter->drawPixmap(0, 0,
                            static_cast<int>(m_iconW), static_cast<int>(m_iconH),
                            icon);

        m_customId = m_modelNode.customId();
        m_annotation = m_modelNode.annotation();

        if (m_readerIsActive)
            drawReader();
        else
            hideReader();
    }
    else {
        removeReader();
    }

    setEnabled(hasAuxData);
    setVisible(hasAuxData);

    painter->restore();
}

QRectF FormEditorAnnotationIcon::boundingRect() const
{
    return QRectF(0, 0, m_iconW, m_iconH);
}

qreal FormEditorAnnotationIcon::iconWidth()
{
    return m_iconW;
}

qreal FormEditorAnnotationIcon::iconHeight()
{
    return m_iconH;
}

bool FormEditorAnnotationIcon::isReaderActive()
{
    return m_readerIsActive;
}

void FormEditorAnnotationIcon::setActive(bool readerStatus)
{
    m_readerIsActive = readerStatus;

    if (m_readerIsActive)
        resetReader();
    else
        removeReader();

    update();
}

void FormEditorAnnotationIcon::hoverEnterEvent(QGraphicsSceneHoverEvent * event)
{
    QGraphicsItem::hoverEnterEvent(event);
    event->accept();
    update();
}

void FormEditorAnnotationIcon::hoverLeaveEvent(QGraphicsSceneHoverEvent * event)
{
    QGraphicsItem::hoverLeaveEvent(event);
    event->accept();
    update();
}

void FormEditorAnnotationIcon::hoverMoveEvent(QGraphicsSceneHoverEvent * event)
{
    QGraphicsItem::hoverMoveEvent(event);
}

void FormEditorAnnotationIcon::mousePressEvent(QGraphicsSceneMouseEvent * event)
{
    event->accept();
    Qt::MouseButton button = event->button();

    if (button == Qt::LeftButton) {
        if (m_readerIsActive) {
            removeReader();
            m_readerIsActive = false;
        } else {
            resetReader();
            m_readerIsActive = true;
        }
    }

    FormEditorScene *scene = qobject_cast<FormEditorScene*>(parentItem()->scene());
    if (scene)
        scene->setAnnotationVisibility(m_readerIsActive);

    update();
}

void FormEditorAnnotationIcon::mouseReleaseEvent(QGraphicsSceneMouseEvent * event)
{
    event->accept();
}

void FormEditorAnnotationIcon::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
    QMenu menu;
    menu.addAction(tr("Edit Annotation"), [this]() {
        createAnnotationEditor();
    });

    menu.addAction(tr("Remove Annotation"), [this]() {
        removeAnnotationDialog();
    });

    menu.exec(event->screenPos());
    event->accept();
}

void FormEditorAnnotationIcon::drawReader()
{
    if (!childItems().isEmpty()) {
        for (QGraphicsItem *item : childItems()) {
            item->show();
        }
    }
    else {
        createReader();
    }
}

void FormEditorAnnotationIcon::hideReader()
{
    if (!childItems().isEmpty()) {
        for (QGraphicsItem *item : childItems()) {
            item->hide();
        }
    }
}

void FormEditorAnnotationIcon::quickResetReader()
{
    hideReader();
    drawReader();
}

void FormEditorAnnotationIcon::resetReader()
{
    removeReader();
    createReader();
}

void FormEditorAnnotationIcon::createReader()
{
    const qreal width = 290;
    const qreal height = 200;
    const qreal offset = 5;

    const QRectF titleRect(0, 0, width, 30);
    const QPointF cornerPosition(m_iconW + offset, 0);

    QGraphicsItem *titleBubble = createTitleBubble(titleRect, m_customId, this);
    titleBubble->setPos(cornerPosition);

    if (m_annotation.hasComments()) {
        QList<QGraphicsItem*> comments;

        QPointF commentPosition(cornerPosition.x(), 40);
        QRectF commentRect(0, 0, width, height);

        for (const Comment &comment : m_annotation.comments()) {
            QGraphicsItem *commentBubble = createCommentBubble(commentRect, comment.title(),
                                                               comment.author(), comment.deescapedText(),
                                                               comment.timestampStr(), this);
            comments.push_back(commentBubble);
        }


        const qreal maxHeight = 650;
        const QPointF commentsStartPosition(cornerPosition.x(), cornerPosition.y() + titleRect.height() + (offset*2));
        QPointF newPos(commentsStartPosition);
        qreal columnHeight = commentsStartPosition.y();

        for (QGraphicsItem *comment : comments) {

            comment->setPos(newPos); //first place comment in its new position, then calculate position for next comment

            const qreal itemHeight = comment->boundingRect().height();
            const qreal itemWidth = comment->boundingRect().width();

            const qreal possibleHeight = columnHeight + offset + itemHeight;
            qreal newX = 0;

            if ((itemWidth > (width + penWidth)) || (possibleHeight > maxHeight)) {
                //move coords to the new column
                columnHeight = commentsStartPosition.y();
                newX = newPos.x() + offset + itemWidth;
            }
            else {
                //move coords lower in the same column
                columnHeight += itemHeight + offset;
                newX = newPos.x();
            }

            newPos = { newX, columnHeight };
        }
    }
}

void FormEditorAnnotationIcon::removeReader()
{
    if (!childItems().isEmpty())
        qDeleteAll(childItems());
}

QGraphicsItem *FormEditorAnnotationIcon::createCommentBubble(QRectF rect, const QString &title,
                                                             const QString &author, const QString &text,
                                                             const QString &date, QGraphicsItem *parent)
{
    static QColor textColor = Utils::creatorTheme()->color(Utils::Theme::QmlDesigner_FormEditorForegroundColor);
    static QColor backgroundColor = Utils::creatorTheme()->color(Utils::Theme::QmlDesigner_BackgroundColorDarker);
    static QColor frameColor = Utils::creatorTheme()->color(Utils::Theme::QmlDesigner_BackgroundColor);
    QFont font;
    font.setBold(true);

    QGraphicsRectItem *frameItem = new QGraphicsRectItem(rect, parent);

    QGraphicsTextItem *titleItem = new QGraphicsTextItem(frameItem);
    titleItem->setPlainText(title);
    titleItem->setFont(font);
    titleItem->setDefaultTextColor(textColor);
    titleItem->setTextWidth(rect.width());
    titleItem->update();

    QGraphicsTextItem *authorItem = new QGraphicsTextItem(frameItem);
    authorItem->setPlainText(tr("By: ") + author);
    authorItem->setDefaultTextColor(textColor);
    authorItem->setTextWidth(rect.width());
    authorItem->setPos(titleItem->x(), titleItem->boundingRect().height() + titleItem->y());
    authorItem->update();

    QGraphicsTextItem *textItem = new QGraphicsTextItem(frameItem);
    textItem->setHtml(text);
    textItem->setDefaultTextColor(textColor);
    textItem->setTextWidth(rect.width());
    textItem->setPos(authorItem->x(), authorItem->boundingRect().height() + authorItem->y() + 5);
    textItem->update();

    if (textItem->boundingRect().width() > textItem->textWidth()) {
        textItem->setTextWidth(textItem->boundingRect().width());
        textItem->update();

        rect.setWidth(textItem->boundingRect().width());
    }

    const qreal contentRect = titleItem->boundingRect().height()
            + authorItem->boundingRect().height()
            + textItem->boundingRect().height();

    if ((contentRect + 60) > rect.height())
        rect.setHeight(contentRect+60);

    frameItem->setRect(rect);

    QGraphicsTextItem *dateItem = new QGraphicsTextItem(frameItem);
    dateItem->setPlainText(tr("Edited: ") + date);
    dateItem->setDefaultTextColor(textColor);
    dateItem->setTextWidth(rect.width());
    dateItem->setPos(frameItem->boundingRect().bottomLeft() + QPointF(0, -30));
    dateItem->update();

    QPen pen;
    pen.setCosmetic(true);
    pen.setWidth(penWidth);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::BevelJoin);
    pen.setColor(frameColor);

    frameItem->setPen(pen); //outline
    frameItem->setBrush(backgroundColor); //back
    frameItem->update();

    return frameItem;
}

QGraphicsItem *FormEditorAnnotationIcon::createTitleBubble(const QRectF &rect, const QString &text, QGraphicsItem *parent)
{
    static QColor textColor = Utils::creatorTheme()->color(Utils::Theme::QmlDesigner_FormEditorForegroundColor);
    static QColor backgroundColor = Utils::creatorTheme()->color(Utils::Theme::QmlDesigner_BackgroundColorDarker);
    static QColor frameColor = Utils::creatorTheme()->color(Utils::Theme::QmlDesigner_BackgroundColor);
    QFont font;
    font.setBold(true);

    QGraphicsRectItem *frameItem = new QGraphicsRectItem(rect, parent);
    QGraphicsTextItem *titleItem = new QGraphicsTextItem(text, frameItem);

    titleItem->setDefaultTextColor(textColor);
    titleItem->setFont(font);
    titleItem->update();

    if (titleItem->boundingRect().width() > rect.width()) {
        frameItem->setRect(QRectF(rect.x(), rect.y(),
                                  titleItem->boundingRect().width(), rect.height()));
    }

    QPen pen;
    pen.setCosmetic(true);
    pen.setWidth(2);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::BevelJoin);
    pen.setColor(frameColor);

    frameItem->setPen(pen); //outline
    frameItem->setBrush(backgroundColor); //back
    frameItem->update();

    return frameItem;
}

void FormEditorAnnotationIcon::createAnnotationEditor()
{
    if (m_annotationEditor) {
        m_annotationEditor->close();
        m_annotationEditor->deleteLater();
        m_annotationEditor = nullptr;
    }

    m_annotationEditor = new AnnotationEditorDialog(Core::ICore::dialogParent(),
                                                    m_modelNode.displayName(),
                                                    m_modelNode.customId(),
                                                    m_modelNode.annotation());

    connect(m_annotationEditor, &AnnotationEditorDialog::accepted,
            this, &FormEditorAnnotationIcon::annotationDialogAccepted);
    connect(m_annotationEditor, &QDialog::rejected,
            this, &FormEditorAnnotationIcon::annotationDialogRejected);

    m_annotationEditor->show();
    m_annotationEditor->raise();
}

void FormEditorAnnotationIcon::removeAnnotationDialog()
{
    QString dialogTitle = tr("Annotation");
    if (!m_customId.isNull()) {
        dialogTitle = m_customId;
    }
    QPointer<QMessageBox> deleteDialog = new QMessageBox(Core::ICore::dialogParent());
    deleteDialog->setWindowTitle(dialogTitle);
    deleteDialog->setText(tr("Delete this annotation?"));
    deleteDialog->setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    deleteDialog->setDefaultButton(QMessageBox::Yes);

    int result = deleteDialog->exec();
    if (deleteDialog)
        deleteDialog->deleteLater();

    if (result == QMessageBox::Yes) {
        m_modelNode.removeCustomId();
        m_modelNode.removeAnnotation();
        update();
    }
}

void FormEditorAnnotationIcon::annotationDialogAccepted()
{
    if (m_annotationEditor) {
        QString customId = m_annotationEditor->customId();
        m_customId = customId;
        m_modelNode.setCustomId(customId);

        Annotation annotation = m_annotationEditor->annotation();

        if (annotation.comments().isEmpty())
            m_modelNode.removeAnnotation();
        else
            m_modelNode.setAnnotation(annotation);

        m_annotation = annotation;

        m_annotationEditor->close();
        m_annotationEditor->deleteLater();
    }

    m_annotationEditor = nullptr;

    if (m_readerIsActive)
        resetReader();
}

void FormEditorAnnotationIcon::annotationDialogRejected()
{
    if (m_annotationEditor) {
        m_annotationEditor->close();
        m_annotationEditor->deleteLater();
    }

    m_annotationEditor = nullptr;
}

} //QmlDesigner
