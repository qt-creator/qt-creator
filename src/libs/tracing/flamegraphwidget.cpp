// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "flamegraphwidget.h"

#include <utils/stylehelper.h>
#include <utils/theme/theme.h>

#include <QAbstractItemModel>
#include <QComboBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QModelIndex>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollArea>
#include <QVBoxLayout>

namespace Timeline {

// ---------------------------------------------------------------------------
// Layout

struct LayoutNode {
    QModelIndex modelIndex;
    qreal relativePosition;
    qreal relativeSize;
    int depth;
    int parentNodeIndex;  // -1 = root
};

// ---------------------------------------------------------------------------
// Private + canvas forward declaration

class FlameGraphCanvas;

class FlameGraphWidgetPrivate
{
public:
    FlameGraphWidget *q = nullptr;
    QAbstractItemModel *model = nullptr;

    // Config
    QList<QPair<int, QString>> sizeRoles;
    int typeIdRole = -1;
    int sourceFileRole = -1;
    int sourceLineRole = -1;
    int sourceColumnRole = -1;
    int summaryRole = -1;
    int detailsTitleRole = -1;
    QList<QPair<int, QString>> detailsRoles;
    int noteRole = -1;
    QString othersText;

    // State
    int selectedTypeId = -1;
    int selectedNodeIndex = -1;
    int lastHoveredNodeIndex = -1;  // kept after mouse leaves so panel stays visible
    QPersistentModelIndex root;

    // Layout
    QList<LayoutNode> nodes;
    int treeDepth = 0;
    static constexpr int kMinRowHeight = 30;
    static constexpr int kMaxRowHeight = 60;

    // UI
    QScrollArea *scrollArea = nullptr;
    FlameGraphCanvas *canvas = nullptr;
    QComboBox *modeCombo = nullptr;

    int currentSizeRole() const
    {
        int idx = modeCombo ? modeCombo->currentIndex() : 0;
        if (idx >= 0 && idx < sizeRoles.size())
            return sizeRoles.at(idx).first;
        return sizeRoles.isEmpty() ? -1 : sizeRoles.first().first;
    }

    QSize canvasSizeHint() const
    {
        return {100, treeDepth * kMinRowHeight};
    }

    void rebuild();

    QVector<QRectF> computeRects(qreal canvasWidth, qreal canvasHeight, int rowHeight) const;
    QColor colorForNode(int i, QStyle::State state) const;
    QString summaryText(int i) const;

    // currentNode: hoveredNode || selectedNode || lastHoveredNode
    int detailsNodeIndex() const;
    void showDetails(int nodeIndex);
};

// ---------------------------------------------------------------------------
// Canvas

class FlameGraphCanvas : public QWidget
{
public:
    explicit FlameGraphCanvas(FlameGraphWidgetPrivate *d, QWidget *parent)
        : QWidget(parent), d(d)
    {
        setMouseTracking(true);
    }

    QSize sizeHint() const override
    {
        return d->canvasSizeHint();
    }

    QSize minimumSizeHint() const override
    {
        return d->canvasSizeHint();
    }

    int m_hoveredNode = -1;

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    int rowHeight() const
    {
        return qBound(d->kMinRowHeight, height() / qMax(1, d->treeDepth), d->kMaxRowHeight);
    }

    int nodeAt(const QPoint &pos) const
    {
        if (d->nodes.isEmpty())
            return -1;
        int rh = rowHeight();
        auto rects = d->computeRects(width(), height(), rh);
        for (int i = rects.size() - 1; i >= 0; --i) {
            if (rects.at(i).contains(pos))
                return i;
        }
        return -1;
    }

    void handleSelect(const QPoint &pos)
    {
        int i = nodeAt(pos);
        if (i < 0) {
            d->selectedTypeId = -1;
            d->selectedNodeIndex = -1;
            d->lastHoveredNodeIndex = -1;
            d->showDetails(-1);
            emit d->q->typeSelected(-1);
            update();
            return;
        }
        const LayoutNode &node = d->nodes.at(i);
        if (!node.modelIndex.isValid())
            return;

        int typeId = (d->typeIdRole >= 0) ? node.modelIndex.data(d->typeIdRole).toInt() : -1;
        bool changed = (typeId != d->selectedTypeId);
        d->selectedTypeId = typeId;
        d->selectedNodeIndex = i;
        d->lastHoveredNodeIndex = -1;
        if (changed) {
            emit d->q->typeSelected(typeId);
            update();
        }
        d->showDetails(d->detailsNodeIndex());

        if (d->sourceFileRole >= 0) {
            QString file = node.modelIndex.data(d->sourceFileRole).toString();
            int line = (d->sourceLineRole >= 0)
                           ? node.modelIndex.data(d->sourceLineRole).toInt() : -1;
            int col  = (d->sourceColumnRole >= 0)
                           ? node.modelIndex.data(d->sourceColumnRole).toInt() : 0;
            if (!file.isEmpty())
                emit d->q->gotoSourceLocation(file, line, col);
        }
    }

    FlameGraphWidgetPrivate *d;
};

void FlameGraphCanvas::paintEvent(QPaintEvent *event)
{
    QPainter p(this);
    p.fillRect(event->rect(), Utils::creatorColor(Utils::Theme::Timeline_BackgroundColor1));

    if (d->nodes.isEmpty())
        return;

    int rh = rowHeight();
    auto rects = d->computeRects(width(), height(), rh);

    p.setRenderHint(QPainter::Antialiasing, false);

    QFont boldFont = font();
    boldFont.setBold(true);

    for (int i = 0; i < d->nodes.size(); ++i) {
        QRect r = rects.at(i).toRect();
        if (!r.intersects(event->rect()))
            continue;

        const LayoutNode &node = d->nodes.at(i);
        int typeId = (d->typeIdRole >= 0 && node.modelIndex.isValid())
                         ? node.modelIndex.data(d->typeIdRole).toInt() : -1;
        bool selected = (typeId >= 0 && typeId == d->selectedTypeId);
        bool hovered = (i == m_hoveredNode);
        bool hasNote = (d->noteRole >= 0 && node.modelIndex.isValid()
                        && !node.modelIndex.data(d->noteRole).toString().isEmpty());

        const QStyle::State state = selected ? QStyle::State_Selected
                                             : (hovered ? QStyle::State_MouseOver
                                                        : QStyle::State_None);
        const QColor fill = d->colorForNode(i, state);

        QColor outline;
        int outlineWidth = 0;
        if (selected) {
            outline = Utils::creatorColor(Utils::Theme::Token_Notification_Neutral_Default);
            outlineWidth = 2;
        } else if (hovered) {
            outline = Utils::creatorColor(Utils::Theme::Token_Notification_Neutral_Muted);
            outlineWidth = 2;
        } else if (hasNote) {
            outline = Utils::creatorColor(Utils::Theme::Timeline_HighlightColor);
            outlineWidth = 2;
        }

        const int gap = 1;
        if (outlineWidth > 0) {
            p.fillRect(r.adjusted(0, 0, -gap, -gap), outline);
            p.fillRect(r.adjusted(outlineWidth, outlineWidth,
                                  -gap - outlineWidth, -gap - outlineWidth), fill);
        } else {
            p.fillRect(r.adjusted(0, 0, -gap, -gap), fill);
        }

        if (selected || hovered || r.width() > 20) {
            p.save();
            if (selected)
                p.setFont(boldFont);
            p.setPen(Utils::creatorColor(Utils::Theme::PanelTextColorLight));
            const int padding = qMax(outlineWidth, 1);
            const QRect textRect = r.adjusted(padding, padding, -padding - gap, -padding - gap);
            p.drawText(textRect, Qt::AlignVCenter | Qt::AlignHCenter | Qt::TextWordWrap,
                       d->summaryText(i));
            p.restore();
        }
    }
}

void FlameGraphCanvas::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        handleSelect(event->pos());
}

void FlameGraphCanvas::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton)
        return;
    int i = nodeAt(event->pos());
    if (i < 0) {
        d->root = QModelIndex();
        d->selectedTypeId = -1;
        d->selectedNodeIndex = -1;
        d->lastHoveredNodeIndex = -1;
        d->showDetails(-1);
    } else {
        const LayoutNode &node = d->nodes.at(i);
        d->root = node.modelIndex.isValid() ? node.modelIndex : QModelIndex();
        handleSelect(event->pos());
    }
    d->rebuild();
}

void FlameGraphCanvas::mouseMoveEvent(QMouseEvent *event)
{
    int i = nodeAt(event->pos());
    if (i != m_hoveredNode) {
        m_hoveredNode = i;
        d->showDetails(d->detailsNodeIndex());
        update();
    }
}

void FlameGraphCanvas::leaveEvent(QEvent *)
{
    if (m_hoveredNode != -1) {
        // QML: on mouseExited, if nothing is selected the last hovered node
        // becomes selectedNode so the panel stays visible.
        if (d->selectedNodeIndex < 0)
            d->lastHoveredNodeIndex = m_hoveredNode;
        m_hoveredNode = -1;
        d->showDetails(d->detailsNodeIndex());
        update();
    }
}

// ---------------------------------------------------------------------------
// Private implementation

static int buildLayoutStep(QAbstractItemModel *model, int sizeRole,
                            const QPersistentModelIndex &root,
                            QList<LayoutNode> *nodes,
                            const QModelIndex &parentIndex, int parentNodeIndex,
                            int depth, int maxDepth)
{
    constexpr qreal kSizeThreshold = 0.002;
    qreal parentSize = model->data(parentIndex, sizeRole).toReal();
    qreal rootSize = model->data(root, sizeRole).toReal();
    int childrenDepth = depth;

    qreal position = 0;
    qreal skipped = 0;

    if (depth < maxDepth - 1) {
        int rowCount = model->rowCount(parentIndex);
        for (int row = 0; row < rowCount; ++row) {
            QModelIndex childIndex = model->index(row, 0, parentIndex);
            qreal size = model->data(childIndex, sizeRole).toReal();
            if (rootSize > 0 && size / rootSize < kSizeThreshold) {
                skipped += size;
                continue;
            }
            int nodeIndex = nodes->size();
            nodes->append({childIndex, position / parentSize, size / parentSize,
                            depth, parentNodeIndex});
            position += size;
            childrenDepth = qMax(childrenDepth,
                                 buildLayoutStep(model, sizeRole, root, nodes,
                                                 childIndex, nodeIndex, depth + 1, maxDepth));
        }
    } else {
        skipped = parentSize;
    }

    if (!parentIndex.isValid())
        skipped = parentSize - position;

    if (skipped > 0) {
        nodes->append({QModelIndex(), position / parentSize, skipped / parentSize,
                        depth, parentNodeIndex});
        childrenDepth = qMax(childrenDepth, depth + 1);
    }

    return childrenDepth;
}

void FlameGraphWidgetPrivate::rebuild()
{
    nodes.clear();
    treeDepth = 0;
    selectedNodeIndex = -1;
    lastHoveredNodeIndex = -1;

    int sizeRole = currentSizeRole();
    if (model && sizeRole >= 0 && model->data(root, sizeRole).toReal() > 0) {
        constexpr int kMaxDepth = 128;
        if (root.isValid()) {
            int rootNodeIndex = nodes.size();
            nodes.append({root, 0, 1, 0, -1});
            treeDepth = buildLayoutStep(model, sizeRole, root, &nodes,
                                        root, rootNodeIndex, 1, kMaxDepth);
        } else {
            treeDepth = buildLayoutStep(model, sizeRole, root, &nodes,
                                        QModelIndex(), -1, 0, kMaxDepth);
        }
    }

    // Restore selected node index after layout rebuild.
    if (selectedTypeId >= 0 && typeIdRole >= 0) {
        for (int i = 0; i < nodes.size(); ++i) {
            if (nodes.at(i).modelIndex.isValid()
                && nodes.at(i).modelIndex.data(typeIdRole).toInt() == selectedTypeId) {
                selectedNodeIndex = i;
                break;
            }
        }
    }

    if (canvas) {
        canvas->updateGeometry();
        canvas->update();
    }
}

QVector<QRectF> FlameGraphWidgetPrivate::computeRects(qreal canvasWidth, qreal canvasHeight, int rowHeight) const
{
    QVector<QRectF> rects(nodes.size());
    for (int i = 0; i < nodes.size(); ++i) {
        const LayoutNode &node = nodes.at(i);
        const QRectF parentRect = node.parentNodeIndex < 0
                                      ? QRectF(0, 0, canvasWidth, 0) // Rect height is irrelevant
                                      : rects.at(node.parentNodeIndex);
        qreal x = parentRect.x() + node.relativePosition * parentRect.width();
        qreal w = node.relativeSize * parentRect.width();
        qreal y = canvasHeight - (node.depth + 1) * rowHeight;
        rects[i] = QRectF(x, y, w, rowHeight);
    }
    return rects;
}

QColor FlameGraphWidgetPrivate::colorForNode(int i, QStyle::State state) const
{
    const LayoutNode &node = nodes.at(i);
    if (!node.modelIndex.isValid())
        return Utils::creatorColor(Utils::Theme::Timeline_BackgroundColor2);

    int typeId = (typeIdRole >= 0) ? node.modelIndex.data(typeIdRole).toInt() : i;
    static const QColor bgColor = Utils::creatorColor(Utils::Theme::Timeline_BackgroundColor1);
    const int bgBlend = state == QStyle::State_Selected ? 15
                                                        : (state == QStyle::State_MouseOver ? 30
                                                                                            : 45);
    const QColor baseColor = Utils::StyleHelper::mergedColors(
        Utils::creatorColor(Utils::Theme::Token_Notification_Danger_Default), bgColor, bgBlend);
    const QColor tipColor = Utils::StyleHelper::mergedColors(
        Utils::creatorColor(Utils::Theme::Token_Notification_Alert_Default), bgColor, bgBlend);
    const int steps = qMin(8, treeDepth);
    const int typeVariationRange = 32;
    const int typeVariation = (((typeId * 97) & 0xFF) % typeVariationRange)
                              - typeVariationRange / 2;
    const int blendFactor = qBound(0,
                                   100 - (100 / steps * (node.depth % steps)) + typeVariation,
                                   100);
    return Utils::StyleHelper::mergedColors(baseColor, tipColor, blendFactor);
}

QString FlameGraphWidgetPrivate::summaryText(int i) const
{
    const LayoutNode &node = nodes.at(i);
    if (!node.modelIndex.isValid())
        return othersText.isEmpty() ? FlameGraphWidget::tr("others") : othersText;
    if (summaryRole < 0)
        return {};
    QString text = node.modelIndex.data(summaryRole).toString();

    // Append percentage like QML: attached.data(summaryRole) + " (" + percent + "%)"
    int sizeRole = currentSizeRole();
    if (sizeRole >= 0 && model) {
        qreal total = model->data(root, sizeRole).toReal();
        if (total > 0) {
            qreal nodeVal = node.modelIndex.data(sizeRole).toReal();
            double pct = qRound(nodeVal / total * 1000.0) / 10.0;
            text += QLatin1String(" (")
                    + QString::number(pct, 'f', 1)
                    + QLatin1String("%)");
        }
    }
    return text;
}

int FlameGraphWidgetPrivate::detailsNodeIndex() const
{
    // currentNode = hoveredNode || selectedNode, keeping the last hovered if
    // nothing is selected (QML onMouseExited logic).
    int hovered = canvas ? canvas->m_hoveredNode : -1;
    if (hovered >= 0)
        return hovered;
    if (selectedNodeIndex >= 0)
        return selectedNodeIndex;
    return lastHoveredNodeIndex;
}

void FlameGraphWidgetPrivate::showDetails(int nodeIndex)
{
    if (nodeIndex < 0 || nodeIndex >= nodes.size()) {
        emit q->detailsCleared();
        return;
    }

    const LayoutNode &node = nodes.at(nodeIndex);

    QString title;
    if (detailsTitleRole >= 0 && node.modelIndex.isValid())
        title = node.modelIndex.data(detailsTitleRole).toString();
    if (title.isEmpty())
        title = summaryText(nodeIndex);
    if (title.isEmpty())
        title = FlameGraphWidget::tr("unknown");

    QList<QPair<QString, QString>> rows;
    if (node.modelIndex.isValid()) {
        for (const auto &[role, name] : std::as_const(detailsRoles)) {
            QVariant val = node.modelIndex.data(role);
            if (val.isNull() || !val.isValid())
                continue;
            QString valStr = val.toString();
            if (valStr.isEmpty())
                continue;
            rows.append({name, valStr});
        }
    } else if (!detailsRoles.isEmpty()) {
        // "others" node: show othersText as the single detail row
        QString label = detailsRoles.first().second;
        rows.append({label, othersText.isEmpty()
                                ? FlameGraphWidget::tr("Various Events") : othersText});
    }

    emit q->detailsChanged(title, rows);
}

// ---------------------------------------------------------------------------
// FlameGraphWidget

FlameGraphWidget::FlameGraphWidget(QAbstractItemModel *model, QWidget *parent)
    : QWidget(parent), d(new FlameGraphWidgetPrivate)
{
    d->q = this;
    d->model = model;

    d->canvas = new FlameGraphCanvas(d, nullptr);
    d->scrollArea = new QScrollArea(this);
    d->scrollArea->setWidget(d->canvas);
    d->scrollArea->setWidgetResizable(true);
    d->scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    d->scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    d->scrollArea->setFrameShape(QFrame::NoFrame);

    auto *topBar = new QWidget(this);
    topBar->setVisible(false);
    auto *barLayout = new QHBoxLayout(topBar);
    barLayout->setContentsMargins(2, 2, 2, 2);
    barLayout->addStretch();
    d->modeCombo = new QComboBox(topBar);
    d->modeCombo->setMinimumWidth(160);
    barLayout->addWidget(d->modeCombo);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(topBar);
    layout->addWidget(d->scrollArea);

    connect(d->modeCombo, &QComboBox::currentIndexChanged, this, [this](int) {
        d->rebuild();
    });

    if (model) {
        connect(model, &QAbstractItemModel::modelReset, this, [this] { d->rebuild(); });
        d->rebuild();
    }
}

FlameGraphWidget::~FlameGraphWidget()
{
    delete d;
}

QAbstractItemModel *FlameGraphWidget::model() const
{
    return d->model;
}

void FlameGraphWidget::setSizeRoles(const QList<QPair<int, QString>> &roles)
{
    d->sizeRoles = roles;
    QSignalBlocker blocker(d->modeCombo);
    d->modeCombo->clear();
    for (const auto &[role, name] : roles)
        d->modeCombo->addItem(name);

    // Show the toolbar only when there are multiple modes to choose from
    auto *topBar = d->modeCombo->parentWidget();
    topBar->setVisible(roles.size() > 1);

    d->rebuild();
}

void FlameGraphWidget::setTypeIdRole(int role)   { d->typeIdRole = role; }
void FlameGraphWidget::setSourceRoles(int fileRole, int lineRole, int columnRole)
{
    d->sourceFileRole = fileRole;
    d->sourceLineRole = lineRole;
    d->sourceColumnRole = columnRole;
}
void FlameGraphWidget::setSummaryRole(int role)       { d->summaryRole = role; }
void FlameGraphWidget::setDetailsTitleRole(int role)  { d->detailsTitleRole = role; }
void FlameGraphWidget::setDetailsRoles(const QList<QPair<int, QString>> &roles)
{
    d->detailsRoles = roles;
}
void FlameGraphWidget::setNoteRole(int role)   { d->noteRole = role; }
void FlameGraphWidget::setOthersText(const QString &text) { d->othersText = text; }

void FlameGraphWidget::selectByTypeId(int typeId)
{
    if (d->selectedTypeId == typeId)
        return;
    d->selectedTypeId = typeId;
    // Clear hover per QML onSelectedTypeIdChanged: tooltip.hoveredNode = null
    if (d->canvas)
        d->canvas->m_hoveredNode = -1;
    // Find the node for this typeId
    d->selectedNodeIndex = -1;
    d->lastHoveredNodeIndex = -1;
    if (typeId >= 0 && d->typeIdRole >= 0) {
        for (int i = 0; i < d->nodes.size(); ++i) {
            if (d->nodes.at(i).modelIndex.isValid()
                && d->nodes.at(i).modelIndex.data(d->typeIdRole).toInt() == typeId) {
                d->selectedNodeIndex = i;
                break;
            }
        }
    }
    d->showDetails(d->detailsNodeIndex());
    d->canvas->update();
}

void FlameGraphWidget::resetRoot()
{
    if (!d->root.isValid())
        return;
    d->root = QModelIndex();
    d->rebuild();
}

bool FlameGraphWidget::isZoomed() const
{
    return d->root.isValid();
}

} // namespace Timeline
