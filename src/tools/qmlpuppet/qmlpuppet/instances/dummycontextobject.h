// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>
#include <QPointer>
#include <qqml.h>

namespace QmlDesigner {

class DummyContextObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QObject * parent READ parentDummy WRITE setParentDummy NOTIFY parentDummyChanged DESIGNABLE false FINAL)
    Q_PROPERTY(bool runningInDesigner READ runningInDesigner FINAL)

public:
    explicit DummyContextObject(QObject *parent = nullptr);

    QObject *parentDummy() const;
    void setParentDummy(QObject *parentDummy);
    bool runningInDesigner() const;

signals:
    void parentDummyChanged();

private:
    QPointer<QObject> m_dummyParent;
};

} // namespace QmlDesigner

QML_DECLARE_TYPE(QmlDesigner::DummyContextObject)
