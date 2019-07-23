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

#include <tracing/flamegraph.h>
#include <tracing/timelinetheme.h>
#include <utils/theme/theme_p.h>

#include <QObject>
#include <QStandardItemModel>
#include <QQmlContext>
#include <QQuickWidget>
#include <QtTest>

class TestFlameGraphModel : public QStandardItemModel
{
    Q_OBJECT
    Q_ENUMS(Role)
public:
    enum Role {
        TypeIdRole = Qt::UserRole + 1,
        SizeRole,
        SourceFileRole,
        SourceLineRole,
        SourceColumnRole,
        DetailsTitleRole,
        SummaryRole,
        MaxRole
    };

    void fill() {
        qreal sizeSum = 0;
        for (int i = 1; i < 10; ++i) {
            QStandardItem *item = new QStandardItem;
            item->setData(i, SizeRole);
            item->setData(100 / i, TypeIdRole);
            item->setData("trara", SourceFileRole);
            item->setData(20, SourceLineRole);
            item->setData(10, SourceColumnRole);
            item->setData("details", DetailsTitleRole);
            item->setData("summary", SummaryRole);

            for (int j = 1; j < i; ++j) {
                QStandardItem *item2 = new QStandardItem;
                item2->setData(1, SizeRole);
                item2->setData(100 / j, TypeIdRole);
                item2->setData(1, SourceLineRole);
                item2->setData("child", DetailsTitleRole);
                item2->setData("childsummary", SummaryRole);
                for (int k = 1; k < 10; ++k) {
                    QStandardItem *skipped = new QStandardItem;
                    skipped->setData(0.001, SizeRole);
                    skipped->setData(100 / k, TypeIdRole);
                    item2->appendRow(skipped);
                }
                item->appendRow(item2);
            }

            appendRow(item);
            sizeSum += i;
        }
        invisibleRootItem()->setData(sizeSum, SizeRole);
        invisibleRootItem()->setData(9 * 20, SourceLineRole);
        invisibleRootItem()->setData(9 * 10, SourceColumnRole);
    }

    Q_INVOKABLE void gotoSourceLocation(const QString &file, int line, int column)
    {
        Q_UNUSED(file)
        Q_UNUSED(line)
        Q_UNUSED(column)
    }
};

class DummyTheme : public Utils::Theme
{
public:
    DummyTheme() : Utils::Theme(QLatin1String("dummy"))
    {
        for (int i = 0; i < d->colors.count(); ++i) {
            d->colors[i] = QPair<QColor, QString>(
                        QColor::fromRgb(qrand() % 256, qrand() % 256, qrand() % 256),
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
    qmlRegisterType<FlameGraph::FlameGraph>("FlameGraph", 1, 0, "FlameGraph");
    qmlRegisterUncreatableType<TestFlameGraphModel>(
                "TestFlameGraphModel", 1, 0, "TestFlameGraphModel",
                QLatin1String("use the context property"));


    Timeline::TimelineTheme::setupTheme(widget.engine());

    widget.rootContext()->setContextProperty(QStringLiteral("flameGraphModel"), &model);
    widget.setSource(QUrl(QStringLiteral("qrc:/tracingtest/TestFlameGraphView.qml")));

    widget.setResizeMode(QQuickWidget::SizeRootObjectToView);
    widget.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    widget.resize(800, 600);

    widget.show();
}

void tst_FlameGraphView::testZoom()
{
    auto selectedTypeId = [&]() {
        return widget.rootObject()->property("selectedTypeId").toInt();
    };

    QWindow *window = widget.windowHandle();

    QCOMPARE(selectedTypeId(), -1);
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, QPoint(widget.width() - 5,
                                                                     widget.height() - 5));
    QTRY_VERIFY(selectedTypeId() != -1);
    const int typeId1 = selectedTypeId();

    QTest::mouseDClick(window, Qt::LeftButton, Qt::NoModifier, QPoint(5, widget.height() - 5));
    QTRY_VERIFY(selectedTypeId() != typeId1);
    QVERIFY(selectedTypeId() != -1);

    QTest::mouseDClick(window, Qt::LeftButton, Qt::NoModifier, QPoint(widget.width() / 2,
                                                                      widget.height() / 2));
    QTRY_COMPARE(selectedTypeId(), -1);
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, QPoint(widget.width() - 5,
                                                                     widget.height() - 5));
    QTRY_COMPARE(selectedTypeId(), typeId1);
}

QTEST_MAIN(tst_FlameGraphView)

#include "tst_flamegraphview.moc"
