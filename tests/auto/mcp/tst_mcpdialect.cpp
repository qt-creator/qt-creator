// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <mcp/server/mcpserver.h>

#include <QJsonObject>
#include <QTest>

using namespace Mcp;

Q_DECLARE_METATYPE(Mcp::Dialect)

// The revision a request states travels in params._meta, the way a client
// sends it.
static QJsonObject request(const QString &bodyVersion)
{
    QJsonObject message{{"jsonrpc", "2.0"}, {"id", 1}, {"method", "tools/list"}};
    if (!bodyVersion.isNull()) {
        message.insert(
            "params",
            QJsonObject{
                {"_meta",
                 QJsonObject{{"io.modelcontextprotocol/protocolVersion", bodyVersion}}}});
    }
    return message;
}

class tst_McpDialect : public QObject
{
    Q_OBJECT

private slots:
    void picksARevision_data()
    {
        QTest::addColumn<QString>("bodyVersion");
        QTest::addColumn<QString>("versionHeader");
        QTest::addColumn<Dialect>("expected");

        const QString none; // a null QString: the key is left out entirely

        // A body that names nothing is an older client, whatever the header
        // says about itself.
        QTest::newRow("silent body, no header") << none << QString() << Dialect::Legacy;
        QTest::newRow("silent body, old header")
            << none << QString("2025-11-25") << Dialect::Legacy;
        QTest::newRow("old body, matching old header")
            << QString("2025-11-25") << QString("2025-11-25") << Dialect::Legacy;
        QTest::newRow("old body, other old header")
            << QString("2025-06-18") << QString("2025-11-25") << Dialect::Legacy;

        // The revision stated in the body is the one answered, and a body
        // with no header behind it - stdio - is taken at its word.
        QTest::newRow("2026 body, no header")
            << QString("2026-07-28") << QString() << Dialect::Revision2026;
        QTest::newRow("2026 body, matching header")
            << QString("2026-07-28") << QString("2026-07-28") << Dialect::Revision2026;

        // The three shapes of disagreement, all refused rather than answered
        // under either revision.
        QTest::newRow("2026 header, silent body")
            << none << QString("2026-07-28") << Dialect::Mismatch;
        QTest::newRow("2026 header, old body")
            << QString("2025-11-25") << QString("2026-07-28") << Dialect::Mismatch;
        QTest::newRow("old header, 2026 body")
            << QString("2026-07-28") << QString("2025-11-25") << Dialect::Mismatch;

        // A revision this server has never heard of is refused, not guessed at.
        QTest::newRow("unknown body, no header")
            << QString("1999-01-01") << QString() << Dialect::Unsupported;
    }

    void picksARevision()
    {
        QFETCH(QString, bodyVersion);
        QFETCH(QString, versionHeader);
        QFETCH(Dialect, expected);

        QCOMPARE(dialectFor(request(bodyVersion), versionHeader), expected);
    }

    // A notification carries no id, and the revision is read the same way.
    void readsANotificationTheSameWay()
    {
        QJsonObject notification{
            {"jsonrpc", "2.0"},
            {"method", "notifications/cancelled"},
            {"params",
             QJsonObject{
                 {"_meta",
                  QJsonObject{{"io.modelcontextprotocol/protocolVersion", "2026-07-28"}}}}}};

        QCOMPARE(dialectFor(notification, "2026-07-28"), Dialect::Revision2026);
        QCOMPARE(dialectFor(notification, "2025-11-25"), Dialect::Mismatch);
    }

    // _meta that is not an object at all, which a client may still send.
    void survivesAnUnreadableMeta()
    {
        const auto withMeta = [](const QJsonValue &meta) {
            return QJsonObject{{"method", "tools/list"}, {"params", QJsonObject{{"_meta", meta}}}};
        };
        QCOMPARE(dialectFor(withMeta(42), QString()), Dialect::Legacy);
        QCOMPARE(dialectFor(withMeta(QJsonValue::Null), "2026-07-28"), Dialect::Mismatch);

        // params that is not an object either.
        QCOMPARE(
            dialectFor(QJsonObject{{"method", "tools/list"}, {"params", 42}}, QString()),
            Dialect::Legacy);
    }
};

QTEST_GUILESS_MAIN(tst_McpDialect)

#include "tst_mcpdialect.moc"
