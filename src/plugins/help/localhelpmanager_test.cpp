// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifdef WITH_TESTS

#include "localhelpmanager_test.h"

#include "localhelpmanager.h"

#include <QFile>
#include <QTest>

namespace Help::Internal {

// Verifies that a dark Qt Creator theme substitutes a dark stylesheet for the
// documentation's default light one, so every help viewer backend follows the
// theme (QTCREATORBUG-8465).
class LocalHelpManagerTest final : public QObject
{
    Q_OBJECT

private slots:
    void testDarkDocumentationCss();
};

void LocalHelpManagerTest::testDarkDocumentationCss()
{
    QFile bundled(":/help/offline-dark.css");
    QVERIFY(bundled.open(QIODevice::ReadOnly));
    const QByteArray darkCss = bundled.readAll();
    QVERIFY(!darkCss.isEmpty());

    // No documentation is registered in the test, so the request for the dark
    // stylesheet falls back to the one bundled with Qt Creator.
    const QUrl cssUrl("qthelp://org.qt-project.example/doc/style/offline.css");
    const LocalHelpManager::HelpData dark = LocalHelpManager::helpData(cssUrl, true);
    QCOMPARE(dark.mimeType, QString("text/css"));
    QCOMPARE(dark.data, darkCss);

    // The simple stylesheet (served to JavaScript-less viewers) is substituted too.
    const QUrl simpleUrl("qthelp://org.qt-project.example/doc/style/offline-simple.css");
    QCOMPARE(LocalHelpManager::helpData(simpleUrl, true).data, darkCss);

    // A light theme leaves the stylesheet request untouched.
    QVERIFY(LocalHelpManager::helpData(cssUrl, false).data != darkCss);
}

QObject *createLocalHelpManagerTest()
{
    return new LocalHelpManagerTest;
}

} // namespace Help::Internal

#include "localhelpmanager_test.moc"

#endif // WITH_TESTS
