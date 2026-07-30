// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "myinterfaces.h"
#include "ui_main.h"

#include <aggregate.h>

#include <QWidget>

class MyMain : public QWidget
{
    Q_OBJECT

public:
    explicit MyMain(QWidget *parent = 0, Qt::WFlags flags = 0);
    ~MyMain();

    void add(IComboEntry *obj);

private:
    void select(int index);

    Ui::mainClass ui;

    QList<IComboEntry *> m_entries;
};
