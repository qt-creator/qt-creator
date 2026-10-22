// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "searchresult_test.h"

#include "searchresulttreeitemroles.h"
#include "searchresulttreemodel.h"
#include "searchresulttreeview.h"
#include "searchresultwidget.h"
#include "searchresultwindow.h"

#include <utils/algorithm.h>
#include <utils/searchresultitem.h>

#include <QSignalSpy>
#include <QTest>

using namespace Utils;

namespace Core::Internal {

static SearchResultItem result(
    const QStringList &path, const QString &text, const std::optional<FilePath> &filePath = {})
{
    SearchResultItem item;
    if (filePath)
        item.setFilePath(*filePath);
    item.setPath(path);
    item.setLineText(text);
    return item;
}

static FilePath fileA()
{
    return FilePath::fromUserInput("/a.cpp");
}

static FilePath fileC()
{
    return FilePath::fromUserInput("/c.cpp");
}

static SearchResultItems flatAndNestedResults()
{
    return {result({"a.cpp"}, "a", fileA()),
            result({"b.cpp"}, "b"),
            result({"Group", "c.cpp"}, "c", fileC()),
            result({"Group", "d.cpp"}, "d")};
}

class LineTextFilter final : public SearchResultFilter
{
public:
    explicit LineTextFilter(const QStringList &rejected)
        : m_rejected(rejected)
    {}

    QWidget *createWidget() final { return nullptr; }
    bool matches(const SearchResultItem &item) const final
    {
        return !m_rejected.contains(item.lineText());
    }

private:
    const QStringList m_rejected;
};

static QModelIndex rowNamed(
    const SearchResultFilterModel *model, const QModelIndex &parent, const QString &text)
{
    for (int row = 0; row < model->rowCount(parent); ++row) {
        const QModelIndex index = model->index(row, 0, parent);
        if (model->data(index, ItemDataRoles::ResultLineRole).toString() == text)
            return index;
    }
    return {};
}

class SearchResultTest final : public QObject
{
    Q_OBJECT

private slots:
    void testItemsAreTheResultsAtAnyDepth()
    {
        SearchResultWidget widget;
        widget.addResults(flatAndNestedResults(), SearchResult::AddOrdered);

        QStringList texts = Utils::transform(widget.items(false), &SearchResultItem::lineText);
        texts.sort();
        QCOMPARE(texts, QStringList({"a", "b", "c", "d"}));
    }

    void testNestedFileCarriesItsFilePath()
    {
        SearchResultTreeView view;
        view.addResults(flatAndNestedResults(), SearchResult::AddOrdered);
        const SearchResultFilterModel *model = view.model();

        const QModelIndex group = rowNamed(model, {}, "Group");
        QVERIFY(group.isValid());
        const auto itemAt = [model](const QModelIndex &index) {
            return model->data(index, ItemDataRoles::ResultItemRole).value<SearchResultItem>();
        };
        QVERIFY(!itemAt(group).filePath());

        const QModelIndex fileWithPath = rowNamed(model, group, "c.cpp");
        QVERIFY(fileWithPath.isValid());
        QCOMPARE(itemAt(fileWithPath).filePath().value_or(FilePath()), fileC());
        const QModelIndex fileWithoutPath = rowNamed(model, group, "d.cpp");
        QVERIFY(fileWithoutPath.isValid());
        QVERIFY(!itemAt(fileWithoutPath).filePath());
        QCOMPARE(itemAt(rowNamed(model, {}, "a.cpp")).filePath().value_or(FilePath()), fileA());
    }

    void testReturnOnGroupTogglesInsteadOfJumping()
    {
        SearchResultTreeView view;
        view.addResults({result({"Group"}, "e")}, SearchResult::AddOrdered);
        view.addResults(flatAndNestedResults(), SearchResult::AddOrdered);
        QSignalSpy jumps(&view, &SearchResultTreeView::jumpToSearchResult);
        const QModelIndex group = rowNamed(view.model(), {}, "Group");
        QVERIFY(group.isValid());

        view.setCurrentIndex(group);
        const bool expanded = view.isExpanded(group);
        QTest::keyClick(&view, Qt::Key_Return);
        QCOMPARE(jumps.count(), 0);
        QCOMPARE(view.isExpanded(group), !expanded);

        const QModelIndex fileWithoutPath = rowNamed(view.model(), group, "d.cpp");
        QVERIFY(fileWithoutPath.isValid());
        view.setCurrentIndex(fileWithoutPath);
        const bool fileExpanded = view.isExpanded(fileWithoutPath);
        QTest::keyClick(&view, Qt::Key_Return);
        QCOMPARE(jumps.count(), 0);
        QCOMPARE(view.isExpanded(fileWithoutPath), !fileExpanded);

        view.setCurrentIndex(rowNamed(view.model(), group, "c.cpp"));
        QTest::keyClick(&view, Qt::Key_Return);
        QCOMPARE(jumps.count(), 1);
        const SearchResultItem jumpedTo = jumps.at(0).at(0).value<SearchResultItem>();
        QCOMPARE(jumpedTo.filePath().value_or(FilePath()), fileC());

        view.setCurrentIndex(rowNamed(view.model(), {}, "a.cpp"));
        QTest::keyClick(&view, Qt::Key_Return);
        QCOMPARE(jumps.count(), 2);
        QCOMPARE(jumps.at(1).at(0).value<SearchResultItem>().lineText(), QString("a.cpp"));
    }

    void testReturnOnFilteredGroupTogglesInsteadOfJumping()
    {
        SearchResultItems results = flatAndNestedResults();
        results.prepend(result({"Group"}, "e"));
        for (SearchResultItem &item : results)
            item.setUserData(true);
        SearchResultTreeView view;
        view.addResults(results, SearchResult::AddOrdered);
        view.setFilter(new LineTextFilter({"c", "d"}));
        QSignalSpy jumps(&view, &SearchResultTreeView::jumpToSearchResult);
        const QModelIndex group = rowNamed(view.model(), {}, "Group");
        QVERIFY(group.isValid());
        QVERIFY(!rowNamed(view.model(), group, "c.cpp").isValid());
        QVERIFY(!rowNamed(view.model(), group, "d.cpp").isValid());

        view.setCurrentIndex(group);
        const bool expanded = view.isExpanded(group);
        QTest::keyClick(&view, Qt::Key_Return);
        QCOMPARE(jumps.count(), 0);
        QCOMPARE(view.isExpanded(group), !expanded);
    }

    void testHiddenReplaceTextIsNotReported()
    {
        SearchResultWidget widget;
        widget.addResults(flatAndNestedResults(), SearchResult::AddOrdered);
        widget.setTextToReplace("term");
        widget.setReplaceTextVisible(false);
        widget.setSupportsReplace(true, {});
        QCOMPARE(widget.textToReplace(), QString());

        QSignalSpy replaced(&widget, &SearchResultWidget::replaceButtonClicked);
        widget.triggerReplace();
        QCOMPARE(replaced.count(), 1);
        QCOMPARE(replaced.at(0).at(0).toString(), QString());
    }
};

QObject *createSearchResultTest()
{
    return new SearchResultTest;
}

} // namespace Core::Internal

#include "searchresult_test.moc"
