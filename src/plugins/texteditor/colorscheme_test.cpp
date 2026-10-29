// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "colorscheme_test.h"

#include "colorscheme.h"

#include <utils/filepath.h>

#include <QTemporaryDir>
#include <QTest>

using namespace Utils;

namespace TextEditor::Internal {

class ColorSchemeTest final : public QObject
{
    Q_OBJECT

private slots:
    void testSaveLoadRoundTrip_data();
    void testSaveLoadRoundTrip();
};

void ColorSchemeTest::testSaveLoadRoundTrip_data()
{
    QTest::addColumn<QColor>("color");
    QTest::addColumn<QString>("savedName");

    QTest::newRow("opaque") << QColor(0x12, 0x34, 0x56) << QString("#123456");
    QTest::newRow("translucent") << QColor(0x12, 0x34, 0x56, 0x80) << QString("#80123456");
}

void ColorSchemeTest::testSaveLoadRoundTrip()
{
    QFETCH(QColor, color);
    QFETCH(QString, savedName);

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const FilePath filePath = FilePath::fromString(dir.path()) / "scheme.xml";

    Format format;
    format.setForeground(color);
    format.setBackground(color);
    format.setUnderlineColor(color);
    format.setUnderlineStyle(QTextCharFormat::SingleUnderline);

    ColorScheme scheme;
    scheme.setDisplayName("Test");
    scheme.setFormatFor(C_TEXT, format);
    QVERIFY(scheme.save(filePath));

    const Result<QByteArray> contents = filePath.fileContents();
    QVERIFY(contents);
    const QString xml = QString::fromUtf8(*contents);
    QVERIFY(xml.contains(QString("foreground=\"%1\"").arg(savedName)));
    QVERIFY(xml.contains(QString("background=\"%1\"").arg(savedName)));
    QVERIFY(xml.contains(QString("underlineColor=\"%1\"").arg(savedName)));

    ColorScheme loaded;
    QVERIFY(loaded.load(filePath));
    const Format loadedFormat = loaded.formatFor(C_TEXT);
    QCOMPARE(loadedFormat.foreground(), color);
    QCOMPARE(loadedFormat.background(), color);
    QCOMPARE(loadedFormat.underlineColor(), color);
}

QObject *createColorSchemeTest()
{
    return new ColorSchemeTest;
}

} // namespace TextEditor::Internal

#include "colorscheme_test.moc"
