// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace QmlDesigner {
namespace Internal {

class ChildrenChangeEventFilter : public QObject
{
    Q_OBJECT
public:
    ChildrenChangeEventFilter(QObject *parent);


signals:
    void childrenChanged(QObject *object);

protected:
    bool eventFilter(QObject *object, QEvent *event) override;

};

} // namespace Internal
} // namespace QmlDesigner
