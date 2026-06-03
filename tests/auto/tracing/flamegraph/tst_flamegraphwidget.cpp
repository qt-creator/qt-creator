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

class tst_FlameGraphWidget : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void nullModel();
    void sizeRoles();
    void selectByTypeId();
    void resetRoot();

private:
    static constexpr int sizeRole = Qt::UserRole + 1;
};

void tst_FlameGraphWidget::initTestCase()
{
    Utils::setCreatorTheme(new DummyTheme);
}

void tst_FlameGraphWidget::cleanupTestCase()
{
    delete Utils::creatorTheme();
    Utils::setCreatorTheme(nullptr);
}

void tst_FlameGraphWidget::init()
{
    QStandardItemModel model;
    FlameGraphWidget widget(&model);
    QCOMPARE(widget.model(), &model);
    QVERIFY(!widget.isZoomed());
}

void tst_FlameGraphWidget::nullModel()
{
    QStandardItemModel model;
    FlameGraphWidget widget(&model);
    QVERIFY(!widget.isZoomed());
}

void tst_FlameGraphWidget::sizeRoles()
{
    QStandardItemModel model;
    FlameGraphWidget widget(&model);
    QList<QPair<int, QString>> roles;
    roles << qMakePair(sizeRole, QString("Size"));
    widget.setSizeRoles(roles);
    widget.setTypeIdRole(Qt::UserRole + 2);
}

void tst_FlameGraphWidget::selectByTypeId()
{
    QStandardItemModel model;
    FlameGraphWidget widget(&model);
    widget.selectByTypeId(-1);
    widget.selectByTypeId(0);
    widget.selectByTypeId(42);
}

void tst_FlameGraphWidget::resetRoot()
{
    QStandardItemModel model;
    FlameGraphWidget widget(&model);
    QVERIFY(!widget.isZoomed());
    widget.resetRoot();
    QVERIFY(!widget.isZoomed());
}

QTEST_MAIN(tst_FlameGraphWidget)

#include "tst_flamegraphwidget.moc"
