// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "searchresulttreeview_test.h"

#include "searchresulttreeitemroles.h"
#include "searchresulttreemodel.h"
#include "searchresulttreeview.h"

#include <utils/filepath.h>
#include <utils/searchresultitem.h>
#include <utils/theme/theme.h>

#include <QFontDatabase>
#include <QImage>
#include <QTest>

using namespace Utils;

namespace Core::Internal {

class SearchResultTreeViewTest final : public QObject
{
    Q_OBJECT

private slots:
    void testRowHeightFollowsLineCount();
    void testUniformRowHeightsOnlyWithSingleLineResults();
    void testHighlightSpansRows();
    void testLongResultIsCutOff();
};

static SearchResultItem makeItem(int line, int column, int endLine, int endColumn,
                                 const QString &text)
{
    SearchResultItem item;
    item.setFilePath(FilePath::fromString("/tmp/file.txt"));
    item.setMainRange(Text::Range{{line, column}, {endLine, endColumn}});
    item.setLineText(text);
    item.setUseTextEditorFont(true);
    return item;
}

static QString lines(int count)
{
    QStringList result;
    for (int i = 0; i < count; ++i)
        result << QString("line %1").arg(i);
    return result.join('\n');
}

static QModelIndex firstResult(SearchResultTreeView &view)
{
    return view.model()->index(0, 0, view.model()->index(0, 0));
}

void SearchResultTreeViewTest::testRowHeightFollowsLineCount()
{
    SearchResultTreeView view;
    view.setAutoExpandResults(true);
    view.addResults({makeItem(1, 0, 1, 4, "line"),
                     makeItem(5, 0, 7, 3, lines(3)),
                     makeItem(10, 0, 17, 3, lines(8))},
                    SearchResult::AddOrdered);

    const QModelIndex parent = view.model()->index(0, 0);
    const int height = view.model()->data(view.model()->index(0, 0, parent), Qt::SizeHintRole)
                           .value<QSize>().height();
    QVERIFY(height > 0);
    QCOMPARE(view.model()->data(view.model()->index(1, 0, parent), Qt::SizeHintRole)
                 .value<QSize>().height(), 3 * height);
    QCOMPARE(view.model()->data(view.model()->index(2, 0, parent), Qt::SizeHintRole)
                 .value<QSize>().height(), maxDisplayedResultLines * height);
}

void SearchResultTreeViewTest::testUniformRowHeightsOnlyWithSingleLineResults()
{
    SearchResultTreeView single;
    single.addResults({makeItem(1, 0, 1, 4, "line")}, SearchResult::AddOrdered);
    QVERIFY(single.uniformRowHeights());

    // A range over several lines with only the first line as text is shown in one row.
    SearchResultTreeView singleLineText;
    singleLineText.addResults({makeItem(1, 0, 2, 4, "line")}, SearchResult::AddOrdered);
    QVERIFY(singleLineText.uniformRowHeights());

    SearchResultTreeView multi;
    multi.addResults({makeItem(1, 0, 2, 4, lines(2))}, SearchResult::AddOrdered);
    QVERIFY(!multi.uniformRowHeights());

    // Searching again starts from a cleared view.
    multi.clear();
    QVERIFY(multi.uniformRowHeights());
    multi.addResults({makeItem(1, 0, 1, 4, "line")}, SearchResult::AddOrdered);
    QVERIFY(multi.uniformRowHeights());
}

struct RedExtent
{
    int left = -1;
    int right = -1;
};

static RedExtent redExtent(const QImage &image, int y, const QColor &color)
{
    RedExtent extent;
    for (int x = 0; x < image.width(); ++x) {
        if (image.pixelColor(x, y) != color)
            continue;
        if (extent.left < 0)
            extent.left = x;
        extent.right = x;
    }
    return extent;
}

static void setupColors(SearchResultTreeView &view, const QColor &highlight)
{
    const QColor background = Qt::white;
    const SearchResultColor colors(background, Qt::black, highlight, Qt::black, background,
                                   Qt::black);
    view.setTextEditorFont(QFontDatabase::systemFont(QFontDatabase::FixedFont),
                           {{SearchResultColor::Style::Default, colors}});
}

void SearchResultTreeViewTest::testHighlightSpansRows()
{
    const QColor highlight(255, 0, 0);
    SearchResultTreeView view;
    setupColors(view, highlight);
    view.setAutoExpandResults(true);
    view.addResults({makeItem(10, 3, 12, 3, "aa foo\nbbbbbb\nbar cc")}, SearchResult::AddOrdered);
    view.resize(600, 300);
    view.show();
    view.doItemsLayout();

    const QModelIndex index = firstResult(view);
    const QRect rect = view.visualRect(index);
    const int rowHeight = rect.height() / 3;
    QVERIFY(rowHeight > 0);
    const QImage image = view.viewport()->grab().toImage();
    const qreal dpr = image.devicePixelRatio();
    const auto row = [&](int r) {
        return redExtent(image, qRound((rect.top() + r * rowHeight + rowHeight / 2) * dpr),
                         highlight);
    };
    const RedExtent first = row(0);
    const RedExtent middle = row(1);
    const RedExtent last = row(2);

    // First row from the begin column, middle row whole, last row up to the end column.
    QVERIFY(first.left >= 0);
    QVERIFY(middle.left >= 0);
    QVERIFY(last.left >= 0);
    QVERIFY(first.left > middle.left);
    QVERIFY(last.right < middle.right);
    QVERIFY(qAbs(last.left - middle.left) <= qRound(dpr));

    // A line in the stroke color closes the result.
    const QColor stroke = Utils::creatorColor(Utils::Theme::Token_Stroke_Subtle);
    const int x = qRound((rect.right() - 1) * dpr);
    QCOMPARE(image.pixelColor(x, qRound(rect.bottom() * dpr)), stroke);
    QVERIFY(image.pixelColor(x, qRound((rect.bottom() - rowHeight) * dpr)) != stroke);
}

void SearchResultTreeViewTest::testLongResultIsCutOff()
{
    const QColor highlight(255, 0, 0);
    SearchResultTreeView view;
    setupColors(view, highlight);
    view.setAutoExpandResults(true);
    view.addResults({makeItem(1, 0, 8, 6, lines(8))}, SearchResult::AddOrdered);
    view.resize(600, 400);
    view.show();
    view.doItemsLayout();

    const QRect rect = view.visualRect(firstResult(view));
    QCOMPARE(rect.height() / maxDisplayedResultLines * maxDisplayedResultLines, rect.height());
    const int rowHeight = rect.height() / maxDisplayedResultLines;
    const QImage image = view.viewport()->grab().toImage();
    const qreal dpr = image.devicePixelRatio();
    const auto row = [&](int r) {
        return redExtent(image, qRound((rect.top() + r * rowHeight + rowHeight / 2) * dpr),
                         highlight);
    };
    // The rows with content are highlighted, the final "..." row is not.
    for (int r = 0; r < maxDisplayedResultLines - 1; ++r)
        QVERIFY2(row(r).left >= 0, qPrintable(QString("row %1").arg(r)));
    QCOMPARE(row(maxDisplayedResultLines - 1).left, -1);
}

QObject *createSearchResultTreeViewTest()
{
    return new SearchResultTreeViewTest;
}

} // namespace Core::Internal

#include "searchresulttreeview_test.moc"
