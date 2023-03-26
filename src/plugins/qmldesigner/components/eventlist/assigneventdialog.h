// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include <QDialog>

QT_FORWARD_DECLARE_CLASS(QTableView)

namespace QmlDesigner {

class EventList;
class FilterLineWidget;

class AssignEventDialog : public QDialog
{
    Q_OBJECT

public:
    AssignEventDialog(QWidget *parent = nullptr);
    void initialize(EventList &events);
    void postShow();

private:
    QTableView *m_nodeTable;
    QTableView *m_eventTable;

    FilterLineWidget *m_nodeLine;
    FilterLineWidget *m_eventLine;

    QMetaObject::Connection m_connection;
};

} // namespace QmlDesigner.
