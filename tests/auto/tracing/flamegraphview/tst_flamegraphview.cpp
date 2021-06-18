/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include <testflamegraphmodel.h>

#include <tracing/flamegraph.h>
#include <tracing/timelinetheme.h>
#include <utils/theme/theme_p.h>

#include <QObject>
#include <QQmlContext>
#include <QQuickWidget>
#include <QtTest>

class DummyTheme : public Utils::Theme
{
public:
    DummyTheme() : Utils::Theme(QLatin1String("dummy"))
    {
        for (int i = 0; i < d->colors.count(); ++i) {
            d->colors[i] = QPair<QColor, QString>(
                        QColor::fromRgb(QRandomGenerator::global()->bounded(256),
                                        QRandomGenerator::global()->bounded(256),
                                        QRandomGenerator::global()->bounded(256)),
                        QString::number(i));
        }
    }
};

class tst_FlameGraphView : public QObject
{
    Q_OBJECT
public:
    tst_FlameGraphView() { Utils::setCreatorTheme(&theme); }

private slots:
    void initTestCase();
    void testZoom();

private:
    static const int sizeRole = Qt::UserRole + 1;
    static const int dataRole = Qt::UserRole + 2;
    TestFlameGraphModel model;
    QQuickWidget widget;
    DummyTheme theme;
};

void tst_FlameGraphView::initTestCase()
{
    model.fill();
#if QT_VERSION < QT_VERSION_CHECK(6, 2, 0)
    qmlRegisterType<FlameGraph::FlameGraph>("QtCreator.Tracing", 1, 0, "FlameGraph");
    qmlRegisterUncreatableType<TestFlameGraphModel>(
                "QtCreator.TstTracingFlameGraphView", 1, 0, "TestFlameGraphModel",
                QLatin1String("use the context property"));
#endif // Qt < 6.2

    Timeline::TimelineTheme::setupTheme(widget.engine());

    widget.rootContext()->setContextProperty(QStringLiteral("flameGraphModel"), &model);
    widget.setSource(QUrl(QStringLiteral(
                              "qrc:/QtCreator/TstTracingFlameGraphView/TestFlameGraphView.qml")));

    widget.setResizeMode(QQuickWidget::SizeRootObjectToView);
    widget.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    widget.resize(800, 600);

    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(widget.windowHandle()));
}

void tst_FlameGraphView::testZoom()
{
    auto selectedTypeId = [&]() {
        return widget.rootObject()->property("selectedTypeId").toInt();
    };

    QWindow *window = widget.windowHandle();

    QCOMPARE(selectedTypeId(), -1);
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, QPoint(widget.width() - 15,
                                                                     widget.height() - 15));
    QTRY_VERIFY(selectedTypeId() != -1);
    const int typeId1 = selectedTypeId();

    QTest::mouseDClick(window, Qt::LeftButton, Qt::NoModifier, QPoint(15, widget.height() - 15));
    QTRY_VERIFY(selectedTypeId() != typeId1);
    QVERIFY(selectedTypeId() != -1);

    QTest::mouseDClick(window, Qt::LeftButton, Qt::NoModifier, QPoint(widget.width() / 2,
                                                                      widget.height() / 2));
    QTRY_COMPARE(selectedTypeId(), -1);
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, QPoint(widget.width() - 15,
                                                                     widget.height() - 15));
    QTRY_COMPARE(selectedTypeId(), typeId1);
}

QTEST_MAIN(tst_FlameGraphView)

#include "tst_flamegraphview.moc"
