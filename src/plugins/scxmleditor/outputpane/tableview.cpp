// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "tableview.h"

using namespace ScxmlEditor::OutputPane;

TableView::TableView(QWidget *parent)
    : QTableView(parent)
{
    setMouseTracking(true);
}

void TableView::leaveEvent(QEvent *e)
{
    emit mouseExited();
    QTableView::leaveEvent(e);
}
