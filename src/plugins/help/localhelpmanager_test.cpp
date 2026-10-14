// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifdef WITH_TESTS

#include "localhelpmanager_test.h"

#include "localhelpmanager.h"

#include <QFile>
#include <QTest>

namespace Help::Internal {

// Verifies that a dark Qt Creator theme substitutes a dark stylesheet for the
// documentation's default light one, so every help viewer backend follows the theme,
// and that the figures it serves are told which page they are drawn on.
class LocalHelpManagerTest final : public QObject
{
    Q_OBJECT

private slots:
    void testDarkDocumentationCss();
    void testDarkThemeSvg();
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

void LocalHelpManagerTest::testDarkThemeSvg()
{
    // A drawing that names no class of its own is given one.
    QCOMPARE(darkThemeSvg("<svg width=\"8\"><g/></svg>"),
             QByteArray("<svg class=\"dark\" width=\"8\"><g/></svg>"));

    // One that already names a class is added to: a second class attribute would not be XML.
    QCOMPARE(darkThemeSvg("<svg class=\"figure\" width=\"8\"/>"),
             QByteArray("<svg class=\"figure dark\" width=\"8\"/>"));

    // An attribute value may hold the '>' that would otherwise end the start tag.
    QCOMPARE(darkThemeSvg("<svg aria-label=\"a &gt; b\" id='a>b'/>"),
             QByteArray("<svg class=\"dark\" aria-label=\"a &gt; b\" id='a>b'/>"));

    // The prolog is left as it was: an entity declared in the internal subset of the DOCTYPE
    // and used on the root, as Illustrator writes them, still resolves.
    const QByteArray illustrator
        = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
          "<!-- Generator: Adobe Illustrator, \302\251 2026 -->\n"
          "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"svg11.dtd\" [\n"
          "\t<!ENTITY ns_svg \"http://www.w3.org/2000/svg\">\n"
          "]>\n"
          "<svg xmlns=\"&ns_svg;\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" width=\"8\">\n"
          "<image xlink:href=\"a.png\"/>\n"
          "</svg>\n";
    const QByteArray themed = darkThemeSvg(illustrator);
    QVERIFY(themed.contains("<svg class=\"dark\" xmlns=\"&ns_svg;\" xmlns:xlink="));
    // Nothing but the root start tag is touched, and the comment before it holds a character
    // that is two bytes long, where a byte offset and a character offset part ways.
    const QByteArray added = " class=\"dark\"";
    QCOMPARE(themed.size(), illustrator.size() + added.size());
    const qsizetype root = illustrator.indexOf("<svg ");
    QCOMPARE(themed.left(root), illustrator.left(root));
    QVERIFY(themed.endsWith("<image xlink:href=\"a.png\"/>\n</svg>\n"));

    // A byte order mark is not part of the markup.
    QCOMPARE(darkThemeSvg("\357\273\277<svg/>"), QByteArray("\357\273\277<svg class=\"dark\"/>"));

    // A root element that carries a prefix is still the root element.
    QCOMPARE(darkThemeSvg("<svg:svg xmlns:svg=\"http://www.w3.org/2000/svg\"/>"),
             QByteArray("<svg:svg class=\"dark\" xmlns:svg=\"http://www.w3.org/2000/svg\"/>"));

    // What does not read as an SVG document is handed back as it came: a drawing is never
    // made worse than it was.
    for (const QByteArray &untouched : {QByteArray("<html><body/></html>"),
                                        QByteArray("\037\213\010compressed svgz"),
                                        QByteArray("<svg width=\"8\""),
                                        QByteArray("<!-- unterminated <svg/>"),
                                        QByteArray()}) {
        QCOMPARE(darkThemeSvg(untouched), untouched);
    }
}

QObject *createLocalHelpManagerTest()
{
    return new LocalHelpManagerTest;
}

} // namespace Help::Internal

#include "localhelpmanager_test.moc"

#endif // WITH_TESTS
