// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QTableView>

namespace ScxmlEditor {

namespace OutputPane {

class TableView : public QTableView
{
    Q_OBJECT

public:
    TableView(QWidget *parent = nullptr);

signals:
    void mouseExited();

protected:
    void leaveEvent(QEvent *e) override;
};

} // namespace OutputPane
} // namespace ScxmlEditor
