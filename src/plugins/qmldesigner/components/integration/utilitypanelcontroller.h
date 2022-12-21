// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

QT_FORWARD_DECLARE_CLASS(QWidget)

namespace QmlDesigner {

class UtilityPanelController : public QObject
{
    Q_OBJECT

public:
    UtilityPanelController(QObject* parent = nullptr);
    ~UtilityPanelController() override = 0;

    QWidget* widget();

protected:
    virtual class QWidget* contentWidget() const = 0;
};

} // namespace QmlDesigner
