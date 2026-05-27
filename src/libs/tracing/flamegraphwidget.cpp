// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "flamegraphwidget.h"

#include <utils/theme/theme.h>
#include <utils/utilsicons.h>

#include <QAbstractItemModel>
#include <QComboBox>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QModelIndex>
#include <QMouseEvent>
#include <QPainter>
#include <QResizeEvent>
#include <QScrollArea>
#include <QStyle>
#include <QToolButton>
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
// RangeDetails panel — mirrors RangeDetails.qml

class RangeDetails : public QWidget
{
    Q_OBJECT
public:
    explicit RangeDetails(QWidget *parent);

    void setTitle(const QString &title);
    void setContent(const QList<QPair<QString, QString>> &rows, const QString &note);
    bool isLocked() const { return m_lockButton->isChecked(); }

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    // outerMargin=10, innerMargin=5, titleBarHeight ~ toolBarHeight/1.2 ~ 24
    static constexpr int kTitleHeight = 24;
    static constexpr int kOuterMargin = 10;
    static constexpr int kInnerMargin = 5;

    QWidget *m_titleBar = nullptr;
    QLabel *m_titleLabel = nullptr;
    QToolButton *m_lockButton = nullptr;
    QToolButton *m_collapseButton = nullptr;
    QWidget *m_content = nullptr;
    QGridLayout *m_grid = nullptr;
    QLabel *m_noteLabel = nullptr;

    bool m_collapsed = false;
    QPoint m_dragOffset;
    bool m_dragging = false;
};

RangeDetails::RangeDetails(QWidget *parent)
    : QWidget(parent)
{
    // initialWidth: 300, minimumInnerWidth: 150
    setFixedWidth(300);

    // The outer widget auto-fills with PanelTextColorMid; the 1px margins become
    // the outer border and the 1px layout spacing becomes the title/content separator,
    // matching QML's separate bordered rectangles for titleBar and contentArea.
    setAutoFillBackground(true);
    {
        QPalette pal = palette();
        pal.setColor(QPalette::Window,
                     Utils::creatorColor(Utils::Theme::PanelTextColorMid));
        setPalette(pal);
    }

    // --- Title bar ---
    m_titleBar = new QWidget(this);
    m_titleBar->setFixedHeight(kTitleHeight);
    m_titleBar->setAutoFillBackground(true);
    {
        QPalette pal = m_titleBar->palette();
        pal.setColor(QPalette::Window,
                     Utils::creatorColor(Utils::Theme::Timeline_PanelHeaderColor));
        m_titleBar->setPalette(pal);
    }

    m_titleLabel = new QLabel(m_titleBar);
    {
        QFont f = m_titleLabel->font();
        f.setBold(true);
        m_titleLabel->setFont(f);
        QPalette pal = m_titleLabel->palette();
        pal.setColor(QPalette::WindowText,
                     Utils::creatorColor(Utils::Theme::PanelTextColorLight));
        m_titleLabel->setPalette(pal);
    }
    m_titleLabel->setAutoFillBackground(false);

    // Lock: lock_open (unlocked) / lock_closed (locked) — matches imageSource in QML
    m_lockButton = new QToolButton(m_titleBar);
    m_lockButton->setCheckable(true);
    m_lockButton->setChecked(false);
    m_lockButton->setAutoRaise(true);
    m_lockButton->setFixedSize(kTitleHeight, kTitleHeight);
    m_lockButton->setIconSize(QSize(16, 16));
    m_lockButton->setIcon(Utils::Icons::UNLOCKED_TOOLBAR.icon());
    m_lockButton->setToolTip(tr("View event information on mouseover."));
    connect(m_lockButton, &QToolButton::toggled, this, [this](bool checked) {
        m_lockButton->setIcon(checked ? Utils::Icons::LOCKED_TOOLBAR.icon()
                                      : Utils::Icons::UNLOCKED_TOOLBAR.icon());
    });

    // Collapse/expand — arrowup/arrowdown in QML
    m_collapseButton = new QToolButton(m_titleBar);
    m_collapseButton->setAutoRaise(true);
    m_collapseButton->setFixedSize(kTitleHeight, kTitleHeight);
    m_collapseButton->setIconSize(QSize(16, 16));
    m_collapseButton->setIcon(style()->standardIcon(QStyle::SP_ArrowUp));
    m_collapseButton->setToolTip(tr("Collapse"));
    connect(m_collapseButton, &QToolButton::clicked, this, [this] {
        m_collapsed = !m_collapsed;
        m_content->setVisible(!m_collapsed);
        m_collapseButton->setIcon(style()->standardIcon(
            m_collapsed ? QStyle::SP_ArrowDown : QStyle::SP_ArrowUp));
        m_collapseButton->setToolTip(m_collapsed ? tr("Expand") : tr("Collapse"));
        adjustSize();
        setFixedWidth(300);
    });

    auto *titleLayout = new QHBoxLayout(m_titleBar);
    titleLayout->setContentsMargins(kOuterMargin, 0, 0, 0);
    titleLayout->setSpacing(0);
    titleLayout->addWidget(m_titleLabel, 1);
    titleLayout->addWidget(m_lockButton);
    titleLayout->addWidget(m_collapseButton);

    // --- Content area ---
    m_content = new QWidget(this);
    m_content->setAutoFillBackground(true);
    {
        QPalette pal = m_content->palette();
        pal.setColor(QPalette::Window,
                     Utils::creatorColor(Utils::Theme::Timeline_PanelBackgroundColor));
        m_content->setPalette(pal);
    }

    m_grid = new QGridLayout(m_content);
    m_grid->setContentsMargins(kOuterMargin, kInnerMargin, kOuterMargin, kInnerMargin);
    m_grid->setHorizontalSpacing(kInnerMargin);
    m_grid->setVerticalSpacing(kInnerMargin);
    m_grid->setColumnStretch(1, 1);

    m_noteLabel = new QLabel(m_content);
    m_noteLabel->setWordWrap(true);
    m_noteLabel->hide();
    {
        QFont nf = m_noteLabel->font();
        nf.setItalic(true);
        m_noteLabel->setFont(nf);
        QPalette pal = m_noteLabel->palette();
        pal.setColor(QPalette::WindowText,
                     Utils::creatorColor(Utils::Theme::Timeline_HighlightColor));
        m_noteLabel->setPalette(pal);
    }

    auto *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(1, 1, 1, 1);
    outerLayout->setSpacing(1);  // 1px gap = visible separator between title and content
    outerLayout->addWidget(m_titleBar);
    outerLayout->addWidget(m_content);
}

void RangeDetails::setTitle(const QString &title)
{
    m_titleLabel->setText(title);
}

void RangeDetails::setContent(const QList<QPair<QString, QString>> &rows, const QString &note)
{
    while (m_grid->count() > 0) {
        QLayoutItem *item = m_grid->takeAt(0);
        delete item->widget();
        delete item;
    }

    // Labels are bold per Detail.qml (font.bold: isLabel)
    int row = 0;
    for (const auto &[label, value] : rows) {
        auto *lbl = new QLabel(label + QLatin1Char(':'), m_content);
        lbl->setAlignment(Qt::AlignRight | Qt::AlignTop);
        {
            QFont f = lbl->font();
            f.setBold(true);
            lbl->setFont(f);
            QPalette pal = lbl->palette();
            pal.setColor(QPalette::WindowText,
                         Utils::creatorColor(Utils::Theme::PanelTextColorLight));
            lbl->setPalette(pal);
        }
        auto *val = new QLabel(value, m_content);
        val->setWordWrap(true);
        val->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        {
            QPalette pal = val->palette();
            pal.setColor(QPalette::WindowText,
                         Utils::creatorColor(Utils::Theme::PanelTextColorLight));
            val->setPalette(pal);
        }
        m_grid->addWidget(lbl, row, 0);
        m_grid->addWidget(val, row, 1);
        ++row;
    }

    if (!note.isEmpty()) {
        m_noteLabel->setText(note);
        m_grid->addWidget(m_noteLabel, row, 0, 1, 2);
        m_noteLabel->show();
    } else {
        m_noteLabel->hide();
    }

    if (!m_collapsed)
        adjustSize();
    setFixedWidth(300);
}

void RangeDetails::paintEvent(QPaintEvent *event)
{
    // Background auto-fill (PanelTextColorMid) provides the outer border and
    // title/content separator via layout margins and spacing. Delegate the rest.
    QWidget::paintEvent(event);
}

void RangeDetails::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton
        && m_titleBar->geometry().contains(event->pos())) {
        m_dragging = true;
        m_dragOffset = event->pos();
        event->accept();
    }
}

void RangeDetails::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_dragging)
        return;
    QPoint newPos = pos() + event->pos() - m_dragOffset;
    if (parentWidget()) {
        QRect pr = parentWidget()->rect();
        newPos.setX(qBound(0, newPos.x(), pr.width() - width()));
        newPos.setY(qBound(0, newPos.y(), pr.height() - height()));
    }
    move(newPos);
    event->accept();
}

void RangeDetails::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        m_dragging = false;
}

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

    // UI
    QScrollArea *scrollArea = nullptr;
    FlameGraphCanvas *canvas = nullptr;
    QComboBox *modeCombo = nullptr;
    RangeDetails *detailsPanel = nullptr;

    int currentSizeRole() const
    {
        int idx = modeCombo ? modeCombo->currentIndex() : 0;
        if (idx >= 0 && idx < sizeRoles.size())
            return sizeRoles.at(idx).first;
        return sizeRoles.isEmpty() ? -1 : sizeRoles.first().first;
    }

    void rebuild();

    QVector<QRectF> computeRects(qreal canvasWidth, int rowHeight) const;
    QColor colorForNode(int i) const;
    QString summaryText(int i) const;

    // currentNode: hoveredNode (when unlocked) || selectedNode || lastHoveredNode
    int detailsNodeIndex() const;
    void showDetails(int nodeIndex);
    void repositionDetails();  // clamp to parent bounds; does not reset position
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
        return {100, d->treeDepth * kMinRowHeight};
    }

    int m_hoveredNode = -1;

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    static constexpr int kMinRowHeight = 30;
    static constexpr int kMaxRowHeight = 60;

    int rowHeight() const
    {
        return qBound(kMinRowHeight, height() / qMax(1, d->treeDepth), kMaxRowHeight);
    }

    int nodeAt(const QPoint &pos) const
    {
        if (d->nodes.isEmpty())
            return -1;
        int rh = rowHeight();
        auto rects = d->computeRects(width(), rh);
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
    auto rects = d->computeRects(width(), rh);

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

        QColor fill = d->colorForNode(i);
        if (selected)
            fill = fill.lighter(130);
        else if (hovered)
            fill = fill.lighter(115);

        // Colors match QML:
        //   selected  → blue2 = rgba(0.375, 0, 1, 1), width 2 (selected IS currentNode)
        //   hovered   → blue1 = lighter("blue"),       width 2
        //   has note  → Timeline_HighlightColor,        width 3
        //   default   → #B0B0B0,                        width 1
        static const QColor blue1 = QColor("blue").lighter();
        static const QColor blue2 = QColor::fromRgbF(0.375f, 0.0f, 1.0f);
        static const QColor grey1 = QColor("#B0B0B0");
        QColor border;
        int borderWidth;
        if (hovered) {
            border = blue1;
            borderWidth = 2;
        } else if (selected) {
            // Selected node is currentNode when nothing is hovered → width 2
            border = blue2;
            borderWidth = 2;
        } else if (hasNote) {
            border = Utils::creatorColor(Utils::Theme::Timeline_HighlightColor);
            borderWidth = 3;
        } else {
            border = grey1;
            borderWidth = 1;
        }

        p.fillRect(r, fill);
        p.setPen(QPen(border, borderWidth));
        p.drawRect(r.adjusted(0, 0, -1, -1));

        if (r.width() > 20 || selected) {
            p.save();
            if (selected)
                p.setFont(boldFont);
            p.setPen(Utils::creatorColor(Utils::Theme::PanelTextColorLight));
            p.setClipRect(r.adjusted(3, 2, -3, -2));
            p.drawText(r.adjusted(3, 2, -3, -2),
                       Qt::AlignVCenter | Qt::AlignHCenter | Qt::TextWordWrap,
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

QVector<QRectF> FlameGraphWidgetPrivate::computeRects(qreal canvasWidth, int rowHeight) const
{
    QVector<QRectF> rects(nodes.size());
    for (int i = 0; i < nodes.size(); ++i) {
        const LayoutNode &node = nodes.at(i);
        QRectF parentRect = node.parentNodeIndex < 0
                                ? QRectF(0, 0, canvasWidth, treeDepth * rowHeight)
                                : rects.at(node.parentNodeIndex);
        qreal x = parentRect.x() + node.relativePosition * parentRect.width();
        qreal w = node.relativeSize * parentRect.width();
        qreal y = (treeDepth - 1 - node.depth) * rowHeight;
        rects[i] = QRectF(x, y, w, rowHeight);
    }
    return rects;
}

QColor FlameGraphWidgetPrivate::colorForNode(int i) const
{
    const LayoutNode &node = nodes.at(i);
    if (!node.modelIndex.isValid())
        return Utils::creatorColor(Utils::Theme::Timeline_BackgroundColor2);

    int typeId = (typeIdRole >= 0) ? node.modelIndex.data(typeIdRole).toInt() : i;
    // Hue in 0..55 degree band (matching QML scheme), shifted by typeId
    int hue = (node.depth % 12) * 5 + (((typeId * 97) & 0xFF) % 5);
    return QColor::fromHsl(hue, 230, 115);
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
    // Mirrors QML: currentNode = !locked && hoveredNode || selectedNode
    // Plus: keep last hovered if nothing selected (QML onMouseExited logic)
    bool locked = detailsPanel && detailsPanel->isLocked();
    int hovered = canvas ? canvas->m_hoveredNode : -1;
    if (hovered >= 0 && !locked)
        return hovered;
    if (selectedNodeIndex >= 0)
        return selectedNodeIndex;
    return lastHoveredNodeIndex;
}

void FlameGraphWidgetPrivate::showDetails(int nodeIndex)
{
    if (!detailsPanel)
        return;

    if (nodeIndex < 0 || nodeIndex >= nodes.size()) {
        // QML: show "No data available" when model has no rows; else hide.
        bool noData = !model || model->rowCount() == 0;
        if (noData) {
            detailsPanel->setTitle(FlameGraphWidget::tr("No data available"));
            detailsPanel->setContent({}, {});
        } else {
            detailsPanel->hide();
            return;
        }
    } else {
        const LayoutNode &node = nodes.at(nodeIndex);

        QString title;
        if (detailsTitleRole >= 0 && node.modelIndex.isValid())
            title = node.modelIndex.data(detailsTitleRole).toString();
        if (title.isEmpty())
            title = summaryText(nodeIndex);
        // QML fallback: "unknown"
        if (title.isEmpty())
            title = FlameGraphWidget::tr("unknown");
        detailsPanel->setTitle(title);

        QList<QPair<QString, QString>> rows;
        if (node.modelIndex.isValid()) {
            for (const auto &[role, name] : detailsRoles) {
                QVariant val = node.modelIndex.data(role);
                if (val.isNull() || !val.isValid())
                    continue;
                QString valStr = val.toString();
                if (valStr.isEmpty())
                    continue;
                rows.append({name, valStr});
            }
        } else {
            // "others" node: show othersText as the single detail row
            if (!detailsRoles.isEmpty()) {
                QString label = detailsRoles.first().second;
                rows.append({label, othersText.isEmpty()
                                        ? FlameGraphWidget::tr("Various Events") : othersText});
            }
        }

        QString note;
        if (noteRole >= 0 && node.modelIndex.isValid())
            note = node.modelIndex.data(noteRole).toString();
        detailsPanel->setContent(rows, note);
    }

    // On first show, place at upper-left (QML default x=0, y=0 within flickable)
    if (!detailsPanel->isVisible()) {
        int margin = 8;
        int topOffset = 0;
        if (modeCombo) {
            QWidget *topBar = modeCombo->parentWidget();
            if (topBar && topBar->isVisible())
                topOffset = topBar->height();
        }
        detailsPanel->move(margin, margin + topOffset);
    }

    detailsPanel->show();
    detailsPanel->raise();
    repositionDetails();
}

void FlameGraphWidgetPrivate::repositionDetails()
{
    if (!detailsPanel || !detailsPanel->isVisible())
        return;
    // Clamp to stay inside parent, without resetting user-chosen position
    QRect pr = q->rect();
    QPoint p = detailsPanel->pos();
    p.setX(qBound(0, p.x(), qMax(0, pr.width() - detailsPanel->width())));
    p.setY(qBound(0, p.y(), qMax(0, pr.height() - detailsPanel->height())));
    detailsPanel->move(p);
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

    d->detailsPanel = new RangeDetails(this);
    d->detailsPanel->hide();

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

void FlameGraphWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    d->repositionDetails();
}

} // namespace Timeline

#include "flamegraphwidget.moc"
