/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "flamegraphview_test.h"
#include "flamegraphmodel_test.h"

#include <qmlprofiler/qmlprofilertool.h>
#include <QtTest>
#include <QMenu>
#include <QWindow>

namespace QmlProfiler {
namespace Internal {

FlameGraphViewTest::FlameGraphViewTest(QObject *parent) : QObject(parent), view(&manager)
{
}

void FlameGraphViewTest::initTestCase()
{
    FlameGraphModelTest::generateData(&manager);
    QCOMPARE(manager.state(), QmlProfilerModelManager::Done);
    view.resize(500, 500);
    view.show();
    QTest::qWaitForWindowExposed(&view);
}

void FlameGraphViewTest::testSelection()
{
    auto con1 = connect(&view, &QmlProfilerEventsView::gotoSourceLocation,
            [](const QString &file, int line, int column) {
        QCOMPARE(line, 0);
        QCOMPARE(column, 20);
        QCOMPARE(file, QLatin1String("somefile.js"));
    });

    auto con2 = connect(&view, &QmlProfilerEventsView::typeSelected, [](int selected) {
        QCOMPARE(selected, 0);
    });

    QSignalSpy spy(&view, SIGNAL(typeSelected(int)));
    QTest::mouseClick(view.childAt(250, 250), Qt::LeftButton, Qt::NoModifier, QPoint(5, 495));
    if (spy.isEmpty())
        QVERIFY(spy.wait());

    // External setting of type should not send gotoSourceLocation or typeSelected
    view.selectByTypeId(1);
    QCOMPARE(spy.count(), 1);

    // Click in empty area shouldn't change anything, either
    QTest::mouseClick(view.childAt(250, 250), Qt::LeftButton, Qt::NoModifier, QPoint(495, 50));
    QCOMPARE(spy.count(), 1);

    view.onVisibleFeaturesChanged(1 << ProfileBinding);
    QCOMPARE(spy.count(), 1); // External event: still doesn't change anything

    disconnect(con1);
    disconnect(con2);

    // The mouse click will select a different event now, as the JS category has been hidden
    con1 = connect(&view, &QmlProfilerEventsView::gotoSourceLocation,
            [](const QString &file, int line, int column) {
        QCOMPARE(file, QLatin1String("somefile.js"));
        QCOMPARE(line, 2);
        QCOMPARE(column, 18);
    });

    con2 = connect(&view, &QmlProfilerEventsView::typeSelected, [](int selected) {
        QCOMPARE(selected, 2);
    });

    QTest::mouseClick(view.childAt(250, 250), Qt::LeftButton, Qt::NoModifier, QPoint(5, 495));
    if (spy.count() == 1)
        QVERIFY(spy.wait());

    disconnect(con1);
    disconnect(con2);
}

void FlameGraphViewTest::testContextMenu()
{
    int targetWidth = 0;
    int targetHeight = 0;
    {
        QMenu testMenu;
        testMenu.addActions(QmlProfilerTool::profilerContextMenuActions());
        testMenu.addSeparator();
        testMenu.show();
        QTest::qWaitForWindowExposed(testMenu.window());
        targetWidth = testMenu.width() / 2;
        int prevHeight = testMenu.height();
        QAction dummy(QString("target"), this);
        testMenu.addAction(&dummy);
        targetHeight = (testMenu.height() + prevHeight) / 2;
    }

    QTest::mouseMove(&view, QPoint(250, 250));
    QSignalSpy spy(&view, SIGNAL(showFullRange()));

    QTimer timer;
    timer.setInterval(50);
    int menuClicks = 0;

    connect(&timer, &QTimer::timeout, this, [&]() {
        auto activePopup = qApp->activePopupWidget();
        if (!activePopup || !activePopup->windowHandle()->isExposed()) {
            QContextMenuEvent *event = new QContextMenuEvent(QContextMenuEvent::Mouse,
                                                             QPoint(250, 250));
            qApp->postEvent(&view, event);
            return;
        }

        QTest::mouseMove(activePopup, QPoint(targetWidth, targetHeight));
        QTest::mouseClick(activePopup, Qt::LeftButton, Qt::NoModifier,
                          QPoint(targetWidth, targetHeight));
        ++menuClicks;

        if (!manager.isRestrictedToRange()) {
            // click somewhere else to remove the menu and return to outer function
            QTest::mouseMove(activePopup, QPoint(-10, -10));
            QTest::mouseClick(activePopup, Qt::LeftButton, Qt::NoModifier, QPoint(-10, -10));
        }
    });

    timer.start();
    QTRY_VERIFY(menuClicks > 0);
    QCOMPARE(spy.count(), 0);
    manager.restrictToRange(1, 10);
    QVERIFY(manager.isRestrictedToRange());
    QTRY_COMPARE(spy.count(), 1);
    QVERIFY(menuClicks > 1);
    timer.stop();
}

void FlameGraphViewTest::cleanupTestCase()
{
    manager.clear();
}

} // namespace Internal
} // namespace QmlProfiler
