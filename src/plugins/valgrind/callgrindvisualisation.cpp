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
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "callgrindvisualisation.h"

#include "callgrindhelper.h"

#include <valgrind/callgrind/callgrindabstractmodel.h>
#include <valgrind/callgrind/callgrinddatamodel.h>
#include <valgrind/callgrind/callgrindfunction.h>
#include <valgrind/callgrind/callgrindproxymodel.h>
#include <utils/qtcassert.h>

#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsSimpleTextItem>
#include <QMouseEvent>
#include <QStaticText>
#include <QStyleOptionGraphicsItem>
#include <QPair>
#include <QPersistentModelIndex>
#include <QLinkedList>
#include <QAbstractItemModel>
#include <QDebug>

#define VISUALISATION_DEBUG 0
// Margin from hardcoded value in:
// QGraphicsView::fitInView(const QRectF &rect,
//                          Qt::AspectRatioMode aspectRatioMode)
// Bug report here: https://bugreports.qt.io/browse/QTBUG-11945
static const int FIT_IN_VIEW_MARGIN = 2;

using namespace Valgrind::Callgrind;

namespace Valgrind {
namespace Internal {

class FunctionGraphicsTextItem : public QAbstractGraphicsShapeItem
{
public:
    FunctionGraphicsTextItem(const QString &text, QGraphicsItem *parent);

    virtual void paint(QPainter *painter,
                       const QStyleOptionGraphicsItem *option, QWidget *widget);
    virtual QRectF boundingRect() const;

private:
    QString m_text;
    QStaticText m_staticText;
    qreal m_previousViewportDimension;
};

class FunctionGraphicsItem : public QGraphicsRectItem
{
public:
    enum DataKey {
        FunctionCallKey
    };

    FunctionGraphicsItem(const QString &text, qreal x, qreal y,
                         qreal width, qreal height, QGraphicsItem *parent = 0);

    virtual void paint(QPainter *painter,
                       const QStyleOptionGraphicsItem *option, QWidget *widget);
    FunctionGraphicsTextItem *textItem() const;

private:
    FunctionGraphicsTextItem *m_text;
};

FunctionGraphicsTextItem::FunctionGraphicsTextItem(const QString &text,
                                                   QGraphicsItem *parent)
    : QAbstractGraphicsShapeItem(parent)
    , m_text(text)
    , m_previousViewportDimension(0)
{
    setFlag(QGraphicsItem::ItemIgnoresTransformations);
    setAcceptedMouseButtons(0); // do not steal focus from parent item
    setToolTip(text);
}

void FunctionGraphicsTextItem::paint(QPainter *painter,
                                     const QStyleOptionGraphicsItem *,
                                     QWidget *widget)
{
    const qreal textHeight = painter->fontMetrics().height();
    // Magic number based on what looked best.
    const int margin = 2 + FIT_IN_VIEW_MARGIN;
    const QRectF viewportRect =
            widget->rect().adjusted(margin, margin, -margin, -margin);

    const qreal maxWidth = viewportRect.width()
            * parentItem()->boundingRect().width()
            / scene()->sceneRect().width();
    const qreal maxHeight = viewportRect.height()
            * parentItem()->boundingRect().height()
            / scene()->sceneRect().height();

    if (textHeight > maxHeight)
        return;

    if (viewportRect.width() != m_previousViewportDimension) {
        const QString &elidedText =
                painter->fontMetrics().elidedText(m_text, Qt::ElideRight,
                                                  maxWidth);
        m_staticText.setText(elidedText);
        m_staticText.prepare();

        m_previousViewportDimension = viewportRect.width();
    }

#if VISUALISATION_DEBUG
    painter->setPen(Qt::red);
    painter->drawRect(boundingRect());
#endif

    painter->save();
    int textLeft = 0;
    int textTop = 0;
    const int textWidth = painter->fontMetrics().width(m_staticText.text());
    textLeft = -textWidth/2;
    textTop = (maxHeight - textHeight)/2;
    painter->drawStaticText(textLeft, textTop, m_staticText);

    painter->restore();
}

QRectF FunctionGraphicsTextItem::boundingRect() const
{
    return mapRectFromParent(parentItem()->boundingRect());
}

FunctionGraphicsItem::FunctionGraphicsItem(const QString &text,
        qreal x, qreal y, qreal width, qreal height, QGraphicsItem *parent)
    : QGraphicsRectItem(x, y, width, height, parent)
    , m_text(0)
{
    setFlag(QGraphicsItem::ItemIsSelectable);
    setFlag(QGraphicsItem::ItemClipsToShape);
    setFlag(QGraphicsItem::ItemClipsChildrenToShape);
    setToolTip(text);

    m_text = new FunctionGraphicsTextItem(text, this);
    m_text->setPos(rect().center().x(), y);
}

FunctionGraphicsTextItem *FunctionGraphicsItem::textItem() const
{
    return m_text;
}

void FunctionGraphicsItem::paint(QPainter *painter,
    const QStyleOptionGraphicsItem *option, QWidget *)
{
    painter->save();

    QRectF rect = this->rect();
    const QColor &color = brush().color();
    if (option->state & QStyle::State_Selected) {
        QLinearGradient gradient(0, 0, rect.width(), 0);
        gradient.setColorAt(0, color.darker(100));
        gradient.setColorAt(0.5, color.lighter(200));
        gradient.setColorAt(1, color.darker(100));
        painter->setBrush(gradient);
    } else {
        painter->setBrush(color);
    }

#if VISUALISATION_DEBUG
    painter->setPen(Qt::blue);
    painter->drawRect(boundingRect());
#endif

    QPen pen = painter->pen();
    pen.setColor(color.darker());
    pen.setWidthF(0.5);
    painter->setPen(pen);
    qreal halfPenWidth = pen.widthF()/2.0;
    rect.adjust(halfPenWidth, halfPenWidth, -halfPenWidth, -halfPenWidth);
    painter->drawRect(rect);

    painter->restore();
}

class Visualisation::Private
{
public:
    Private(Visualisation *qq);

    void handleMousePressEvent(QMouseEvent *event, bool doubleClicked);
    qreal sceneHeight() const;
    qreal sceneWidth() const;

    Visualisation *q;
    DataProxyModel *m_model;
    QGraphicsScene m_scene;
};

Visualisation::Private::Private(Visualisation *qq)
    : q(qq)
    , m_model(new DataProxyModel(qq))
{
    // setup scene
    m_scene.setObjectName(QLatin1String("Visualisation Scene"));
    ///NOTE: with size 100x100 the Qt-internal mouse selection fails...
    m_scene.setSceneRect(0, 0, 1024, 1024);

    // setup model
    m_model->setMinimumInclusiveCostRatio(0.1);
    connect(m_model, &DataProxyModel::filterFunctionChanged,
            qq, &Visualisation::populateScene);
}

void Visualisation::Private::handleMousePressEvent(QMouseEvent *event,
                                                   bool doubleClicked)
{
    // find the first item that accepts mouse presses under the cursor position
    QGraphicsItem *itemAtPos = 0;
    foreach (QGraphicsItem *item, q->items(event->pos())) {
        if (!(item->acceptedMouseButtons() & event->button()))
            continue;

        itemAtPos = item;
        break;
    }

    // if there is an item, select it
    if (itemAtPos) {
        const Function *func = q->functionForItem(itemAtPos);

        if (doubleClicked) {
            q->functionActivated(func);
        } else {
            q->scene()->clearSelection();
            itemAtPos->setSelected(true);
            q->functionSelected(func);
        }
    }

}

qreal Visualisation::Private::sceneHeight() const
{
    return m_scene.height() - FIT_IN_VIEW_MARGIN;
}

qreal Visualisation::Private::sceneWidth() const
{
    // Magic number to improve margins appearance
    return m_scene.width() + 1;
}

Visualisation::Visualisation(QWidget *parent)
    : QGraphicsView(parent)
    , d(new Private(this))
{
    setObjectName(QLatin1String("Visualisation View"));
    setScene(&d->m_scene);
    setRenderHint(QPainter::Antialiasing);
}

Visualisation::~Visualisation()
{
    delete d;
}

const Function *Visualisation::functionForItem(QGraphicsItem *item) const
{
    return item->data(FunctionGraphicsItem::FunctionCallKey).value<const Function *>();
}

QGraphicsItem *Visualisation::itemForFunction(const Function *function) const
{
    foreach (QGraphicsItem *item, items()) {
        if (functionForItem(item) == function)
            return item;
    }
    return 0;
}

void Visualisation::setFunction(const Function *function)
{
    d->m_model->setFilterFunction(function);
}

const Function *Visualisation::function() const
{
    return d->m_model->filterFunction();
}

void Visualisation::setMinimumInclusiveCostRatio(double ratio)
{
    d->m_model->setMinimumInclusiveCostRatio(ratio);
}

void Visualisation::setModel(QAbstractItemModel *model)
{
    QTC_ASSERT(!d->m_model->sourceModel() && model, return); // only set once!
    d->m_model->setSourceModel(model);

    connect(model, &QAbstractItemModel::columnsInserted,
            this, &Visualisation::populateScene);
    connect(model, &QAbstractItemModel::columnsMoved,
            this, &Visualisation::populateScene);
    connect(model, &QAbstractItemModel::columnsRemoved,
            this, &Visualisation::populateScene);
    connect(model, &QAbstractItemModel::dataChanged,
            this, &Visualisation::populateScene);
    connect(model, &QAbstractItemModel::headerDataChanged,
            this, &Visualisation::populateScene);
    connect(model, &QAbstractItemModel::layoutChanged, this, &Visualisation::populateScene);
    connect(model, &QAbstractItemModel::modelReset, this, &Visualisation::populateScene);
    connect(model, &QAbstractItemModel::rowsInserted,
            this, &Visualisation::populateScene);
    connect(model, &QAbstractItemModel::rowsMoved,
            this, &Visualisation::populateScene);
    connect(model, &QAbstractItemModel::rowsRemoved,
            this, &Visualisation::populateScene);

    populateScene();
}

void Visualisation::setText(const QString &message)
{
    d->m_scene.clear();

    QGraphicsSimpleTextItem *textItem = d->m_scene.addSimpleText(message);
    textItem->setBrush(palette().foreground());
    textItem->setPos((d->sceneWidth() - textItem->boundingRect().width()) / 2,
                     (d->sceneHeight() - textItem->boundingRect().height()) / 2);
    textItem->setFlag(QGraphicsItem::ItemIgnoresTransformations);
}

void Visualisation::populateScene()
{
    // reset scene first
    d->m_scene.clear();

    const qreal sceneWidth = d->sceneWidth();
    const qreal sceneHeight = d->sceneHeight();

    // cache costs of each element, calculate total costs
    qreal total = 0;

    typedef QPair<QModelIndex, qreal> Pair;
    QLinkedList<Pair> costs;
    for (int row = 0; row < d->m_model->rowCount(); ++row) {
        const QModelIndex index = d->m_model->index(row, DataModel::InclusiveCostColumn);

        bool ok = false;
        const qreal cost = index.data().toReal(&ok);
        QTC_ASSERT(ok, continue);
        costs << QPair<QModelIndex, qreal>(d->m_model->index(row, 0), cost);
        total += cost;
    }

    if (!costs.isEmpty() || d->m_model->filterFunction()) {
        // item showing the current filter function

        QString text;
        if (d->m_model->filterFunction()) {
            text = d->m_model->filterFunction()->name();
        } else {
            const float ratioPercent = d->m_model->minimumInclusiveCostRatio() * 100;
            QString ratioPercentString = QString::number(ratioPercent);
            ratioPercentString.append(QLocale::system().percent());
            const int hiddenFunctions = d->m_model->sourceModel()->rowCount() - d->m_model->rowCount();
            text = tr("All functions with an inclusive cost ratio higher than %1 (%2 are hidden)")
                    .arg(ratioPercentString)
                    .arg(hiddenFunctions);
        }

        const qreal height = sceneHeight * (costs.isEmpty() ? 1.0 : 0.1);
        FunctionGraphicsItem *item = new FunctionGraphicsItem(text, 0, 0, sceneWidth, height);
        const QColor background = CallgrindHelper::colorForString(text);
        item->setBrush(background);
        item->setData(FunctionGraphicsItem::FunctionCallKey, QVariant::fromValue(d->m_model->filterFunction()));
        // NOTE: workaround wrong tooltip being show, no idea why...
        item->setZValue(-1);
        d->m_scene.addItem(item);
    }

    // add the canvas elements to the scene
    qreal used = sceneHeight * 0.1;
    foreach (const Pair &cost, costs) {
        const QModelIndex &index = cost.first;
        const QString text = index.data().toString();

        const qreal height = (sceneHeight * 0.9 * cost.second) / total;

        FunctionGraphicsItem *item = new FunctionGraphicsItem(text, 0, used, sceneWidth, height);
        const QColor background = CallgrindHelper::colorForString(text);
        item->setBrush(background);
        item->setData(FunctionGraphicsItem::FunctionCallKey, index.data(DataModel::FunctionRole));
        d->m_scene.addItem(item);
        used += height;
    }
}

void Visualisation::mousePressEvent(QMouseEvent *event)
{
    d->handleMousePressEvent(event, false);

    QGraphicsView::mousePressEvent(event);
}

void Visualisation::mouseDoubleClickEvent(QMouseEvent *event)
{
    d->handleMousePressEvent(event, true);

    QGraphicsView::mouseDoubleClickEvent(event);
}

void Visualisation::resizeEvent(QResizeEvent *event)
{
    fitInView(sceneRect());

    QGraphicsView::resizeEvent(event);
}

} // namespace Internal
} // namespace Valgrind
