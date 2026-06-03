// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <tracing/flamegraphwidget.h>

#include <utils/theme/theme.h>
#include <utils/theme/theme_p.h>

#include <QAbstractItemModel>
#include <QStandardItemModel>
#include <QTest>

using namespace Timeline;

class DummyTheme : public Utils::Theme
{
public:
    DummyTheme() : Utils::Theme(QLatin1String("dummy")) {}
};

class tst_FlameGraphWidgetView : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testZoom();

private:
    static constexpr int sizeRole = Qt::UserRole + 1;
    static constexpr int typeIdRole = Qt::UserRole + 2;

    QStandardItemModel m_model;
    FlameGraphWidget *m_widget = nullptr;
};

void tst_FlameGraphWidgetView::cleanupTestCase()
{
    delete m_widget;
    m_widget = nullptr;
    delete Utils::creatorTheme();
    Utils::setCreatorTheme(nullptr);
}

void tst_FlameGraphWidgetView::initTestCase()
{
    Utils::setCreatorTheme(new DummyTheme);
    m_model.invisibleRootItem()->setData(15, sizeRole);
    for (int i = 1; i <= 5; ++i) {
        auto item = new QStandardItem;
        item->setData(i, sizeRole);
        item->setData(i, typeIdRole);
        m_model.appendRow(item);
    }

    m_widget = new FlameGraphWidget(&m_model);
    QList<QPair<int, QString>> roles;
    roles << qMakePair(sizeRole, QString("Size"));
    m_widget->setSizeRoles(roles);
    m_widget->setTypeIdRole(typeIdRole);
    m_widget->resize(400, 200);
    m_widget->show();
    QVERIFY(QTest::qWaitForWindowExposed(m_widget));
}

void tst_FlameGraphWidgetView::testZoom()
{
    QVERIFY(m_widget);
    QVERIFY(!m_widget->isZoomed());

    QPoint center(m_widget->width() / 2, m_widget->height() / 2);
    QTest::mouseDClick(m_widget, Qt::LeftButton, Qt::NoModifier, center);
    QApplication::processEvents();

    if (m_widget->isZoomed()) {
        m_widget->resetRoot();
        QApplication::processEvents();
        QVERIFY(!m_widget->isZoomed());
    }
}

QTEST_MAIN(tst_FlameGraphWidgetView)

#include "tst_flamegraphwidgetview.moc"
