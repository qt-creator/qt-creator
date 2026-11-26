// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "scxmleditor_test.h"

#include "scxmldocument.h"

#include <QBuffer>
#include <QTest>

using namespace ScxmlEditor::PluginInterface;

namespace ScxmlEditor::Internal {

class ScxmlEditorTest final : public QObject
{
    Q_OBJECT

private slots:
    void testKeepTextContent_data();
    void testKeepTextContent();
};

void ScxmlEditorTest::testKeepTextContent_data()
{
    QTest::addColumn<QByteArray>("element");
    QTest::addColumn<QByteArray>("text");

    QTest::newRow("data") << QByteArray("<datamodel><data id=\"v\">\n    [1,2,3]\n  </data>"
                                        "</datamodel>")
                          << QByteArray("[1,2,3]");
    QTest::newRow("data with comment")
        << QByteArray("<datamodel><data id=\"v\">[1,<!-- two -->2]</data></datamodel>")
        << QByteArray("[1,2]");
    QTest::newRow("content") << QByteArray("<final id=\"f\"><donedata><content>42</content>"
                                           "</donedata></final>")
                             << QByteArray("42");
    QTest::newRow("script") << QByteArray("<script>var x = 1;</script>")
                            << QByteArray("var x = 1;");
}

void ScxmlEditorTest::testKeepTextContent()
{
    QFETCH(QByteArray, element);
    QFETCH(QByteArray, text);

    QByteArray input = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
                       "<scxml xmlns=\"http://www.w3.org/2005/07/scxml\" version=\"1.0\">"
                       + element + "</scxml>";
    QBuffer buffer(&input);
    QVERIFY(buffer.open(QIODevice::ReadOnly));

    ScxmlDocument document;
    QVERIFY(document.load(&buffer));

    const QByteArray saved = document.content();
    QVERIFY2(saved.contains(text), saved.constData());
}

QObject *createScxmlEditorTest()
{
    return new ScxmlEditorTest;
}

} // ScxmlEditor::Internal

#include "scxmleditor_test.moc"
