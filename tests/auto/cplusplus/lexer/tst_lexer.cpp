/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include <cplusplus/Token.h>
#include <cplusplus/SimpleLexer.h>

#include <QtTest>
#include <QDebug>

Q_DECLARE_METATYPE(QList<unsigned>)

//TESTED_COMPONENT=src/libs/cplusplus
using namespace CPlusPlus;

class tst_SimpleLexer: public QObject
{
    Q_OBJECT

private slots:
    void doxygen_comments();
    void doxygen_comments_data();
};

void tst_SimpleLexer::doxygen_comments()
{
    QFETCH(QByteArray, source);
    QFETCH(QList<unsigned>, expectedTokenKindList);

    SimpleLexer lexer;
    const QList<Token> tokenList = lexer(source);

    int i = 0;
    for (; i < tokenList.size(); ++i) {
        QVERIFY2(i < expectedTokenKindList.size(), "More tokens than expected.");

        // Compare spelled tokens to have it more readable
        const Token token = tokenList.at(i);
        const unsigned expectedTokenKind = expectedTokenKindList.at(i);
        Token expectedToken; // Create a Token in order to spell the token kind
        expectedToken.f.kind = expectedTokenKind;
//        qDebug("Comparing (i=%d): \"%s\" \"%s\"", i, token.spell(), expectedToken.spell());
        QCOMPARE(token.kind(), expectedTokenKind);
    }
    QVERIFY2(i == expectedTokenKindList.size(), "Less tokens than expected.");
}

void tst_SimpleLexer::doxygen_comments_data()
{
    QTest::addColumn<QByteArray>("source");
    QTest::addColumn<QList<unsigned> >("expectedTokenKindList");

    QByteArray source;
    QList<unsigned> expectedTokenKindList;

    source = "// comment";
    expectedTokenKindList = QList<unsigned>() << T_CPP_COMMENT;
    QTest::newRow(source) << source << expectedTokenKindList;

    source = "//// comment";
    expectedTokenKindList = QList<unsigned>() << T_CPP_DOXY_COMMENT;
    QTest::newRow(source) << source << expectedTokenKindList;

    source = "/// comment";
    expectedTokenKindList = QList<unsigned>() << T_CPP_DOXY_COMMENT;
    QTest::newRow(source) << source << expectedTokenKindList;

    source = "///< comment";
    expectedTokenKindList = QList<unsigned>() << T_CPP_DOXY_COMMENT;
    QTest::newRow(source) << source << expectedTokenKindList;

    source = "//! comment";
    expectedTokenKindList = QList<unsigned>() << T_CPP_DOXY_COMMENT;
    QTest::newRow(source) << source << expectedTokenKindList;

    source = "//!< comment";
    expectedTokenKindList = QList<unsigned>() << T_CPP_DOXY_COMMENT;
    QTest::newRow(source) << source << expectedTokenKindList;

    source = "///\n";
    expectedTokenKindList = QList<unsigned>() << T_CPP_DOXY_COMMENT;
    QTest::newRow(source) << source << expectedTokenKindList;

    source = "///\n"
             "int i;";
    expectedTokenKindList = QList<unsigned>()
        << T_CPP_DOXY_COMMENT
        << T_INT << T_IDENTIFIER << T_SEMICOLON;
    QTest::newRow(source) << source << expectedTokenKindList;

    source = "/* comment */\n";
    expectedTokenKindList = QList<unsigned>() << T_COMMENT;
    QTest::newRow(source) << source << expectedTokenKindList;

    source = "/* comment\n"
             "   comment\n"
             " */\n";
    expectedTokenKindList = QList<unsigned>() << T_COMMENT;
    QTest::newRow(source) << source << expectedTokenKindList;

    source = "/** comment */";
    expectedTokenKindList = QList<unsigned>() << T_DOXY_COMMENT;
    QTest::newRow(source) << source << expectedTokenKindList;

    source = "/** comment */\n";
    expectedTokenKindList = QList<unsigned>() << T_DOXY_COMMENT;
    QTest::newRow(source) << source << expectedTokenKindList;

    source = "/** comment */ int i;\n";
    expectedTokenKindList = QList<unsigned>()
        << T_DOXY_COMMENT << T_INT << T_IDENTIFIER << T_SEMICOLON;
    QTest::newRow(source) << source << expectedTokenKindList;

    source = "/**\n"
            "  * comment\n"
             " */\n";
    expectedTokenKindList = QList<unsigned>() << T_DOXY_COMMENT;
    QTest::newRow(source) << source << expectedTokenKindList;

    source = "/*!\n"
            "  * comment\n"
             " */\n";
    expectedTokenKindList = QList<unsigned>() << T_DOXY_COMMENT;
    QTest::newRow(source) << source << expectedTokenKindList;

    source = "/*!\n"
             "    comment\n"
             "*/\n";
    expectedTokenKindList = QList<unsigned>() << T_DOXY_COMMENT;
    QTest::newRow(source) << source << expectedTokenKindList;

    source = "int i; /*!< first counter */\n"
             "int j; /**< second counter */\n"
             "int k; ///< third counter\n"
             "int l; //!< fourth counter\n"
             "       //!< more details...  ";
    expectedTokenKindList = QList<unsigned>()
        << T_INT << T_IDENTIFIER << T_SEMICOLON << T_DOXY_COMMENT
        << T_INT << T_IDENTIFIER << T_SEMICOLON << T_DOXY_COMMENT
        << T_INT << T_IDENTIFIER << T_SEMICOLON << T_CPP_DOXY_COMMENT
        << T_INT << T_IDENTIFIER << T_SEMICOLON << T_CPP_DOXY_COMMENT << T_CPP_DOXY_COMMENT;
    QTest::newRow(source) << source << expectedTokenKindList;
}

QTEST_APPLESS_MAIN(tst_SimpleLexer)
#include "tst_lexer.moc"
