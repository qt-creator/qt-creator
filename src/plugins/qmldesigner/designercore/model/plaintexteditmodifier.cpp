// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "plaintexteditmodifier.h"

#include <qmljs/qmljsmodelmanagerinterface.h>

#include <qmljstools/qmljscodestylepreferences.h>
#include <qmljstools/qmljsindenter.h>
#include <qmljstools/qmljstoolssettings.h>

#include <utils/changeset.h>

#include <QDebug>
#include <QPlainTextEdit>

using namespace Utils;
using namespace QmlDesigner;

PlainTextEditModifier::PlainTextEditModifier(QPlainTextEdit *textEdit)
    : PlainTextEditModifier(textEdit->document(), textEdit->textCursor())
{
    connect(textEdit, &QPlainTextEdit::textChanged, this, &PlainTextEditModifier::textEditChanged);
}

PlainTextEditModifier::PlainTextEditModifier(QTextDocument *document, const QTextCursor &textCursor)
    : m_textDocument{document}
    , m_textCursor{textCursor}
{}

PlainTextEditModifier::~PlainTextEditModifier() = default;

void PlainTextEditModifier::replace(int offset, int length, const QString &replacement)
{
#if 0
    qDebug() << "Original:"    << m_textEdit->toPlainText();
    qDebug() << "Replacement:" << replacement;
    qDebug() << "     offset:" << offset;
    qDebug() << "     length:" << length;
#endif

    Q_ASSERT(offset >= 0);
    Q_ASSERT(length >= 0);

    const int replacementLength = replacement.length();

    if (m_changeSet) {
        m_changeSet->replace(offset, offset + length, replacement);
        emit replaced(offset, length, replacementLength);
    } else {
        ChangeSet changeSet;
        changeSet.replace(offset, offset + length, replacement);
        emit replaced(offset, length, replacementLength);
        runRewriting(&changeSet);
    }
}

void PlainTextEditModifier::move(const MoveInfo &moveInfo)
{
#if 0
    qDebug() << "Original:"    << m_textEdit->toPlainText();
    qDebug() << "Move:" << m_textEdit->toPlainText().mid(moveInfo.objectStart, moveInfo.objectEnd - moveInfo.objectStart);
    qDebug() << "     prefix:" << moveInfo.prefixToInsert;
    qDebug() << "     suffix:" <<  moveInfo.suffixToInsert;
    qDebug() << "     leadingCharsToRemove:" <<  moveInfo.leadingCharsToRemove;
    qDebug() << "                          " <<  m_textEdit->toPlainText().mid(moveInfo.objectStart - moveInfo.leadingCharsToRemove,  moveInfo.leadingCharsToRemove);
    qDebug() << "     trailingCharsToRemove:" <<  moveInfo.trailingCharsToRemove;
    qDebug() << "                           " <<  m_textEdit->toPlainText().mid(moveInfo.objectEnd, moveInfo.trailingCharsToRemove);
#endif

    Q_ASSERT(moveInfo.objectStart >= 0);
    Q_ASSERT(moveInfo.objectEnd > moveInfo.objectStart);
    Q_ASSERT(moveInfo.destination >= 0);
    Q_ASSERT(moveInfo.leadingCharsToRemove >= 0);
    Q_ASSERT(moveInfo.trailingCharsToRemove >= 0);
    Q_ASSERT(moveInfo.objectStart - moveInfo.leadingCharsToRemove >= 0);

    if (m_changeSet) {
        m_changeSet->insert(moveInfo.destination, moveInfo.prefixToInsert);
        m_changeSet->move(moveInfo.objectStart, moveInfo.objectEnd, moveInfo.destination);
        m_changeSet->insert(moveInfo.destination, moveInfo.suffixToInsert);
        m_changeSet->remove(moveInfo.objectStart - moveInfo.leadingCharsToRemove, moveInfo.objectStart);
        m_changeSet->remove(moveInfo.objectEnd, moveInfo.objectEnd + moveInfo.trailingCharsToRemove);
        emit moved(moveInfo);
    } else {
        ChangeSet changeSet;
        changeSet.insert(moveInfo.destination, moveInfo.prefixToInsert);
        changeSet.move(moveInfo.objectStart, moveInfo.objectEnd, moveInfo.destination);
        changeSet.insert(moveInfo.destination, moveInfo.suffixToInsert);
        changeSet.remove(moveInfo.objectStart - moveInfo.leadingCharsToRemove, moveInfo.objectStart);
        changeSet.remove(moveInfo.objectEnd, moveInfo.objectEnd + moveInfo.trailingCharsToRemove);
        emit moved(moveInfo);
        runRewriting(&changeSet);
    }
}

void PlainTextEditModifier::startGroup()
{
    if (!m_changeSet)
        m_changeSet = new ChangeSet;

    textCursor().beginEditBlock();
}

void PlainTextEditModifier::flushGroup()
{
    if (m_changeSet)
        runRewriting(m_changeSet);
}

void PlainTextEditModifier::commitGroup()
{
    if (m_changeSet) {
        runRewriting(m_changeSet);
        delete m_changeSet;
        m_changeSet = nullptr;
    }

    textCursor().endEditBlock();
}

void PlainTextEditModifier::textEditChanged()
{
    if (!m_ongoingTextChange && m_changeSignalsEnabled)
        emit textChanged();
    else
        m_pendingChangeSignal = true;
}

void PlainTextEditModifier::runRewriting(ChangeSet *changeSet)
{
    m_ongoingTextChange = true;
    QTextCursor cursor = textCursor();
    changeSet->apply(&cursor);
    m_ongoingTextChange = false;
    textEditChanged();
}

QTextDocument *PlainTextEditModifier::textDocument() const
{
    return m_textDocument;
}

QString PlainTextEditModifier::text() const
{
    return m_textDocument->toPlainText();
}

QTextCursor PlainTextEditModifier::textCursor() const
{
    return m_textCursor;
}

void PlainTextEditModifier::deactivateChangeSignals()
{
    m_changeSignalsEnabled = false;
}

void PlainTextEditModifier::reactivateChangeSignals()
{
    m_changeSignalsEnabled = true;

    if (m_pendingChangeSignal) {
        m_pendingChangeSignal = false;
        emit textChanged();
    }
}

IndentingTextEditModifier::IndentingTextEditModifier(QTextDocument *document, const QTextCursor &textCursor)
    : NotIndentingTextEditModifier{document, textCursor}
{
    m_tabSettings = QmlJSTools::QmlJSToolsSettings::globalCodeStyle()->tabSettings();
}

void IndentingTextEditModifier::indent(int offset, int length)
{
    if (length == 0 || offset < 0 || offset + length >= text().length())
        return;

    int startLine = getLineInDocument(textDocument(), offset);
    int endLine = getLineInDocument(textDocument(), offset + length);

    if (startLine > -1 && endLine > -1)
        indentLines(startLine, endLine);
}

void IndentingTextEditModifier::indentLines(int startLine, int endLine)
{
    if (startLine < 0)
        return;

    QTextCursor tc(textDocument());

    tc.beginEditBlock();
    for (int i = startLine; i <= endLine; i++) {
        QTextBlock start = textDocument()->findBlockByNumber(i);

        if (start.isValid()) {
            QmlJSEditor::Internal::Indenter indenter(textDocument());
            indenter.indentBlock(start, QChar::Null, m_tabSettings);
        }
    }
    tc.endEditBlock();
}

