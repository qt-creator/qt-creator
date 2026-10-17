// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifdef WITH_TESTS

#include "basetextdocument_test.h"

#include "coreconstants.h"
#include "icore.h"
#include "textdocument.h"

#include <utils/filepath.h>
#include <utils/qtcsettings.h>
#include <utils/textcodec.h>
#include <utils/temporarydirectory.h>

#include <QScopeGuard>
#include <QTest>

using namespace Utils;

namespace Core {

class BaseTextDocumentTest : public QObject
{
    Q_OBJECT

private slots:
    void testRereadAfterFallback()
    {
        QtcSettings *settings = ICore::settings();
        const Key fallbackKey(Constants::SETTINGS_FALLBACKTEXTENCODING);
        const QVariant previousFallback = settings->value(fallbackKey);
        const bool hadFallback = settings->contains(fallbackKey);
        const QScopeGuard restoreFallback([=] {
            if (hadFallback)
                settings->setValue(fallbackKey, previousFallback);
            else
                settings->remove(fallbackKey);
        });
        settings->setValue(fallbackKey, TextEncoding(TextEncoding::Latin1).name());

        TemporaryDirectory directory("basetextdocument-fallback-reread");
        QVERIFY(directory.isValid());
        const FilePath path = directory.filePath("input.txt");
        QVERIFY(path.writeFileContents(QByteArray("caf\xE9" "X", 5)));

        BaseTextDocument document;
        const TextEncoding utf8(TextEncoding::Utf8);
        document.setEncoding(utf8);
        QVERIFY(!document.isDecodingFallbackEnabled());
        QCOMPARE(document.read(path).code, TextFileFormat::ReadEncodingError);
        document.setDecodingFallbackEnabled(true);
        const auto firstRead = document.read(path);
        QCOMPARE(firstRead.code, TextFileFormat::ReadSuccess);
        QVERIFY(firstRead.usedDecodingFallback);
        QCOMPARE(document.encoding(), TextEncoding(TextEncoding::Latin1));
        QCOMPARE(document.failedPrimaryEncoding(), utf8);

        QVERIFY(path.writeFileContents(QByteArray::fromHex("636166c3a9")));
        const auto secondRead = document.read(path);
        QCOMPARE(secondRead.code, TextFileFormat::ReadSuccess);
        QVERIFY(!secondRead.usedDecodingFallback);
        QVERIFY(!document.decodedWithFallback());
        QCOMPARE(document.encoding(), utf8);
        QCOMPARE(secondRead.content, QString::fromUtf8(QByteArray::fromHex("636166c3a9")));
    }
};

QObject *createBaseTextDocumentTest()
{
    return new BaseTextDocumentTest;
}

} // namespace Core

#include "basetextdocument_test.moc"

#endif // WITH_TESTS
