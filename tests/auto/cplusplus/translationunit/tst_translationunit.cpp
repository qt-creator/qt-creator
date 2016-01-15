/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "../cplusplus_global.h"

#include <cplusplus/PreprocessorClient.h>
#include <cplusplus/PreprocessorEnvironment.h>
#include <cplusplus/Token.h>
#include <cplusplus/TranslationUnit.h>
#include <cplusplus/pp-engine.h>

#include <QtTest>
#include <QDebug>

#ifdef Q_OS_WIN
    #include <cctype>  // std:isspace
#endif

struct LineColumn
{
    LineColumn(unsigned line = 0, unsigned column = 0) : line(line), column(column) {}
    unsigned line;
    unsigned column;
};
typedef QList<LineColumn> LineColumnList;
Q_DECLARE_METATYPE(LineColumnList)

//TESTED_COMPONENT=src/libs/cplusplus
using namespace CPlusPlus;

class tst_TranslationUnit: public QObject
{
    Q_OBJECT
private slots:
    void unicodeIdentifier();
    void unicodeIdentifier_data();

    void unicodeStringLiteral();
    void unicodeStringLiteral_data();

    void locationOfUtf16CharOffset();
    void locationOfUtf16CharOffset_data();

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

    class TokenGetter
    {
    public:
        typedef QSharedPointer<TokenGetter> Ptr;
    public:
        TokenGetter(TranslationUnit *translationUnit) : m_translationUnit(translationUnit) {}
        virtual ~TokenGetter() {}
        virtual unsigned tokenCount() { return m_translationUnit->tokenCount(); }
        virtual Token tokenAt(unsigned index) { return m_translationUnit->tokenAt(index); }
    protected:
        TranslationUnit *m_translationUnit;
    };

    class CommentTokenGetter : public TokenGetter
    {
    public:
        CommentTokenGetter(TranslationUnit *translationUnit) : TokenGetter(translationUnit) {}
        unsigned tokenCount() { return m_translationUnit->commentCount(); }
        Token tokenAt(unsigned index) { return m_translationUnit->commentAt(index); }
    };

    static void compareTokenLocations(TranslationUnit *translationUnit,
                                      tst_TranslationUnit::TokenGetter *tokenGetter,
                                      const LineColumnList &expectedLinesColumns);
};

void tst_TranslationUnit::compareTokenLocations(TranslationUnit *translationUnit,
                                                tst_TranslationUnit::TokenGetter *tokenGetter,
                                                const LineColumnList &expectedLinesColumns)
{
    QCOMPARE(tokenGetter->tokenCount(), (unsigned) expectedLinesColumns.count());
    for (unsigned i = 0, tokenCount = tokenGetter->tokenCount(); i < tokenCount; ++i) {
        const LineColumn expected = expectedLinesColumns.at(i);
        const unsigned utf16CharOffset = tokenGetter->tokenAt(i).utf16charsBegin();
        unsigned line, column;
        translationUnit->getPosition(utf16CharOffset, &line, &column);
//        qDebug("%d: LineColumn(%u, %u)", i, line, column);
        QCOMPARE(line, expected.line);
        QCOMPARE(column, expected.column);
    }
}

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

    QTest::newRow("non-latin1 identifier 1") << _("prefix" UC_U00FC UC_U4E8C UC_U10302);
    QTest::newRow("non-latin1 identifier 2") << _("prefix" UC_U10302 UC_U00FC UC_U4E8C);
    QTest::newRow("non-latin1 identifier 3") << _(UC_U10302 UC_U00FC UC_U4E8C);
    QTest::newRow("non-latin1 identifier 4") << _(UC_U4E8C UC_U10302 UC_U00FC);
    QTest::newRow("non-latin1 identifier 5") << _(UC_U4E8C UC_U10302 UC_U00FC "suffix");
    QTest::newRow("non-latin1 identifier 6") << _(UC_U10302 UC_U00FC UC_U4E8C "suffix");

    // Some special cases (different code path inside lexer)
    QTest::newRow("non-latin1 identifier 7") << _("LR" UC_U10302 UC_U00FC UC_U4E8C);
    QTest::newRow("non-latin1 identifier 8") << _("u8R" UC_U10302 UC_U00FC UC_U4E8C);
    QTest::newRow("non-latin1 identifier 9") << _("u8" UC_U10302 UC_U00FC UC_U4E8C);
    QTest::newRow("non-latin1 identifier 10") << _("u" UC_U10302 UC_U00FC UC_U4E8C);
}

static QByteArray stripEncodingPrefixAndQuotationMarks(const QByteArray &literal)
{
    const char quotationMark = '"';
    const int firstQuotationMarkPosition = literal.indexOf(quotationMark);
    const int lastQuotationMarkPosition = literal.lastIndexOf(quotationMark);
    Q_ASSERT(firstQuotationMarkPosition != -1);
    Q_ASSERT(lastQuotationMarkPosition == literal.size() - 1);
    Q_ASSERT(firstQuotationMarkPosition < lastQuotationMarkPosition - 1);
    Q_UNUSED(lastQuotationMarkPosition);

    QByteArray result = literal.mid(firstQuotationMarkPosition + 1);
    result.chop(1);
    return result;
}

void tst_TranslationUnit::unicodeStringLiteral()
{
    QFETCH(QByteArray, literalText);

    Document::Ptr document = Document::create("char t[] = " + literalText + ";");
    QVERIFY(document);

    const StringLiteral *actual = document->lastStringLiteral();
    QCOMPARE(QString::fromUtf8(actual->chars(), actual->size()),
             QString::fromUtf8(stripEncodingPrefixAndQuotationMarks(literalText)));
}

void tst_TranslationUnit::unicodeStringLiteral_data()
{
    QTest::addColumn<QByteArray>("literalText");

    typedef QByteArray _;

    QTest::newRow("latin1 literal") << _("\"var\"");

    QTest::newRow("non-latin1 literal 1") << _("\"prefix" UC_U00FC UC_U4E8C UC_U10302 "\"");
    QTest::newRow("non-latin1 literal 2") << _("\"prefix" UC_U10302 UC_U00FC UC_U4E8C"\"");
    QTest::newRow("non-latin1 literal 3") << _("\"" UC_U10302 UC_U00FC UC_U4E8C "\"");
    QTest::newRow("non-latin1 literal 4") << _("\"" UC_U4E8C UC_U10302 UC_U00FC "\"");
    QTest::newRow("non-latin1 literal 5") << _("\"" UC_U4E8C UC_U10302 UC_U00FC "suffix\"");
    QTest::newRow("non-latin1 literal 6") << _("\"" UC_U10302 UC_U00FC UC_U4E8C "suffix\"");

    QTest::newRow("non-latin1 literal 7") << _("L\"U10302U00FCU4E8C\"");
    QTest::newRow("non-latin1 literal 8") << _("u8\"U10302U00FCU4E8C\"");
    QTest::newRow("non-latin1 literal 9") << _("u\"U10302U00FCU4E8C\"");
    QTest::newRow("non-latin1 literal 10") << _("U\"U10302U00FCU4E8C\"");
}

void tst_TranslationUnit::locationOfUtf16CharOffset()
{
    QFETCH(QByteArray, source);
    QFETCH(LineColumnList, expectedNonCommentLinesColumns);
    QFETCH(LineColumnList, expectedCommentLinesColumns);

    Document::Ptr document = Document::create(source);
    TranslationUnit *translationUnit = document->translationUnit();

    const TokenGetter::Ptr nonCommentTokenGetter(new TokenGetter(translationUnit));
    compareTokenLocations(translationUnit, nonCommentTokenGetter.data(),
                          expectedNonCommentLinesColumns);

    const TokenGetter::Ptr commentTokenGetter(new CommentTokenGetter(translationUnit));
    compareTokenLocations(translationUnit, commentTokenGetter.data(), expectedCommentLinesColumns);
}

void tst_TranslationUnit::locationOfUtf16CharOffset_data()
{
    QTest::addColumn<QByteArray>("source");
    QTest::addColumn<LineColumnList>("expectedNonCommentLinesColumns");
    QTest::addColumn<LineColumnList>("expectedCommentLinesColumns");

    typedef QByteArray _;

    QTest::newRow("empty")
        << _("")
        << (LineColumnList()
            << LineColumn(0, 0)
            << LineColumn(0, 0)
           )
        << LineColumnList();

    // --- Identifiers ---------------------------------------------------------------------------

    QTest::newRow("latin1 identifiers")
        << _("int i;")
        << (LineColumnList()
            << LineColumn(0, 0)
            << LineColumn(1, 1) // int
            << LineColumn(1, 5) // i
            << LineColumn(1, 6) // ;
            << LineColumn(1, 7)
           )
        << LineColumnList();

    QTest::newRow("latin1 identifiers")
        << _("int i;\n"
             "int jj;\n")
        << (LineColumnList()
            << LineColumn(0, 0)
            << LineColumn(1, 1) // int 1
            << LineColumn(1, 5) // i
            << LineColumn(1, 6) // ;
            << LineColumn(2, 1) // int 2
            << LineColumn(2, 5) // jj
            << LineColumn(2, 7) // ;
            << LineColumn(3, 1)
           )
        << LineColumnList();

    QTest::newRow("non-latin1 identifier")
        << _("int " UC_U00FC ";")
        << (LineColumnList()
            << LineColumn(0, 0)
            << LineColumn(1, 1) // int
            << LineColumn(1, 5) // non-latin1 identifier
            << LineColumn(1, 6) // ;
            << LineColumn(1, 7)
           )
        << LineColumnList();

    QTest::newRow("non-latin1 identifiers 1")
        << _("int " UC_U00FC ";\n"
             "int " UC_U00FC ";")
        << (LineColumnList()
            << LineColumn(0, 0)
            << LineColumn(1, 1) // int 1
            << LineColumn(1, 5) // non-latin1 identifier 1
            << LineColumn(1, 6) // ;
            << LineColumn(2, 1) // int 2
            << LineColumn(2, 5) // non-latin1 identifier 2
            << LineColumn(2, 6) // ;
            << LineColumn(2, 7) // ;
           )
        << LineColumnList();

    QTest::newRow("non-latin1 identifiers 2")
        << _("int " UC_U00FC UC_U4E8C UC_U10302 ";\n"
             "int v;\n"
             "int " UC_U10302 UC_U4E8C ";")
        << (LineColumnList()
            << LineColumn(0, 0)
            << LineColumn(1, 1) // int 1
            << LineColumn(1, 5) // non-latin1 identifier 1
            << LineColumn(1, 9) // ;
            << LineColumn(2, 1) // int 2
            << LineColumn(2, 5) // non-latin1 identifier 2
            << LineColumn(2, 6) // ;
            << LineColumn(3, 1) // int 3
            << LineColumn(3, 5) // non-latin1 identifier 3
            << LineColumn(3, 8) // ;
            << LineColumn(3, 9)
           )
        << LineColumnList();

    // --- String literals -----------------------------------------------------------------------

    QTest::newRow("latin1 string literal")
        << _("char t[] = \"foo\";")
        << (LineColumnList()
            << LineColumn(0, 0)
            << LineColumn(1, 1)  // char
            << LineColumn(1, 6)  // t
            << LineColumn(1, 7)  // [
            << LineColumn(1, 8)  // ]
            << LineColumn(1, 10) // =
            << LineColumn(1, 12) // latin1 string literal
            << LineColumn(1, 17) // ;
            << LineColumn(1, 18)
           )
        << LineColumnList();

    QTest::newRow("non-latin1 string literal")
        << _("char t[] = \"i" UC_U00FC UC_U4E8C UC_U10302 "\";")
        << (LineColumnList()
            << LineColumn(0, 0)
            << LineColumn(1, 1)  // char
            << LineColumn(1, 6)  // t
            << LineColumn(1, 7)  // [
            << LineColumn(1, 8)  // ]
            << LineColumn(1, 10) // =
            << LineColumn(1, 12) // non-latin1 string literal
            << LineColumn(1, 19) // ;
            << LineColumn(1, 20)
           )
        << LineColumnList();

    QTest::newRow("non-latin1 string literal multiple lines")
        << _("char t[] = \"i" UC_U00FC UC_U4E8C UC_U10302 " \\\n"
             "\";")
        << (LineColumnList()
            << LineColumn(0, 0)
            << LineColumn(1, 1)  // char
            << LineColumn(1, 6)  // t
            << LineColumn(1, 7)  // [
            << LineColumn(1, 8)  // ]
            << LineColumn(1, 10) // =
            << LineColumn(1, 12) // non-latin1 string literal
            << LineColumn(2, 2)  // ;
            << LineColumn(2, 3)
           )
        << LineColumnList();

    // --- Comments ------------------------------------------------------------------------------

    QTest::newRow("latin1 c++ comment line")
        << _("// comment line\n"
             "int i;")
        << (LineColumnList()
            << LineColumn(0, 0)
            << LineColumn(2, 1) // int
            << LineColumn(2, 5) // i
            << LineColumn(2, 6) // ;
            << LineColumn(2, 7)
           )
        << (LineColumnList()
            << LineColumn(1, 1) // comment
           );

    QTest::newRow("latin1 c comment line")
        << _("/* comment line */ int i;")
        << (LineColumnList()
            << LineColumn(0, 0)
            << LineColumn(1, 20) // int
            << LineColumn(1, 24) // i
            << LineColumn(1, 25) // ;
            << LineColumn(1, 26)
           )
        << (LineColumnList()
            << LineColumn(1, 1)  // comment
           );

    QTest::newRow("latin1 c comment lines")
        << _("/* comment line 1\n"
             "   comment line 2 */ int i;")
        << (LineColumnList()
            << LineColumn(0, 0)
            << LineColumn(2, 22) // int
            << LineColumn(2, 26) // i
            << LineColumn(2, 27) // ;
            << LineColumn(2, 28)
           )
        << (LineColumnList()
            << LineColumn(1, 1)  // comment
           );

    QTest::newRow("non-latin1 c++ comment line")
        << _("// comment line " UC_U00FC UC_U4E8C UC_U10302 "\n"
             "int i;")
        << (LineColumnList()
            << LineColumn(0, 0)
            << LineColumn(2, 1) // int
            << LineColumn(2, 5) // i
            << LineColumn(2, 6) // ;
            << LineColumn(2, 7)
           )
        << (LineColumnList()
            << LineColumn(1, 1) // comment
           );

    QTest::newRow("non-latin1 c comment lines")
        << _("/* comment line 1\n"
             "   comment line 2 */ int i;")
        << (LineColumnList()
            << LineColumn(0, 0)
            << LineColumn(2, 22) // int
            << LineColumn(2, 26) // i
            << LineColumn(2, 27) // ;
            << LineColumn(2, 28)
           )
        << (LineColumnList()
            << LineColumn(1, 1)  // comment
           );
}

QTEST_APPLESS_MAIN(tst_TranslationUnit)
#include "tst_translationunit.moc"
