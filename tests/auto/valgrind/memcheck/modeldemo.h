// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QApplication>
#include <QObject>
#include <QDebug>
#include <QItemSelection>
#include <QModelIndex>

#include <valgrind/xmlprotocol/error.h>
#include <valgrind/xmlprotocol/errorlistmodel.h>
#include <valgrind/xmlprotocol/stackmodel.h>
#include <valgrind/valgrindprocess.h>

class ModelDemo : public QObject
{
    Q_OBJECT
public:
    explicit ModelDemo(Valgrind::ValgrindProcess *r, QObject *parent = 0)
        : QObject(parent)
        , runner(r)
    {
    }

    Valgrind::XmlProtocol::StackModel* stackModel;

public Q_SLOTS:
    void selectionChanged(const QItemSelection &sel, const QItemSelection &) {
        if (sel.indexes().isEmpty())
            return;
        const QModelIndex idx = sel.indexes().first();
        const Valgrind::XmlProtocol::Error err = idx.data(Valgrind::XmlProtocol::ErrorListModel::ErrorRole).value<Valgrind::XmlProtocol::Error>();
        qDebug() << idx.row() << err.what();
        stackModel->setError(err);
    }

private:
    Valgrind::ValgrindProcess *runner;
};
