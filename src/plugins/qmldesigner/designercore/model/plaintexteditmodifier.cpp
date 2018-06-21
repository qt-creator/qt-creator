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

#include "plaintexteditmodifier.h"

#include <utils/changeset.h>
#include <qmljs/qmljsmodelmanagerinterface.h>

#include <QPlainTextEdit>

#include <QDebug>

using namespace Utils;
using namespace QmlDesigner;

PlainTextEditModifier::PlainTextEditModifier(QPlainTextEdit *textEdit):
        m_changeSet(0),
        m_textEdit(textEdit),
        m_changeSignalsEnabled(true),
        m_pendingChangeSignal(false),
        m_ongoingTextChange(false)
{
    Q_ASSERT(textEdit);

    connect(m_textEdit, &QPlainTextEdit::textChanged,
            this, &PlainTextEditModifier::textEditChanged);
}

PlainTextEditModifier::~PlainTextEditModifier()
{
}

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
        m_changeSet = 0;
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
    return m_textEdit->document();
}

QString PlainTextEditModifier::text() const
{
    return m_textEdit->toPlainText();
}

QTextCursor PlainTextEditModifier::textCursor() const
{
    return m_textEdit->textCursor();
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
