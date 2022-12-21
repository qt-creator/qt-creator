// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include "signalhandlerproperty.h"
#include <QDialog>

QT_FORWARD_DECLARE_CLASS(QTableView)

namespace QmlDesigner {

class EventList;
class FilterLineWidget;

class ConnectSignalDialog : public QDialog
{
    Q_OBJECT

public:
    ConnectSignalDialog(QWidget *parent = nullptr);
    void initialize(EventList &events, const SignalHandlerProperty &signal);

protected:
    QSize sizeHint() const override;

private:
    QTableView *m_table;
    FilterLineWidget *m_filter;
    SignalHandlerProperty m_property;
};

} // namespace QmlDesigner.
