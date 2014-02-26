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

#ifdef WITH_TESTS

#include <QString>
#include <QtTest/QtTest>

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/coreconstants.h>

#include "basetexteditor.h"
#include "texteditorplugin.h"

using namespace TextEditor;

enum TransFormationType { Uppercase, Lowercase };

Q_DECLARE_METATYPE(TransFormationType)

void Internal::TextEditorPlugin::testBlockSelectionTransformation_data()
{
    QTest::addColumn<QByteArray>("input");
    QTest::addColumn<QByteArray>("transformedText");
    QTest::addColumn<int>("position");
    QTest::addColumn<int>("anchor");
    QTest::addColumn<TransFormationType>("type");

    QTest::newRow("uppercase")
            << QByteArray("aaa\nbbb\nccc\n")
            << QByteArray("aAa\nbBb\ncCc\n")
            << 10 << 1 << Uppercase;
    QTest::newRow("lowercase")
            << QByteArray("AAA\nBBB\nCCC\n")
            << QByteArray("AaA\nBbB\nCcC\n")
            << 10 << 1 << Lowercase;
    QTest::newRow("uppercase_leading_tabs")
            << QByteArray("\taaa\n\tbbb\n\tccc\n")
            << QByteArray("\taAa\n\tbBb\n\tcCc\n")
            << 13 << 2 << Uppercase;
    QTest::newRow("lowercase_leading_tabs")
            << QByteArray("\tAAA\n\tBBB\n\tCCC\n")
            << QByteArray("\tAaA\n\tBbB\n\tCcC\n")
            << 13 << 2 << Lowercase;
    QTest::newRow("uppercase_mixed_leading_whitespace")
            << QByteArray("\taaa\n    bbbbb\n    ccccc\n")
            << QByteArray("\tAaa\n    bbbbB\n    ccccC\n")
            << 24 << 1 << Uppercase;
    QTest::newRow("lowercase_mixed_leading_whitespace")
            << QByteArray("\tAAA\n    BBBBB\n    CCCCC\n")
            << QByteArray("\taAA\n    BBBBb\n    CCCCc\n")
            << 24 << 1 << Lowercase;
    QTest::newRow("uppercase_mid_tabs1")
            << QByteArray("a\ta\nbbbbbbbbb\nccccccccc\n")
            << QByteArray("a\ta\nbBBBBBbbb\ncCCCCCccc\n")
            << 20 << 1 << Uppercase;
    QTest::newRow("lowercase_mid_tabs2")
            << QByteArray("AA\taa\n\t\t\nccccCCCCCC\n")
            << QByteArray("Aa\taa\n\t\t\ncccccccccC\n")
            << 18 << 1 << Lowercase;
    QTest::newRow("uppercase_over_line_ending")
            << QByteArray("aaaaa\nbbb\nccccc\n")
            << QByteArray("aAAAa\nbBB\ncCCCc\n")
            << 14 << 1 << Uppercase;
    QTest::newRow("lowercase_over_line_ending")
            << QByteArray("AAAAA\nBBB\nCCCCC\n")
            << QByteArray("AaaaA\nBbb\nCcccC\n")
            << 14 << 1 << Lowercase;
}

void Internal::TextEditorPlugin::testBlockSelectionTransformation()
{
    // fetch test data
    QFETCH(QByteArray, input);
    QFETCH(QByteArray, transformedText);
    QFETCH(int, position);
    QFETCH(int, anchor);
    QFETCH(TransFormationType, type);

    // open editor
    Core::IEditor *editor = Core::EditorManager::openEditorWithContents(
                Core::Constants::K_DEFAULT_TEXT_EDITOR_ID, 0, input);
    QVERIFY(editor);
    if (BaseTextEditor *textEditor = qobject_cast<BaseTextEditor*>(editor)) {

        // place cursor and enable block selection
        BaseTextEditorWidget *editorWidget = textEditor->editorWidget();
        QTextCursor cursor = editorWidget->textCursor();
        cursor.setPosition(anchor);
        cursor.setPosition(position, QTextCursor::KeepAnchor);
        editorWidget->setTextCursor(cursor);
        editorWidget->setBlockSelection(true);

        // transform blockselection
        switch (type) {
        case Uppercase:
            editorWidget->uppercaseSelection();
            break;
        case Lowercase:
            editorWidget->lowercaseSelection();
            break;
        }
        QCOMPARE(textEditor->baseTextDocument()->plainText().toLatin1(), transformedText);
    } else {

    }
    Core::EditorManager::closeEditor(editor, false);
}

#endif // ifdef WITH_TESTS
