// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <utils/basetreeview.h>
#include <utils/qtcsettings.h>

#include <QHeaderView>
#include <QStandardItemModel>
#include <QTemporaryDir>
#include <QTest>

using namespace Utils;

class tst_BaseTreeView : public QObject
{
    Q_OBJECT

private slots:
    void restoreColumnWidthsWhenModelComesFirst();
};

void tst_BaseTreeView::restoreColumnWidthsWhenModelComesFirst()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QtcSettings settings(dir.filePath("settings.ini"), QSettings::IniFormat);
    settings.beginGroup("View");
    settings.setValue("Columns", QVariantList{1, 123});
    settings.endGroup();

    QStandardItemModel model(1, 3);
    BaseTreeView view;
    view.setModel(&model);
    view.setSettings(&settings, "View");

    QCOMPARE(view.header()->sectionSize(1), 123);
}

QTEST_MAIN(tst_BaseTreeView)

#include "tst_basetreeview.moc"
