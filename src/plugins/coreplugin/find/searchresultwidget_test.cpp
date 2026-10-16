// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "searchresultwidget_test.h"

#include "searchresultwidget.h"
#include "searchresultwindow.h"

#include <utils/filepath.h>
#include <utils/searchresultitem.h>

#include <QComboBox>
#include <QLabel>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QStackedWidget>
#include <QTest>

using namespace Utils;

namespace Core::Internal {

class SearchResultWidgetTest final : public QObject
{
    Q_OBJECT

private slots:
    void testCtrlReturnInReplaceFieldStartsANewLine();
    void testMultiLineTermIsShownOnOneLine_data();
    void testMultiLineTermIsShownOnOneLine();
};

void SearchResultWidgetTest::testCtrlReturnInReplaceFieldStartsANewLine()
{
    SearchResultWidget widget;
    widget.setSupportsReplace(true, {});
    SearchResultItem item;
    item.setFilePath(FilePath::fromString("/tmp/file.txt"));
    item.setMainRange(Text::Range{{1, 0}, {2, 3}});
    item.setLineText("old\nold");
    widget.addResults({item}, SearchResult::AddOrdered);
    widget.finishSearch(false, {});
    widget.setTextToReplace("new");
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    // Showing the replace field focuses it
    QWidget *replaceField = widget.focusWidget();
    QVERIFY(replaceField);
    QSignalSpy replaceSpy(&widget, &SearchResultWidget::replaceButtonClicked);

    QTest::keyClick(replaceField, Qt::Key_End);
    QTest::keyClick(replaceField, Qt::Key_Return, Qt::ControlModifier);
    QTest::keyClicks(replaceField, "new");

    QCOMPARE(replaceSpy.count(), 0);
    QCOMPARE(widget.textToReplace(), "new\nnew");

    QTest::keyClick(replaceField, Qt::Key_Return);

    QCOMPARE(replaceSpy.count(), 1);
    QCOMPARE(replaceSpy.at(0).at(0).toString(), "new\nnew");
}

void SearchResultWidgetTest::testMultiLineTermIsShownOnOneLine_data()
{
    QTest::addColumn<QString>("term");

    QTest::newRow("LF") << QString("a\nb");
    QTest::newRow("CRLF") << QString("a\r\nb");
    QTest::newRow("CR") << QString("a\rb");
}

void SearchResultWidgetTest::testMultiLineTermIsShownOnOneLine()
{
    QFETCH(QString, term);
    const QString shown = QString("a") + QChar(0x23ce) + "b";
    SearchResultWindow *window = SearchResultWindow::instance();
    const auto cleanup = qScopeGuard([window] { window->clearContents(); });

    window->startNewSearch("Label:", {}, term);

    QComboBox *history = nullptr;
    for (QWidget *widget : window->toolBarWidgets()) {
        if ((history = qobject_cast<QComboBox *>(widget)))
            break;
    }
    QVERIFY(history);
    QCOMPARE(history->itemText(1), "Label: " + shown);

    const auto stack = qobject_cast<QStackedWidget *>(window->outputWidget(nullptr));
    QVERIFY(stack);
    const auto widget = qobject_cast<SearchResultWidget *>(stack->widget(1));
    QVERIFY(widget);
    QStringList labels;
    for (const QLabel *label : widget->findChildren<QLabel *>())
        labels << label->text();
    QVERIFY2(labels.contains(shown), qPrintable(labels.join(" | ")));
}

QObject *createSearchResultWidgetTest()
{
    return new SearchResultWidgetTest;
}

} // namespace Core::Internal

#include "searchresultwidget_test.moc"
