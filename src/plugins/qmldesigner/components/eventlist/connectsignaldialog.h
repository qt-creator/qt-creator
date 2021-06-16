/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at http://www.qt.io/contact-us
**
** This file is part of the Qt Event List module.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** If you have questions regarding the use of this file, please use
** contact form at http://www.qt.io/contact-us
**
******************************************************************************/

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
