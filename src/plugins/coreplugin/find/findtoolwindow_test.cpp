// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "findtoolwindow_test.h"

#include "findplugin.h"
#include "findtoolwindow.h"
#include "ifindfilter.h"

#include <utils/fancytextedit.h>

#include <QAbstractItemView>
#include <QCompleter>
#include <QPlainTextEdit>
#include <QTest>

namespace Core::Internal {

class RecordingFilter final : public IFindFilter
{
public:
    QString id() const override { return "FindToolWindowTestFilter"; }
    QString displayName() const override { return "Find tool window test"; }
    bool isEnabled() const override { return true; }
    void findAll(const QString &txt, Utils::FindFlags) override { searches << txt; }

    QStringList searches;
};

class FindToolWindowTest final : public QObject
{
    Q_OBJECT

private slots:
    void testReturnStartsOneSearch();
    void testReturnWhileTheHistoryIsShownStartsOneSearch();
    void testReturnPickingACompletionDoesNotSearch();
};

static void waitForQueuedEvents(QObject *context)
{
    bool reached = false;
    QMetaObject::invokeMethod(context, [&reached] { reached = true; }, Qt::QueuedConnection);
    QTRY_VERIFY(reached);
}

static QPlainTextEdit *inputOf(Utils::FancyTextEdit *edit)
{
    return edit->findChild<QPlainTextEdit *>();
}

void FindToolWindowTest::testReturnStartsOneSearch()
{
    RecordingFilter filter;
    FindToolWindow window;
    window.setFindFilters({&filter});
    auto term = window.findChild<Utils::FancyTextEdit *>();
    QVERIFY(term);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    inputOf(term)->setFocus();
    QTest::keyClicks(inputOf(term), "find-tool-window-test-no-history");

    QTest::keyClick(inputOf(term), Qt::Key_Return);

    waitForQueuedEvents(&window);
    QCOMPARE(filter.searches, QStringList{"find-tool-window-test-no-history"});
}

void FindToolWindowTest::testReturnWhileTheHistoryIsShownStartsOneSearch()
{
    // The search adds its term to the history of the search term, as every search does
    Find::updateFindCompletion("find-tool-window-test-history", Find::findFlags());
    RecordingFilter filter;
    FindToolWindow window;
    window.setFindFilters({&filter});
    auto term = window.findChild<Utils::FancyTextEdit *>();
    auto completer = window.findChild<QCompleter *>();
    QVERIFY(term);
    QVERIFY(completer);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    inputOf(term)->setFocus();
    QTest::keyClicks(inputOf(term), "find-tool-window-test-hi");
    QTRY_VERIFY(completer->popup()->isVisible());

    QTest::keyClick(inputOf(term), Qt::Key_Return); // No entry is picked

    waitForQueuedEvents(&window);
    QCOMPARE(filter.searches, QStringList{"find-tool-window-test-hi"});
}

void FindToolWindowTest::testReturnPickingACompletionDoesNotSearch()
{
    // The search adds its term to the history of the search term, as every search does
    Find::updateFindCompletion("find-tool-window-test-history", Find::findFlags());
    RecordingFilter filter;
    FindToolWindow window;
    window.setFindFilters({&filter});
    auto term = window.findChild<Utils::FancyTextEdit *>();
    auto completer = window.findChild<QCompleter *>();
    QVERIFY(term);
    QVERIFY(completer);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    inputOf(term)->setFocus();
    QTest::keyClicks(inputOf(term), "find-tool-window-test-hi");
    QTRY_VERIFY(completer->popup()->isVisible());
    QTest::keyClick(completer->popup(), Qt::Key_Down);

    QTest::keyClick(completer->popup(), Qt::Key_Return);

    waitForQueuedEvents(&window);
    QCOMPARE(term->text(), "find-tool-window-test-history");
    QCOMPARE(filter.searches.size(), 0); // Picking a completion does not start the search
}

QObject *createFindToolWindowTest()
{
    return new FindToolWindowTest;
}

} // namespace Core::Internal

#include "findtoolwindow_test.moc"
