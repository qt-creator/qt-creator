// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QStandardItemModel>
#include <QtQml/qqml.h>

class TestFlameGraphModel : public QStandardItemModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("use the context property")

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
    Q_ENUM(Role)

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
