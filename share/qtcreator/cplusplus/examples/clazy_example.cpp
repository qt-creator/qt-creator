// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

// -Wclazy-qt-macros
#ifdef Q_OS_XXX
#endif

#include <QCoreApplication>
#include <QDateTime>
#include <QFileInfo>
#include <QMap>
#include <QObject>
#include <QPoint>
#include <QPointer>

// -Wclazy-ctor-missing-parent-argument
class TestObject : public QObject
{
    Q_OBJECT

public:
    TestObject();
    TestObject(const TestObject& other) {}

    bool event(QEvent *event) override
    {
        // -Wclazy-base-class-event
        return false;
    }

public slots:
    void someSlot() {}

    // -Wclazy-function-args-by-value
    void someOtherSlot(const QPoint &point)
    {
        point.isNull();
    }

signals:
    void someSignal();
private:
    char m_bigArray[100];
};

// -Wclazy-copyable-polymorphic
class TestObjectDerived : public TestObject
{

};

QList<int> getList()
{
    QList<int> list;
    return list;
}

TestObject::TestObject()
{
    // -Wclazy-old-style-connect
    QObject::connect(this, SIGNAL(someSlot()), SLOT(someOtherSlot()));

    int a;

    // -Wclazy-connect-non-signal, -Wclazy-lambda-in-connect
    QObject::connect(this, &TestObject::someSlot, [&a]() {
        return;
    });

    // -Wclazy-incorrect-emit
    someSignal();

    QMap<int, int *> map;

    // -Wclazy-unused-non-trivial-variable, -Wclazy-mutable-container-key
    QMap<QPointer<QObject>, int> map2;

    // -Wclazy-container-anti-pattern, -Wclazy-range-loop
    for (auto value : map.values()) {
    }

    // -Wclazy-qdeleteall
    qDeleteAll(map.values());
    for (auto it = getList().begin(); it != getList().end(); ++it) { // -Wclazy-detaching-temporary, -Wclazy-temporary-iterator
    }

    // -Wclazy-inefficient-qlist-soft
    QList<TestObject> list;

    // -Wclazy-range-loop
    for (auto obj : list) {
    }

    // -Wclazy-qfileinfo-exists
    // -Wclazy-qstring-allocations
    QFileInfo("filename").exists();

    // -Wclazy-qlatin1string-non-ascii
    QLatin1String latinStr("ййй");

    QString str = latinStr;

    // -Wclazy-qstring-left
    str = str.left(1);

    bool ok;

    // -Wclazy-qstring-ref
    str.mid(5).toInt(&ok);

    // -Wclazy-qstring-arg
    (void) QString("%1 %2").arg("1").arg("2");
}
