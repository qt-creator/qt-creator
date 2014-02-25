/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include <cplusplus/PreprocessorClient.h>
#include <cplusplus/PreprocessorEnvironment.h>
#include <cplusplus/Token.h>
#include <cplusplus/TranslationUnit.h>
#include <cplusplus/pp-engine.h>

#include <QtTest>
#include <QDebug>

//TESTED_COMPONENT=src/libs/cplusplus
using namespace CPlusPlus;

class tst_TranslationUnit: public QObject
{
    Q_OBJECT
private slots:

    //
    // The following "non-latin1" code points are used in the tests following this comment:
    //
    //   U+00FC  - 2 code units in UTF8, 1 in UTF16 - LATIN SMALL LETTER U WITH DIAERESIS
    //   U+4E8C  - 3 code units in UTF8, 1 in UTF16 - CJK UNIFIED IDEOGRAPH-4E8C
    //   U+10302 - 4 code units in UTF8, 2 in UTF16 - OLD ITALIC LETTER KE
    //

    void unicodeIdentifier();
    void unicodeIdentifier_data();

    void unicodeStringLiteral();
    void unicodeStringLiteral_data();

private:
    class Document
    {
    public:
        typedef QSharedPointer<Document> Ptr;

        static Document::Ptr create(const QByteArray &source)
        {
            LanguageFeatures features;
            features.objCEnabled = true;
            features.qtEnabled = false;
            features.qtKeywordsEnabled = false;
            features.qtMocRunEnabled = false;

            Document::Ptr document = Document::Ptr(new Document);
            document->translationUnit()->setLanguageFeatures(features);
            const QByteArray preprocessedSource = preprocess(source);
            document->translationUnit()->setSource(preprocessedSource.constData(),
                                                preprocessedSource.length());
            document->translationUnit()->parse();

            if (document->hasParsingErrors())
                return Document::Ptr();
            return document;
        }

    public:
        Document()
            : m_translationUnit(&m_control, m_control.stringLiteral("testFile"))
        {
            m_control.setDiagnosticClient(&m_diagnosticClient);
        }

        TranslationUnit *translationUnit()
        { return &m_translationUnit; }

        bool hasParsingErrors() const
        { return m_diagnosticClient.errorCount != 0; }

        const Identifier *lastIdentifier() const
        { return *(m_control.lastIdentifier() - 1); }

        const StringLiteral *lastStringLiteral() const
        { return *(m_control.lastStringLiteral() - 1); }

    private:
        static QByteArray preprocess(const QByteArray &source)
        {
            Client *client = 0; // no client.
            Environment env;
            Preprocessor preprocess(client, &env);
            preprocess.setKeepComments(true);
            return preprocess.run(QLatin1String("<stdin>"), source);
        }

    private:
        Control m_control;
        TranslationUnit m_translationUnit;

        class Diagnostic: public DiagnosticClient {
        public:
            int errorCount;

            Diagnostic() : errorCount(0) {}

            void report(int /*level*/, const StringLiteral *fileName, unsigned line,
                        unsigned column, const char *format, va_list ap)
            {
                ++errorCount;
                qDebug() << fileName->chars() << ':' << line << ':' << column
                         << ' ' << QString().vsprintf(format, ap);
            }
        } m_diagnosticClient;
    };
};

void tst_TranslationUnit::unicodeIdentifier()
{
    QFETCH(QByteArray, identifierText);

    Document::Ptr document = Document::create("void " + identifierText + ";");
    QVERIFY(document);

    const Identifier *actual = document->lastIdentifier();
    QCOMPARE(QString::fromUtf8(actual->chars(), actual->size()),
             QString::fromUtf8(identifierText));
}

void tst_TranslationUnit::unicodeIdentifier_data()
{
    QTest::addColumn<QByteArray>("identifierText");

    typedef QByteArray _;

    QTest::newRow("latin1 identifier") << _("var");

    QTest::newRow("non-latin1 identifier 1") << _("prefix\u00FC\u4E8C\U00010302");
    QTest::newRow("non-latin1 identifier 2") << _("prefix\U00010302\u00FC\u4E8C");
    QTest::newRow("non-latin1 identifier 3") << _("\U00010302\u00FC\u4E8C");
    QTest::newRow("non-latin1 identifier 4") << _("\u4E8C\U00010302\u00FC");
    QTest::newRow("non-latin1 identifier 5") << _("\u4E8C\U00010302\u00FCsuffix");
    QTest::newRow("non-latin1 identifier 6") << _("\U00010302\u00FC\u4E8Csuffix");

    // Some special cases (different code path inside lexer)
    QTest::newRow("non-latin1 identifier 7") << _("LR\U00010302\u00FC\u4E8C");
    QTest::newRow("non-latin1 identifier 8") << _("u8R\U00010302\u00FC\u4E8C");
    QTest::newRow("non-latin1 identifier 9") << _("u8\U00010302\u00FC\u4E8C");
    QTest::newRow("non-latin1 identifier 10") << _("u\U00010302\u00FC\u4E8C");
}

static QByteArray stripQuotesFromLiteral(const QByteArray literal)
{
    QByteArray result = literal;

    // Strip front
    while (!result.isEmpty() && result[0] != '"')
        result = result.mid(1);
    if (result.isEmpty())
        return QByteArray();
    result = result.mid(1);

    // Strip end
    while (result.size() >= 2
           && (std::isspace(result[result.size() - 1]) || result[result.size()-1] == '"')) {
        result.chop(1);
    }

    return result;
}

void tst_TranslationUnit::unicodeStringLiteral()
{
    QFETCH(QByteArray, literalText);

    Document::Ptr document = Document::create("char t[] = " + literalText + ";");
    QVERIFY(document);

    const StringLiteral *actual = document->lastStringLiteral();
    QCOMPARE(QString::fromUtf8(actual->chars(), actual->size()),
             QString::fromUtf8(stripQuotesFromLiteral(literalText)));
}

void tst_TranslationUnit::unicodeStringLiteral_data()
{
    QTest::addColumn<QByteArray>("literalText");

    typedef QByteArray _;

    QTest::newRow("latin1 literal") << _("\"var\"");

    QTest::newRow("non-latin1 literal 1") << _("\"prefix\u00FC\u4E8C\U00010302\"");
    QTest::newRow("non-latin1 literal 2") << _("\"prefix\U00010302\u00FC\u4E8C\"");
    QTest::newRow("non-latin1 literal 3") << _("\"\U00010302\u00FC\u4E8C\"");
    QTest::newRow("non-latin1 literal 4") << _("\"\u4E8C\U00010302\u00FC\"");
    QTest::newRow("non-latin1 literal 5") << _("\"\u4E8C\U00010302\u00FCsuffix\"");
    QTest::newRow("non-latin1 literal 6") << _("\"\U00010302\u00FC\u4E8Csuffix\"");

    QTest::newRow("non-latin1 literal 7") << _("L\"\U00010302\u00FC\u4E8C\"");
    QTest::newRow("non-latin1 literal 8") << _("u8\"\U00010302\u00FC\u4E8C\"");
    QTest::newRow("non-latin1 literal 9") << _("u\"\U00010302\u00FC\u4E8C\"");
    QTest::newRow("non-latin1 literal 10") << _("U\"\U00010302\u00FC\u4E8C\"");
}

QTEST_APPLESS_MAIN(tst_TranslationUnit)
#include "tst_translationunit.moc"
