// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qmldebug/baseenginedebugclient.h>

#include <QDataStream>
#include <QObject>
#include <QTest>
#include <QUrl>

using namespace QmlDebug;

// The decode() helpers are protected; expose them for testing. A null debug
// connection is fine because decode() only touches the passed QDataStream.
class TestEngineDebugClient : public BaseEngineDebugClient
{
public:
    TestEngineDebugClient()
        : BaseEngineDebugClient(QLatin1String("TestEngineDebugClient"), nullptr)
    {}

    using BaseEngineDebugClient::decode;
};

class tst_BaseEngineDebugClient : public QObject
{
    Q_OBJECT

private slots:
    void deeplyNestedObjectTree();
    void deeplyNestedContextTree();
};

// Regression test for QTCREATORBUG-33434. The QML object/context trees are
// decoded from the wire into recursive value types (ObjectReference /
// ContextReference). We feed a chain far deeper than any real QML hierarchy and
// deeper than the point at which such a tree can no longer be built, walked or
// destroyed without overflowing the call stack. decode() must handle it without
// crashing by decoding iteratively and capping the depth, so the returned tree
// stays shallow enough to be safe. This chain length overflows both the original
// recursive decode (on build) and an uncapped iterative decode (on the value
// type's recursive destructor).
static const int InputDepth = 50000;
static const int StreamVersion = QDataStream::Qt_DefaultCompiledVersion;

static void writeObjectHeader(QDataStream &ds, int objectId, int parentId)
{
    // Matches operator>>(QDataStream &, QmlObjectData &) followed by parentId:
    // url, lineNumber, columnNumber, idString, objectName, objectType,
    // objectId, contextId, parentId.
    ds << QUrl() << 0 << 0 << QString() << QString() << QString()
       << objectId << 0 << parentId;
}

void tst_BaseEngineDebugClient::deeplyNestedObjectTree()
{
    // Serialize a linear chain of InputDepth objects, each the single child of
    // the previous one. The wire format places an object's properties after its
    // entire child subtree, so all headers/child-counts come first (descending)
    // and the (empty) property counts follow (one per level).
    QByteArray data;
    {
        QDataStream ds(&data, QIODevice::WriteOnly);
        ds.setVersion(StreamVersion);
        for (int i = 0; i < InputDepth; ++i) {
            writeObjectHeader(ds, i, i - 1);
            const int childCount = (i < InputDepth - 1) ? 1 : 0;
            ds << childCount << true; // childCount, recur
        }
        for (int i = 0; i < InputDepth; ++i)
            ds << int(0); // propCount, read while unwinding
    }

    QDataStream ds(&data, QIODevice::ReadOnly);
    ds.setVersion(StreamVersion);

    TestEngineDebugClient client;
    ObjectReference root;
    client.decode(ds, root, false); // must not crash

    // Count the decoded chain depth iteratively (the tree itself may be deep).
    int depth = 0;
    for (const ObjectReference *o = &root; ; ++depth) {
        if (o->children().isEmpty())
            break;
        o = &o->children().first();
    }
    ++depth; // count the leaf

    // The tree was capped well below the input depth, so it is safe to hold and
    // destroy. The exact cap is an implementation detail; use a generous bound.
    QVERIFY(depth > 0);
    QVERIFY(depth < InputDepth);
    QVERIFY2(depth <= 8192, qPrintable(QString("decoded depth not capped: %1").arg(depth)));
}

void tst_BaseEngineDebugClient::deeplyNestedContextTree()
{
    // Same idea for the context tree: a linear chain of InputDepth contexts. Per
    // context the wire format is name, debugId, contextCount, <child contexts>,
    // objectCount, <objects>; the objectCount is read while unwinding.
    QByteArray data;
    {
        QDataStream ds(&data, QIODevice::WriteOnly);
        ds.setVersion(StreamVersion);
        for (int i = 0; i < InputDepth; ++i) {
            const int contextCount = (i < InputDepth - 1) ? 1 : 0;
            ds << QString() << i << contextCount; // name, debugId, contextCount
        }
        for (int i = 0; i < InputDepth; ++i)
            ds << int(0); // objectCount, read while unwinding
    }

    QDataStream ds(&data, QIODevice::ReadOnly);
    ds.setVersion(StreamVersion);

    TestEngineDebugClient client;
    ContextReference root;
    client.decode(ds, root); // must not crash

    int depth = 0;
    for (const ContextReference *c = &root; ; ++depth) {
        if (c->contexts().isEmpty())
            break;
        c = &c->contexts().first();
    }
    ++depth;

    QVERIFY(depth > 0);
    QVERIFY(depth < InputDepth);
    QVERIFY2(depth <= 8192, qPrintable(QString("decoded depth not capped: %1").arg(depth)));
}

QTEST_GUILESS_MAIN(tst_BaseEngineDebugClient)

#include "tst_baseenginedebugclient.moc"
