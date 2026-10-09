// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "calltreeview.h"

#include "calltreemodel.h"

#include "profilertr.h"

#include <coreplugin/minisplitter.h>

#include <utils/itemviews.h>
#include <utils/stylehelper.h>
#include <utils/theme/theme.h>

#include <QAbstractItemView>
#include <QBrush>
#include <QCache>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPaintEngine>
#include <QPainter>
#include <QPixmap>
#include <QScrollBar>
#include <QStyledItemDelegate>
#include <QTreeWidget>
#include <QVBoxLayout>

using namespace Utils;

namespace Profiler::Internal {

class CallTreeItemDelegate final : public QStyledItemDelegate
{
public:
    explicit CallTreeItemDelegate(QAbstractItemView *view)
        : QStyledItemDelegate(view)
        , m_view(view)
    {
        const auto clear = [this] { m_cache.clear(); };
        connect(view->model(), &QAbstractItemModel::dataChanged, this, clear);
        connect(view->model(), &QAbstractItemModel::modelAboutToBeReset, this, clear);
        connect(view->model(), &QAbstractItemModel::layoutAboutToBeChanged, this, clear);
        connect(view->model(), &QAbstractItemModel::rowsAboutToBeInserted, this, clear);
        connect(view->model(), &QAbstractItemModel::rowsAboutToBeRemoved, this, clear);
        connect(view->model(), &QAbstractItemModel::rowsAboutToBeMoved, this, clear);
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        const qreal pixelRatio = painter->device()->devicePixelRatioF();
        const QStyle *style = option.widget->style();
        QRect renderRect = option.rect;
        if (!painter->paintEngine()->systemClip().isEmpty()) {
            renderRect.setLeft(qMax(renderRect.left(), 0));
            renderRect.setRight(qMin(renderRect.right(), m_view->viewport()->width() - 1));
        }
        if (renderRect.isEmpty())
            return;
        const QRect sourceRect = renderRect.translated(-option.rect.topLeft());
        CachedItem *item = m_cache.object(index);
        if (!item || item->style != style || item->pixmap.devicePixelRatioF() != pixelRatio
            || !item->sourceRect.contains(sourceRect)
            || !matches(item->option, option)) {
            QRect cacheRect = renderRect;
            if (!painter->paintEngine()->systemClip().isEmpty()) {
                const int viewportWidth = m_view->viewport()->width();
                const int rowCount = m_view->viewport()->height() / option.rect.height() + 2;
                const int maximumWidth = m_cache.maxCost()
                                         / (rowCount * option.rect.height() * pixelRatio
                                            * pixelRatio * 4);
                const int margin = qBound(0, (maximumWidth - viewportWidth) / 2, viewportWidth);
                cacheRect.setLeft(qMax(option.rect.left(), renderRect.left() - margin));
                cacheRect.setRight(qMin(option.rect.right(), renderRect.right() + margin));
            }
            const QSize size = cacheRect.size() * pixelRatio;
            const qint64 cost = qint64(size.width()) * size.height() * 4;
            if (size.isEmpty() || cost > m_cache.maxCost()) {
                QStyledItemDelegate::paint(painter, option, index);
                return;
            }
            QPixmap pixmap(size);
            if (pixmap.isNull()) {
                QStyledItemDelegate::paint(painter, option, index);
                return;
            }
            pixmap.setDevicePixelRatio(pixelRatio);
            pixmap.fill(Qt::transparent);
            QStyleOptionViewItem cachedOption = option;
            cachedOption.rect.translate(-cacheRect.topLeft());
            {
                QPainter cachedPainter(&pixmap);
                QStyledItemDelegate::paint(&cachedPainter, cachedOption, index);
            }
            item = new CachedItem{option, cacheRect.translated(-option.rect.topLeft()), pixmap, style};
            m_cache.insert(index, item, int(cost));
        }
        painter->drawPixmap(option.rect.topLeft() + item->sourceRect.topLeft(), item->pixmap);
    }

private:
    struct CachedItem
    {
        QStyleOptionViewItem option;
        QRect sourceRect;
        QPixmap pixmap;
        const QStyle *style;
    };

    static bool matches(const QStyleOptionViewItem &cached, const QStyleOptionViewItem &option)
    {
        return cached.rect.size() == option.rect.size() && cached.state == option.state
               && cached.direction == option.direction && cached.font == option.font
               && cached.palette.cacheKey() == option.palette.cacheKey()
               && cached.features == option.features
               && cached.decorationSize == option.decorationSize
               && cached.decorationAlignment == option.decorationAlignment
               && cached.displayAlignment == option.displayAlignment
               && cached.decorationPosition == option.decorationPosition
               && cached.showDecorationSelected == option.showDecorationSelected
               && cached.textElideMode == option.textElideMode
               && cached.viewItemPosition == option.viewItemPosition;
    }

    QAbstractItemView *m_view;
    mutable QCache<QModelIndex, CachedItem> m_cache{16 * 1024 * 1024};
};

class CallStackTreeView final : public Utils::TreeView
{
public:
    CallStackTreeView(CallTreeModel *model, QWidget *parent)
        : Utils::TreeView(parent)
        , m_fixedColumns(new Utils::TreeView(this))
    {
        setModel(model);
        setItemDelegate(new CallTreeItemDelegate(this));
        setUniformRowHeights(true);
        setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        hideColumn(CallTreeModel::WeightColumn);
        hideColumn(CallTreeModel::SelfColumn);
        m_fixedColumns->setObjectName("SamplerCallTreeFixedColumns");
        m_fixedColumns->setModel(model);
        m_fixedColumns->setItemDelegate(new CallTreeItemDelegate(m_fixedColumns));
        m_fixedColumns->setSelectionModel(selectionModel());
        m_fixedColumns->setSelectionMode(QAbstractItemView::SingleSelection);
        m_fixedColumns->setUniformRowHeights(true);
        m_fixedColumns->setRootIsDecorated(false);
        m_fixedColumns->setIndentation(0);
        m_fixedColumns->setFocusPolicy(Qt::NoFocus);
        m_fixedColumns->setFrameShape(QFrame::NoFrame);
        m_fixedColumns->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        m_fixedColumns->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_fixedColumns->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_fixedColumns->setExpandsOnDoubleClick(false);
        m_fixedColumns->hideColumn(CallTreeModel::SymbolColumn);
        m_fixedColumns->header()->setStretchLastSection(false);
        m_fixedColumns->header()->setSectionResizeMode(QHeaderView::ResizeToContents);

        connect(header(), &QHeaderView::sectionResized,
                this, [this] { updateFixedColumnsGeometry(); });
        connect(m_fixedColumns->header(), &QHeaderView::sectionResized, this, [this] {
            header()->resizeSections(QHeaderView::ResizeToContents);
            updateFixedColumnsGeometry();
        });
        connect(this, &QTreeView::expanded, m_fixedColumns,
                [this](const QModelIndex &index) { m_fixedColumns->setExpanded(index, true); });
        connect(this, &QTreeView::collapsed, m_fixedColumns,
                [this](const QModelIndex &index) { m_fixedColumns->setExpanded(index, false); });
        connect(verticalScrollBar(), &QScrollBar::valueChanged,
                m_fixedColumns->verticalScrollBar(), &QScrollBar::setValue);
        connect(m_fixedColumns->verticalScrollBar(), &QScrollBar::valueChanged,
                verticalScrollBar(), &QScrollBar::setValue);
        connect(m_fixedColumns, &QAbstractItemView::doubleClicked,
                this, &QAbstractItemView::doubleClicked);
        updateFixedColumnsGeometry();
    }

    void scrollTo(const QModelIndex &index, ScrollHint hint = EnsureVisible) override
    {
        const int horizontalPosition = horizontalScrollBar()->value();
        Utils::TreeView::scrollTo(index.siblingAtColumn(CallTreeModel::SymbolColumn), hint);
        horizontalScrollBar()->setValue(horizontalPosition);
    }

private:
    int sizeHintForColumn(int column) const override
    {
        const int width = Utils::TreeView::sizeHintForColumn(column);
        return column == CallTreeModel::SymbolColumn
                   ? width + m_fixedColumns->header()->length() : width;
    }

    void updateGeometries() override
    {
        Utils::TreeView::updateGeometries();
        updateFixedColumnsGeometry();
    }

    void updateFixedColumnsGeometry()
    {
        const int width = m_fixedColumns->header()->length();
        m_fixedColumns->header()->setFixedHeight(header()->height());
        const int left = isRightToLeft() ? viewport()->geometry().left()
                : viewport()->geometry().right() - width + 1;
        m_fixedColumns->setGeometry(left,
                                    header()->geometry().top(), width,
                                    viewport()->geometry().bottom() - header()->geometry().top() + 1);
        m_fixedColumns->raise();
    }

    Utils::TreeView *m_fixedColumns;
};

// A one-line colour key explaining the QML/JS vs C++ frame tint. Shown only for
// merged native-mixed traces (see CallTreeModel::hasJsFrames).
static QWidget *createLegend(QWidget *parent)
{
    auto legend = new QWidget(parent);
    // A one-line key: keep it at its natural height so it never steals vertical
    // space from the tree when the view is resized.
    legend->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    auto layout = new QHBoxLayout(legend);
    layout->setContentsMargins(StyleHelper::SpacingTokens::PaddingHM,
                               StyleHelper::SpacingTokens::PaddingVXs,
                               StyleHelper::SpacingTokens::PaddingHM,
                               StyleHelper::SpacingTokens::PaddingVXs);
    layout->setSpacing(StyleHelper::SpacingTokens::GapHS);

    const auto addEntry = [&](Theme::Color color, const QString &text) {
        auto swatch = new QLabel(legend);
        swatch->setFixedSize(10, 10);
        StyleHelper::setBackgroundColor(swatch, color);
        auto label = new QLabel(text, legend);
        label->setFont(StyleHelper::uiFont(StyleHelper::UiElementCaption));
        layout->addWidget(swatch);
        layout->addWidget(label);
    };

    addEntry(Theme::Token_Text_Accent, Tr::tr("QML / JS"));
    layout->addSpacing(StyleHelper::SpacingTokens::GapHM);
    addEntry(Theme::Token_Text_Default, Tr::tr("C++ (native)"));
    layout->addStretch();
    return legend;
}

CallTreeView::CallTreeView(CallTreeModel *model, QWidget *parent)
    : QWidget(parent)
    , m_model(model)
{
    m_tree = new CallStackTreeView(model, this);
    m_tree->setObjectName("SamplerCallTree");
    m_tree->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tree->setUniformRowHeights(true);
    m_tree->header()->setStretchLastSection(false);
    m_tree->header()->setSectionResizeMode(QHeaderView::ResizeToContents);

    m_heaviest = new QTreeWidget(this);
    m_heaviest->setObjectName("SamplerHeaviestStack");
    m_heaviest->setColumnCount(2);
    m_heaviest->setItemDelegate(new CallTreeItemDelegate(m_heaviest));
    m_heaviest->setHeaderLabels({Tr::tr("Weight"), Tr::tr("Heaviest Stack")});
    m_heaviest->setRootIsDecorated(false);
    m_heaviest->setUniformRowHeights(true);
    m_heaviest->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_heaviest->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_heaviest->header()->setStretchLastSection(false);

    auto splitter = new Core::MiniSplitter(Qt::Horizontal);
    splitter->addWidget(m_tree);
    splitter->addWidget(m_heaviest);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 1);

    m_legend = createLegend(this);
    m_legend->setVisible(m_model->hasJsFrames());

    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_legend);
    layout->addWidget(splitter);

    connect(m_tree->selectionModel(), &QItemSelectionModel::currentChanged,
            this, [this] { updateHeaviestStack(); });
    connect(m_tree, &QAbstractItemView::doubleClicked,
            this, &CallTreeView::emitGotoForIndex);
    connect(m_heaviest, &QTreeWidget::currentItemChanged,
            this, [this](QTreeWidgetItem *cur, QTreeWidgetItem *) { onHeaviestCurrentChanged(cur); });
    connect(m_heaviest, &QTreeWidget::itemDoubleClicked,
            this, [this](QTreeWidgetItem *item, int) { onHeaviestDoubleClicked(item); });
    connect(model, &QAbstractItemModel::modelReset, this, [this] {
        m_legend->setVisible(m_model->hasJsFrames());
        updateHeaviestStack();
    });

    updateHeaviestStack();
}

void CallTreeView::updateHeaviestStack()
{
    if (m_syncingSelection) // a heaviest->tree click must not re-root the pane
        return;
    m_heaviest->clear();
    const CallTreeModel::Node *from = m_model->node(m_tree->currentIndex());
    const QList<const CallTreeModel::Node *> path = m_model->heaviestPath(from);
    const QBrush jsBrush(Utils::creatorColor(Utils::Theme::Token_Text_Accent));
    for (const CallTreeModel::Node *node : path) {
        auto item = new QTreeWidgetItem(m_heaviest);
        item->setText(0, QString::number(node->weight));
        item->setTextAlignment(0, Qt::AlignRight | Qt::AlignVCenter);
        item->setText(1, m_model->symbol(node));
        // Match the tree: QML/JS frames are tinted with the accent colour.
        if (m_model->isJsFrame(node))
            item->setForeground(1, jsBrush);
        item->setData(0, Qt::UserRole,
                      QVariant::fromValue(QPersistentModelIndex(m_model->indexFor(node))));
    }
}

void CallTreeView::emitGotoForIndex(const QModelIndex &index)
{
    const CallTreeModel::Node *node = m_model->node(index);
    if (!node)
        return;
    const CallTreeModel::SourceLocation loc = m_model->location(node);
    if (loc.file.isEmpty() && loc.module.isEmpty() && loc.offset == 0)
        return; // nothing to point at
    // Emit source (when known) and module/offset (always available for sampled
    // frames) together; the consumer uses whichever it needs.
    emit gotoSourceLocation(loc.file, loc.line, 0, loc.module, loc.offset);
}

void CallTreeView::onHeaviestCurrentChanged(QTreeWidgetItem *item)
{
    if (!item)
        return;
    const QModelIndex index = item->data(0, Qt::UserRole).value<QPersistentModelIndex>();
    if (!index.isValid())
        return;
    // Navigate the tree to the frame without rebuilding (and thus re-rooting)
    // the heaviest pane, so a double-click's two clicks land on the same row.
    m_syncingSelection = true;
    m_tree->setCurrentIndex(index);
    m_tree->scrollTo(index);
    m_syncingSelection = false;
}

void CallTreeView::onHeaviestDoubleClicked(QTreeWidgetItem *item)
{
    if (!item)
        return;
    emitGotoForIndex(item->data(0, Qt::UserRole).value<QPersistentModelIndex>());
}

} // namespace Profiler::Internal
