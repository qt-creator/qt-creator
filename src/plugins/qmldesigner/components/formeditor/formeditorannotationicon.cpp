// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "formeditorannotationicon.h"
#include "formeditortracing.h"

#include <qmldesignertr.h>

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
using FormEditorTracing::category;

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
    NanotraceHR::Tracer tracer{"formeditor annotation icon constructor", category()};

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

    setToolTip(Tr::tr("Annotation"));
    setCursor(Qt::ArrowCursor);
}

FormEditorAnnotationIcon::~FormEditorAnnotationIcon()
{
    NanotraceHR::Tracer tracer{"formeditor annotation icon destructor", category()};

    if (m_annotationEditor) {
        m_annotationEditor->deleteLater();
    }
}

void FormEditorAnnotationIcon::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    NanotraceHR::Tracer tracer{"formeditor annotation icon paint", category()};

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
    NanotraceHR::Tracer tracer{"formeditor annotation icon bounding rect", category()};

    return QRectF(0, 0, m_iconW, m_iconH);
}

qreal FormEditorAnnotationIcon::iconWidth()
{
    NanotraceHR::Tracer tracer{"formeditor annotation icon icon width", category()};

    return m_iconW;
}

qreal FormEditorAnnotationIcon::iconHeight()
{
    NanotraceHR::Tracer tracer{"formeditor annotation icon icon height", category()};

    return m_iconH;
}

bool FormEditorAnnotationIcon::isReaderActive()
{
    NanotraceHR::Tracer tracer{"formeditor annotation icon is reader active", category()};

    return m_readerIsActive;
}

void FormEditorAnnotationIcon::setActive(bool readerStatus)
{
    NanotraceHR::Tracer tracer{"formeditor annotation icon set active", category()};

    m_readerIsActive = readerStatus;

    if (m_readerIsActive)
        resetReader();
    else
        removeReader();

    update();
}

void FormEditorAnnotationIcon::hoverEnterEvent(QGraphicsSceneHoverEvent * event)
{
    NanotraceHR::Tracer tracer{"formeditor annotation icon hover enter event", category()};

    QGraphicsItem::hoverEnterEvent(event);
    event->accept();
    update();
}

void FormEditorAnnotationIcon::hoverLeaveEvent(QGraphicsSceneHoverEvent * event)
{
    NanotraceHR::Tracer tracer{"formeditor annotation icon hover leave event", category()};

    QGraphicsItem::hoverLeaveEvent(event);
    event->accept();
    update();
}

void FormEditorAnnotationIcon::hoverMoveEvent(QGraphicsSceneHoverEvent * event)
{
    NanotraceHR::Tracer tracer{"formeditor annotation icon hover move event", category()};

    QGraphicsItem::hoverMoveEvent(event);
}

void FormEditorAnnotationIcon::mousePressEvent(QGraphicsSceneMouseEvent * event)
{
    NanotraceHR::Tracer tracer{"formeditor annotation icon mouse press event", category()};

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
    NanotraceHR::Tracer tracer{"formeditor annotation icon mouse release event", category()};

    event->accept();
}

void FormEditorAnnotationIcon::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
    NanotraceHR::Tracer tracer{"formeditor annotation icon context menu event", category()};

    QMenu menu;
    menu.addAction(Tr::tr("Edit Annotation"), [this] { createAnnotationEditor(); });

    menu.addAction(Tr::tr("Remove Annotation"), [this] { removeAnnotationDialog(); });

    menu.exec(event->screenPos());
    event->accept();
}

void FormEditorAnnotationIcon::drawReader()
{
    NanotraceHR::Tracer tracer{"formeditor annotation icon draw reader", category()};

    if (!childItems().isEmpty()) {
        for (QGraphicsItem * const item  : childItems()) {
            item->show();
        }
    }
    else {
        createReader();
    }
}

void FormEditorAnnotationIcon::hideReader()
{
    NanotraceHR::Tracer tracer{"formeditor annotation icon hide reader", category()};

    if (!childItems().isEmpty())
        for (QGraphicsItem * const item : childItems())
            item->hide();
}

void FormEditorAnnotationIcon::quickResetReader()
{
    NanotraceHR::Tracer tracer{"formeditor annotation icon quick reset reader", category()};

    hideReader();
    drawReader();
}

void FormEditorAnnotationIcon::resetReader()
{
    NanotraceHR::Tracer tracer{"formeditor annotation icon reset reader", category()};

    removeReader();
    createReader();
}

void FormEditorAnnotationIcon::createReader()
{
    NanotraceHR::Tracer tracer{"formeditor annotation icon create reader", category()};

    const qreal width = 290.;
    const qreal height = 30.;
    const qreal offset = 5.;


    QPointF cornerPosition(m_iconW + offset, 0);
    qreal nextYAfterTitle = 40.;

    if (!m_customId.isEmpty()) {
        const QRectF titleRect(0., 0., width, height);

        QGraphicsItem *titleBubble = createTitleBubble(titleRect, m_customId, this);
        titleBubble->setPos(cornerPosition);

        nextYAfterTitle = (titleRect.height() + (offset*2));
    }
    else {
        nextYAfterTitle = 0.;
    }

    if (m_annotation.hasComments()) {
        QList<QGraphicsItem*> comments;

        QPointF commentPosition(cornerPosition.x(), nextYAfterTitle);
        QRectF commentRect(0., 0., width, height);

        for (const Comment &comment : m_annotation.comments()) {
            QGraphicsItem *commentBubble = createCommentBubble(commentRect, comment.title(),
                                                               comment.author(), comment.deescapedText(),
                                                               comment.timestampStr(), this);
            comments.push_back(commentBubble);
        }


        const qreal maxHeight = 650.;
        const QPointF commentsStartPosition(cornerPosition.x(), cornerPosition.y() + nextYAfterTitle);
        QPointF newPos(commentsStartPosition);
        qreal columnHeight = commentsStartPosition.y();

        for (QGraphicsItem * const comment : comments) {

            comment->setPos(newPos); //first place comment in its new position, then calculate position for next comment

            const qreal itemHeight = comment->boundingRect().height();
            const qreal itemWidth = comment->boundingRect().width();

            const qreal possibleHeight = columnHeight + offset + itemHeight;
            qreal newX = 0.;

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
    NanotraceHR::Tracer tracer{"formeditor annotation icon remove reader", category()};

    if (!childItems().isEmpty())
        qDeleteAll(childItems());
}

QGraphicsItem *FormEditorAnnotationIcon::createCommentBubble(QRectF rect, const QString &title,
                                                             const QString &author, const QString &text,
                                                             const QString &date, QGraphicsItem *parent)
{
    NanotraceHR::Tracer tracer{"formeditor annotation icon create comment bubble", category()};

    static QColor textColor = Utils::creatorColor(Utils::Theme::DStextColor);
    static QColor backgroundColor = Utils::creatorColor(Utils::Theme::QmlDesigner_BackgroundColorDarker);
    static QColor frameColor = Utils::creatorColor(Utils::Theme::QmlDesigner_BackgroundColor);
    QFont font;
    font.setBold(true);

    QGraphicsRectItem *frameItem = new QGraphicsRectItem(rect, parent);
    const qreal frameX = frameItem->x();
    qreal nextY = 0.;

    const qreal offset = 5.; //used only in text block
    qreal titleHeight = 0.;
    qreal authorHeight = 0.;
    qreal textHeight = 0.;
    qreal dateHeight = 0.;

    if (!title.isEmpty()) {
        QGraphicsTextItem *titleItem = new QGraphicsTextItem(frameItem);
        titleItem->setPlainText(title);
        titleItem->setFont(font);
        titleItem->setDefaultTextColor(textColor);
        titleItem->setTextWidth(rect.width());
        titleItem->update();

        titleHeight = titleItem->boundingRect().height();
        nextY = titleHeight + titleItem->y();
    }

    if (!author.isEmpty()) {
        QGraphicsTextItem *authorItem = new QGraphicsTextItem(frameItem);
        authorItem->setPlainText(Tr::tr("By: ") + author);
        authorItem->setDefaultTextColor(textColor);
        authorItem->setTextWidth(rect.width());
        authorItem->setPos(frameX, nextY);
        authorItem->update();

        authorHeight = authorItem->boundingRect().height();
        nextY = authorHeight + authorItem->y();
    }

    if (!text.isEmpty()) {
        QGraphicsTextItem *textItem = new QGraphicsTextItem(frameItem);
        textItem->setHtml(text);

        //we can receive rich text qstr with html header, but without content
        if (textItem->toPlainText().isEmpty()) {
            delete textItem;
        }
        else {
            if (!title.isEmpty() || !author.isEmpty()) {
                nextY += offset;
                textHeight += offset;
            }
            textItem->setDefaultTextColor(textColor);
            textItem->setTextWidth(rect.width());
            textItem->setPos(frameX, nextY);
            textItem->update();

            if (textItem->boundingRect().width() > textItem->textWidth()) {
                textItem->setTextWidth(textItem->boundingRect().width());
                textItem->update();
                rect.setWidth(textItem->boundingRect().width());
            }

            textHeight += textItem->boundingRect().height() + offset;
            nextY = textHeight + textItem->y();
        }
    }

    if (!date.isEmpty()) {
        QGraphicsTextItem *dateItem = new QGraphicsTextItem(frameItem);
        dateItem->setPlainText(Tr::tr("Edited: ") + date);
        dateItem->setDefaultTextColor(textColor);
        dateItem->setTextWidth(rect.width());
        dateItem->setPos(frameX, nextY);
        dateItem->update();

        dateHeight = dateItem->boundingRect().height();

//        nextY = dateHeight + dateItem->y();
    }

    const qreal contentRect = titleHeight + authorHeight + textHeight + dateHeight;
    rect.setHeight(contentRect);

    frameItem->setRect(rect);

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
    NanotraceHR::Tracer tracer{"formeditor annotation icon create title bubble", category()};

    static QColor textColor = Utils::creatorColor(Utils::Theme::DStextColor);
    static QColor backgroundColor = Utils::creatorColor(Utils::Theme::QmlDesigner_BackgroundColorDarker);
    static QColor frameColor = Utils::creatorColor(Utils::Theme::QmlDesigner_BackgroundColor);
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
    NanotraceHR::Tracer tracer{"formeditor annotation icon create annotation editor", category()};

    if (m_annotationEditor) {
        m_annotationEditor->close();
        m_annotationEditor->deleteLater();
        m_annotationEditor = nullptr;
    }

    m_annotationEditor = new AnnotationEditorDialog(Core::ICore::dialogParent(),
                                                    m_modelNode.displayName(),
                                                    m_modelNode.customId());
    m_annotationEditor->setAnnotation(m_modelNode.annotation());

    connect(m_annotationEditor, &AnnotationEditorDialog::acceptedDialog,
            this, &FormEditorAnnotationIcon::annotationDialogAccepted);
    connect(m_annotationEditor, &QDialog::rejected,
            this, &FormEditorAnnotationIcon::annotationDialogRejected);

    m_annotationEditor->show();
    m_annotationEditor->raise();
}

void FormEditorAnnotationIcon::removeAnnotationDialog()
{
    NanotraceHR::Tracer tracer{"formeditor annotation icon remove annotation dialog", category()};

    QString dialogTitle = Tr::tr("Annotation");
    if (!m_customId.isNull()) {
        dialogTitle = m_customId;
    }
    QPointer<QMessageBox> deleteDialog = new QMessageBox(Core::ICore::dialogParent());
    deleteDialog->setWindowTitle(dialogTitle);
    deleteDialog->setText(Tr::tr("Delete this annotation?"));
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
    NanotraceHR::Tracer tracer{"formeditor annotation icon annotation dialog accepted", category()};

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
    NanotraceHR::Tracer tracer{"formeditor annotation icon annotation dialog rejected", category()};

    if (m_annotationEditor) {
        m_annotationEditor->close();
        m_annotationEditor->deleteLater();
    }

    m_annotationEditor = nullptr;
}

} //QmlDesigner
