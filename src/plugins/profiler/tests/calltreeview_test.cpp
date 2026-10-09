// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "calltreeview_test.h"

#include <coreplugin/manhattanstyle.h>

#include <profiler/calltreemodel.h>
#include <profiler/calltreeview.h>

#include <QHeaderView>
#include <QPainter>
#include <QScrollBar>
#include <QStandardItemModel>
#include <QStyleFactory>
#include <QStyleOption>
#include <QStyledItemDelegate>
#include <QTest>
#include <QTreeView>
#include <QTreeWidget>

#include <memory>

namespace Profiler::Internal {

class CallTreeBranchBaseStyle final : public QProxyStyle
{
public:
    explicit CallTreeBranchBaseStyle(const QString &styleName)
        : QProxyStyle(styleName)
    {}

    void drawPrimitive(PrimitiveElement element, const QStyleOption *option,
                       QPainter *painter, const QWidget *widget) const override
    {
        if (element == PE_IndicatorBranch) {
            ++branchCalls;
            if (widget && !option->rect.intersects(widget->rect()))
                ++offscreenBranchCalls;
        }
        QProxyStyle::drawPrimitive(element, option, painter, widget);
    }

    mutable int branchCalls = 0;
    mutable int offscreenBranchCalls = 0;
};

class CallTreeBranchProbeStyle final : public Core::ManhattanStyle
{
public:
    CallTreeBranchProbeStyle(const QString &styleName, CallTreePanelProbeWidget &widget)
        : Core::ManhattanStyle(styleName)
        , m_widget(widget)
    {}

    void drawPrimitive(PrimitiveElement element, const QStyleOption *option,
                       QPainter *painter, const QWidget *widget) const override
    {
        const int initialReads = m_widget.panelReads;
        Core::ManhattanStyle::drawPrimitive(element, option, painter, widget);
        if (element == PE_IndicatorBranch) {
            ++branchCalls;
            branchPanelReads += m_widget.panelReads - initialReads;
        }
    }

    mutable int branchCalls = 0;
    mutable int branchPanelReads = 0;

    void drawControl(ControlElement element, const QStyleOption *option,
                     QPainter *painter, const QWidget *widget) const override
    {
        if (element == CE_ItemViewItem)
            ++itemPaintCalls;
        Core::ManhattanStyle::drawControl(element, option, painter, widget);
    }

    mutable int itemPaintCalls = 0;

private:
    CallTreePanelProbeWidget &m_widget;
};

void CallTreeViewTest::testBranchPainting_data()
{
    QTest::addColumn<QString>("styleName");
    QTest::addColumn<Qt::LayoutDirection>("direction");
    QTest::newRow("fusion-left-to-right") << QString("fusion") << Qt::LeftToRight;
    QTest::newRow("fusion-right-to-left") << QString("fusion") << Qt::RightToLeft;
    if (QStyleFactory::keys().contains("macos", Qt::CaseInsensitive)) {
        QTest::newRow("macos-left-to-right") << QString("macos") << Qt::LeftToRight;
        QTest::newRow("macos-right-to-left") << QString("macos") << Qt::RightToLeft;
    }
}

void CallTreeViewTest::testBranchPainting()
{
    QFETCH(QString, styleName);
    QFETCH(Qt::LayoutDirection, direction);
    SampleTraceData data;
    SampleTraceData::ThreadSample sample;
    sample.tid = 1;
    sample.running = true;
    for (int frame = 0; frame < 256; ++frame) {
        data.labels.append(QString("Namespace::Class%1::function("
                                   "std::vector<std::pair<QString, QByteArray>> const &, int)")
                               .arg(frame));
        sample.frames.append(frame);
    }
    data.samples.append(sample);
    CallTreeModel model;
    model.setTraceData(&data);
    CallTreePanelProbeWidget parent;
    parent.setLayoutDirection(direction);
    CallTreeBranchProbeStyle style(styleName, parent);
    auto baseStyle = new CallTreeBranchBaseStyle(styleName);
    style.setBaseStyle(baseStyle);
    CallTreeView view(&model, &parent);
    view.resize(500, 300);
    auto tree = view.findChild<QTreeView *>("SamplerCallTree");
    QVERIFY(tree);
    tree->setStyle(&style);
    view.show();
    parent.show();
    QVERIFY(QTest::qWaitForWindowExposed(&parent));
    tree->expandAll();
    QTRY_VERIFY(tree->verticalScrollBar()->maximum() > 0);
    tree->verticalScrollBar()->setValue(tree->verticalScrollBar()->maximum());
    tree->horizontalScrollBar()->setValue(tree->horizontalScrollBar()->maximum());
    QModelIndex lastIndex = model.index(0, 0);
    while (model.rowCount(lastIndex) > 0)
        lastIndex = model.index(0, 0, lastIndex);
    const QRect lastRect = tree->visualRect(lastIndex);
    const int offset = direction == Qt::LeftToRight
                           ? lastRect.left() - tree->indentation() - tree->viewport()->width() / 2
                           : tree->viewport()->width() / 2 - tree->indentation() - lastRect.right() - 1;
    tree->horizontalScrollBar()->setValue(tree->horizontalScrollBar()->value() + offset);
    style.branchCalls = 0;
    style.branchPanelReads = 0;
    baseStyle->branchCalls = 0;
    baseStyle->offscreenBranchCalls = 0;
    tree->viewport()->repaint();
    QVERIFY(style.branchCalls > 0);
    QVERIFY(baseStyle->branchCalls > 0);
    QCOMPARE(baseStyle->offscreenBranchCalls, 0);
    QVERIFY(baseStyle->branchCalls < style.branchCalls / 4);
    QCOMPARE(style.branchPanelReads, 0);
    style.itemPaintCalls = 0;
    tree->viewport()->repaint();
    QCOMPARE(style.itemPaintCalls, 0);

    style.setBaseStyle(QStyleFactory::create(styleName));
    if (styleName == "macos")
        QVERIFY2(style.baseStyle()->inherits("QMacStyle"), style.baseStyle()->metaObject()->className());
    tree->viewport()->repaint();
    QBENCHMARK {
        tree->viewport()->repaint();
    }
}

void CallTreeViewTest::testCellPainting_data()
{
    testBranchPainting_data();
}

void CallTreeViewTest::testBranchPrimitives_data()
{
    testBranchPainting_data();
}

void CallTreeViewTest::testRowBackgrounds_data()
{
    testBranchPainting_data();
}

void CallTreeViewTest::testRowBackgrounds()
{
    QFETCH(QString, styleName);
    QFETCH(Qt::LayoutDirection, direction);
    Core::ManhattanStyle style(styleName);
    const std::unique_ptr<QStyle> reference(QStyleFactory::create(styleName));
    QVERIFY(reference);
    QStandardItemModel model(4, 1);
    for (int row = 0; row < model.rowCount(); ++row)
        model.setData(model.index(row, 0), QString("Frame %1").arg(row));
    CallTreePanelProbeView widget;
    widget.setModel(&model);
    widget.setStyle(&style);
    widget.setLayoutDirection(direction);
    widget.setCurrentIndex(model.index(1, 0));
    widget.resize(300, 200);
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    QStyleOptionViewItem option;
    option.initFrom(&widget);
    option.widget = &widget;
    option.index = model.index(1, 0);
    option.direction = direction;
    option.rect = QRect(10, 10, 80, 20);
    const QList<QStyle::State> states{
        QStyle::State_None,
        QStyle::State_Enabled,
        QStyle::State_Enabled | QStyle::State_Active,
        QStyle::State_Enabled | QStyle::State_Selected,
        QStyle::State_Enabled | QStyle::State_Selected | QStyle::State_Active,
        QStyle::State_Enabled | QStyle::State_Selected | QStyle::State_HasFocus,
    };
    const auto render = [&](QStyle *renderStyle) {
        const qreal pixelRatio = widget.devicePixelRatioF();
        QImage image(QSize(100, 40) * pixelRatio, QImage::Format_ARGB32_Premultiplied);
        image.setDevicePixelRatio(pixelRatio);
        image.fill(Qt::transparent);
        QPainter painter(&image);
        renderStyle->drawPrimitive(QStyle::PE_PanelItemViewRow, &option, &painter, &widget);
        return image;
    };
    for (bool enabled : {false, true}) {
        widget.setEnabled(enabled);
        for (bool alternate : {false, true}) {
            option.features.setFlag(QStyleOptionViewItem::Alternate, alternate);
            widget.setAlternatingRowColors(alternate);
            for (bool decorationSelected : {false, true}) {
                option.showDecorationSelected = decorationSelected;
                widget.setAllColumnsShowFocus(decorationSelected);
                for (QStyle::State state : states) {
                    option.state = state;
                    widget.panelReads = 0;
                    const QImage actual = render(&style);
                    const int panelReads = widget.panelReads;
                    QCOMPARE(actual, render(reference.get()));
                    QCOMPARE(panelReads, 0);
                }
                const QImage actual = widget.viewport()->grab().toImage();
                widget.setStyle(reference.get());
                QCOMPARE(actual, widget.viewport()->grab().toImage());
                widget.setStyle(&style);
            }
        }
    }
}

void CallTreeViewTest::testBranchPrimitives()
{
    QFETCH(QString, styleName);
    QFETCH(Qt::LayoutDirection, direction);
    QWidget widget;
    Core::ManhattanStyle style(styleName);
    const std::unique_ptr<QStyle> reference(QStyleFactory::create(styleName));
    QVERIFY(reference);
    QStyleOption option;
    option.initFrom(&widget);
    option.direction = direction;
    option.rect = QRect(10, 10, 20, 20);
    const QList<QStyle::State> states{
        QStyle::State_None,
        QStyle::State_Enabled,
        QStyle::State_Enabled | QStyle::State_Sibling,
        QStyle::State_Enabled | QStyle::State_Item,
        QStyle::State_Enabled | QStyle::State_Selected | QStyle::State_HasFocus,
        QStyle::State_Enabled | QStyle::State_Item | QStyle::State_Children,
        QStyle::State_Enabled | QStyle::State_Item | QStyle::State_Children | QStyle::State_Open,
        QStyle::State_Enabled | QStyle::State_Item | QStyle::State_Children | QStyle::State_Open
            | QStyle::State_Selected | QStyle::State_HasFocus,
    };
    const auto render = [&](QStyle *renderStyle) {
        const qreal pixelRatio = widget.devicePixelRatioF();
        QImage image(QSize(40, 40) * pixelRatio, QImage::Format_ARGB32_Premultiplied);
        image.setDevicePixelRatio(pixelRatio);
        image.fill(Qt::transparent);
        QPainter painter(&image);
        renderStyle->drawPrimitive(QStyle::PE_IndicatorBranch, &option, &painter, &widget);
        return image;
    };
    for (QStyle::State state : states) {
        option.state = state;
        QCOMPARE(render(&style), render(reference.get()));
    }
}

void CallTreeViewTest::testWideColumnCache_data()
{
    testBranchPainting_data();
}

void CallTreeViewTest::testWideColumnCache()
{
    QFETCH(QString, styleName);
    QFETCH(Qt::LayoutDirection, direction);
    SampleTraceData data;
    SampleTraceData::ThreadSample sample;
    sample.tid = 1;
    sample.running = true;
    for (int frame = 0; frame < 64; ++frame) {
        data.labels.append(QString::number(frame));
        sample.frames.append(frame);
    }
    data.samples.append(sample);
    CallTreeModel model;
    model.setTraceData(&data);
    CallTreePanelProbeWidget parent;
    parent.setLayoutDirection(direction);
    CallTreeBranchProbeStyle style(styleName, parent);
    CallTreeView view(&model, &parent);
    view.resize(600, 360);
    auto tree = view.findChild<QTreeView *>("SamplerCallTree");
    QVERIFY(tree);
    tree->setStyle(&style);
    view.show();
    parent.show();
    QVERIFY(QTest::qWaitForWindowExposed(&parent));
    tree->expandAll();
    tree->header()->setSectionResizeMode(QHeaderView::Fixed);
    tree->header()->resizeSection(CallTreeModel::SymbolColumn, 32768);
    QTRY_VERIFY(tree->verticalScrollBar()->maximum() > 0);
    tree->verticalScrollBar()->setValue(tree->verticalScrollBar()->maximum() / 2);
    tree->horizontalScrollBar()->setValue(tree->indentation() * 32);
    QTest::mouseMove(&parent, QPoint(parent.width() - 1, parent.height() - 1));
    tree->viewport()->repaint();
    style.itemPaintCalls = 0;
    tree->viewport()->repaint();
    QCOMPARE(style.itemPaintCalls, 0);

    QAbstractItemDelegate *cachedDelegate = tree->itemDelegate();
    QStyledItemDelegate reference;
    const auto compareViewport = [&] {
        const QImage cached = tree->viewport()->grab().toImage();
        tree->setItemDelegate(&reference);
        const QImage original = tree->viewport()->grab().toImage();
        tree->setItemDelegate(cachedDelegate);
        QCOMPARE(cached, original);
    };
    const int initialPosition = tree->horizontalScrollBar()->value();
    style.itemPaintCalls = 0;
    for (int offset = 1; offset <= 32; ++offset) {
        tree->horizontalScrollBar()->setValue(initialPosition + offset);
        tree->viewport()->repaint();
    }
    QCOMPARE(style.itemPaintCalls, 0);
    compareViewport();
    tree->horizontalScrollBar()->setValue(initialPosition);
    style.itemPaintCalls = 0;
    tree->viewport()->repaint();
    QCOMPARE(style.itemPaintCalls, 0);
    compareViewport();
    tree->horizontalScrollBar()->setValue(initialPosition + tree->viewport()->width() * 4);
    style.itemPaintCalls = 0;
    tree->viewport()->repaint();
    QVERIFY(style.itemPaintCalls > 0);
    compareViewport();
    tree->horizontalScrollBar()->setValue(tree->horizontalScrollBar()->value()
                                          + tree->indentation());
    compareViewport();
    const QModelIndex selected = tree->indexAt(tree->viewport()->rect().center());
    QVERIFY(selected.isValid());
    tree->setCurrentIndex(selected);
    compareViewport();
    const int benchmarkPosition = tree->horizontalScrollBar()->value();
    QBENCHMARK {
        tree->horizontalScrollBar()->setValue(
            tree->horizontalScrollBar()->value() == benchmarkPosition ? benchmarkPosition + 1
                                                                      : benchmarkPosition);
        tree->viewport()->repaint();
    }
}

void CallTreeViewTest::testCellPainting()
{
    QFETCH(QString, styleName);
    QFETCH(Qt::LayoutDirection, direction);
    SampleTraceData data;
    data.labels.append("Namespace::function(std::vector<QString> const &, int)");
    SampleTraceData::ThreadSample sample;
    sample.tid = 1;
    sample.running = true;
    sample.frames.append(0);
    data.samples.append(sample);
    CallTreeModel model;
    model.setTraceData(&data);
    CallTreePanelProbeWidget parent;
    parent.setLayoutDirection(direction);
    CallTreeBranchProbeStyle style(styleName, parent);
    CallTreeView view(&model, &parent);
    view.resize(500, 300);
    auto tree = view.findChild<QTreeView *>("SamplerCallTree");
    QVERIFY(tree);
    tree->setStyle(&style);
    view.show();
    parent.show();
    QVERIFY(QTest::qWaitForWindowExposed(&parent));
    QModelIndex index = model.index(0, 0);
    while (model.rowCount(index) > 0)
        index = model.index(0, 0, index);
    QVERIFY(index.isValid());
    QStyledItemDelegate reference;
    QStyleOptionViewItem option;
    option.initFrom(tree);
    option.widget = tree;
    option.direction = direction;
    option.rect = QRect(20, 10, 1000, 30);
    const auto render = [&](QAbstractItemDelegate *delegate) {
        const qreal pixelRatio = tree->devicePixelRatioF();
        QImage image(QSize(1200, 60) * pixelRatio, QImage::Format_ARGB32_Premultiplied);
        image.setDevicePixelRatio(pixelRatio);
        image.fill(Qt::transparent);
        QPainter painter(&image);
        delegate->paint(&painter, option, index);
        return image;
    };
    QCOMPARE(render(tree->itemDelegate()), render(&reference));
    style.itemPaintCalls = 0;
    QCOMPARE(render(tree->itemDelegate()), render(&reference));
    QCOMPARE(style.itemPaintCalls, 1);
    option.rect.moveTopLeft(QPoint(21, 11));
    QCOMPARE(render(tree->itemDelegate()), render(&reference));
    option.state |= QStyle::State_Selected | QStyle::State_HasFocus;
    QCOMPARE(render(tree->itemDelegate()), render(&reference));
    option.state &= ~(QStyle::State_Selected | QStyle::State_HasFocus);
    option.font.setItalic(true);
    option.fontMetrics = QFontMetrics(option.font);
    QCOMPARE(render(tree->itemDelegate()), render(&reference));
    option.palette.setColor(QPalette::Text, option.palette.color(QPalette::Highlight));
    QCOMPARE(render(tree->itemDelegate()), render(&reference));
    option.rect.setWidth(300);
    QCOMPARE(render(tree->itemDelegate()), render(&reference));
    const QImage original = render(tree->itemDelegate());
    data.labels[0] = "Changed::function()";
    model.setTraceData(&data);
    index = model.index(0, 0);
    while (model.rowCount(index) > 0)
        index = model.index(0, 0, index);
    QCOMPARE(render(tree->itemDelegate()), render(&reference));
    QVERIFY(render(tree->itemDelegate()) != original);

    auto heaviest = view.findChild<QTreeWidget *>("SamplerHeaviestStack");
    QVERIFY(heaviest);
    heaviest->setStyle(&style);
    index = heaviest->model()->index(0, 1);
    QVERIFY(index.isValid());
    option.widget = heaviest;
    QCOMPARE(render(heaviest->itemDelegate()), render(&reference));
    const QImage originalHeaviest = render(heaviest->itemDelegate());
    heaviest->topLevelItem(0)->setText(1, "Changed::heaviest()");
    QCOMPARE(render(heaviest->itemDelegate()), render(&reference));
    QVERIFY(render(heaviest->itemDelegate()) != originalHeaviest);
}

void CallTreeViewTest::testHorizontalScrolling_data()
{
    QTest::addColumn<bool>("heaviest");
    QTest::addColumn<Qt::LayoutDirection>("direction");
    QTest::newRow("deep-call-tree-ltr") << false << Qt::LeftToRight;
    QTest::newRow("deep-call-tree-rtl") << false << Qt::RightToLeft;
    QTest::newRow("long-heaviest-stack-symbol-ltr") << true << Qt::LeftToRight;
    QTest::newRow("long-heaviest-stack-symbol-rtl") << true << Qt::RightToLeft;
}

void CallTreeViewTest::testNavigationKeepsHorizontalPosition_data()
{
    QTest::addColumn<Qt::LayoutDirection>("direction");
    QTest::newRow("left-to-right") << Qt::LeftToRight;
    QTest::newRow("right-to-left") << Qt::RightToLeft;
}

void CallTreeViewTest::testNavigationKeepsHorizontalPosition()
{
    QFETCH(Qt::LayoutDirection, direction);
    SampleTraceData data;
    SampleTraceData::ThreadSample sample;
    sample.tid = 1;
    sample.running = true;
    for (int frame = 0; frame < 64; ++frame) {
        data.labels.append(QString::number(frame));
        sample.frames.append(frame);
    }
    data.samples.append(sample);
    CallTreeModel model;
    model.setTraceData(&data);
    CallTreeView view(&model);
    view.setLayoutDirection(direction);
    view.resize(500, 300);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    auto tree = view.findChild<QTreeView *>("SamplerCallTree");
    auto fixed = view.findChild<QTreeView *>("SamplerCallTreeFixedColumns");
    auto heaviest = view.findChild<QTreeWidget *>("SamplerHeaviestStack");
    QVERIFY(tree);
    QVERIFY(fixed);
    QVERIFY(heaviest);
    tree->expandAll();
    QTRY_VERIFY(tree->horizontalScrollBar()->maximum() > 0);
    QModelIndex index = model.index(0, 0);
    while (model.rowCount(index) > 0)
        index = model.index(0, 0, index);
    QVERIFY(index.parent().isValid());
    tree->setCurrentIndex(index.parent());
    tree->setFocus();
    const int position = tree->horizontalScrollBar()->maximum() - 10;
    QVERIFY(position > 0);
    tree->horizontalScrollBar()->setValue(position);
    QTest::keyClick(tree, Qt::Key_Down);
    QCOMPARE(tree->currentIndex(), index);
    QCOMPARE(tree->horizontalScrollBar()->value(), position);
    const QModelIndex weight = index.siblingAtColumn(CallTreeModel::WeightColumn);
    const QPoint center = fixed->visualRect(weight).center();
    QVERIFY(fixed->viewport()->rect().contains(center));
    QTest::mouseClick(fixed->viewport(), Qt::LeftButton, Qt::NoModifier, center);
    QCOMPARE(tree->currentIndex(), weight);
    QCOMPARE(tree->horizontalScrollBar()->value(), position);
    QTest::keyClick(tree, Qt::Key_Up);
    QCOMPARE(tree->currentIndex().siblingAtColumn(0), index.parent());
    QCOMPARE(tree->horizontalScrollBar()->value(), position);
    QVERIFY(heaviest->topLevelItemCount() > 1);
    heaviest->setCurrentItem(heaviest->topLevelItem(heaviest->topLevelItemCount() - 1));
    QCOMPARE(tree->currentIndex(), index);
    QCOMPARE(tree->horizontalScrollBar()->value(), position);
    QVERIFY(tree->viewport()->rect().intersects(tree->visualRect(index)));
}

void CallTreeViewTest::testHorizontalScrolling()
{
    QFETCH(bool, heaviest);
    QFETCH(Qt::LayoutDirection, direction);
    SampleTraceData data;
    SampleTraceData::ThreadSample sample;
    sample.tid = 1;
    sample.running = true;
    const int depth = heaviest ? 1 : 64;
    for (int frame = 0; frame < depth; ++frame) {
        data.labels.append(heaviest ? QString(120, QLatin1Char('W'))
                                    : QString::number(frame));
        sample.frames.append(frame);
    }
    data.samples.append(sample);
    CallTreeModel model;
    model.setTraceData(&data);
    CallTreeView view(&model);
    view.setLayoutDirection(direction);
    view.resize(500, 300);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    auto tree = view.findChild<QTreeView *>(heaviest ? "SamplerHeaviestStack" : "SamplerCallTree");
    QVERIFY(tree);
    tree->expandAll();
    QTRY_VERIFY(tree->horizontalScrollBar()->maximum() > 0);

    QModelIndex index = tree->model()->index(0, heaviest ? 1 : 0);
    if (!heaviest) {
        while (tree->model()->rowCount(index) > 0)
            index = tree->model()->index(0, 0, index);
    }
    QVERIFY(index.isValid());
    QTreeView *fixedColumns = nullptr;
    QRect initialWeightRect;
    QRect initialSelfRect;
    if (!heaviest) {
        fixedColumns = view.findChild<QTreeView *>("SamplerCallTreeFixedColumns");
        QVERIFY(fixedColumns);
        QVERIFY(fixedColumns->viewport()->isVisibleTo(&view));
        QVERIFY(tree->isColumnHidden(CallTreeModel::WeightColumn));
        QVERIFY(tree->isColumnHidden(CallTreeModel::SelfColumn));
        QCOMPARE(fixedColumns->selectionModel(), tree->selectionModel());
        initialWeightRect = fixedColumns->visualRect(
            index.siblingAtColumn(CallTreeModel::WeightColumn));
        initialSelfRect = fixedColumns->visualRect(index.siblingAtColumn(CallTreeModel::SelfColumn));
        tree->horizontalScrollBar()->setValue(0);
        const QRect rootRect = tree->visualRect(model.index(0, 0));
        const QPoint rootPoint(direction == Qt::LeftToRight ? rootRect.left() + 2
                                                            : rootRect.right() - 2,
                               rootRect.center().y());
        QVERIFY(tree->viewport()->rect().contains(rootPoint));
        QCOMPARE(view.childAt(tree->viewport()->mapTo(&view, rootPoint)), tree->viewport());
        const QPoint headerPoint(direction == Qt::LeftToRight ? 2 : tree->header()->width() - 3,
                                 tree->header()->height() / 2);
        QCOMPARE(view.childAt(tree->header()->mapTo(&view, headerPoint)),
                 tree->header()->viewport());
    }
    tree->horizontalScrollBar()->setValue(0);
    const int initialLeft = tree->visualRect(index).left();
    tree->horizontalScrollBar()->setValue(tree->horizontalScrollBar()->maximum());
    if (direction == Qt::LeftToRight)
        QVERIFY(tree->visualRect(index).left() < initialLeft);
    else
        QVERIFY(tree->visualRect(index).left() > initialLeft);
    if (fixedColumns) {
        const QModelIndex weightIndex = index.siblingAtColumn(CallTreeModel::WeightColumn);
        QCOMPARE(fixedColumns->visualRect(weightIndex), initialWeightRect);
        QCOMPARE(fixedColumns->visualRect(index.siblingAtColumn(CallTreeModel::SelfColumn)),
                 initialSelfRect);
        tree->verticalScrollBar()->setValue(tree->verticalScrollBar()->maximum());
        QTRY_COMPARE(fixedColumns->visualRect(weightIndex).top(), tree->visualRect(index).top());
        const QPoint weightCenter = fixedColumns->viewport()->mapTo(
            &view, fixedColumns->visualRect(weightIndex).center());
        QCOMPARE(view.childAt(weightCenter), fixedColumns->viewport());
        tree->collapseAll();
        QTRY_VERIFY(!fixedColumns->isExpanded(model.index(0, 0)));
        view.resize(650, 350);
        if (direction == Qt::LeftToRight)
            QTRY_COMPARE(fixedColumns->geometry().right(), tree->viewport()->geometry().right());
        else
            QTRY_COMPARE(fixedColumns->geometry().left(), tree->viewport()->geometry().left());
        QTRY_COMPARE(fixedColumns->geometry().bottom(), tree->viewport()->geometry().bottom());
        tree->expandAll();
        QTRY_VERIFY(fixedColumns->isExpanded(model.index(0, 0)));
        QTRY_COMPARE(fixedColumns->geometry().bottom(), tree->viewport()->geometry().bottom());
    }
}

} // namespace Profiler::Internal
