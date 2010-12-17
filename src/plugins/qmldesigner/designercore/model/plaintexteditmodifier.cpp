/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "plaintexteditmodifier.h"

#include <utils/changeset.h>

#include <QtGui/QPlainTextEdit>
#include <QtGui/QUndoStack>

#include <QtCore/QDebug>

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

    connect(m_textEdit, SIGNAL(textChanged()),
            this, SLOT(textEditChanged()));
}

PlainTextEditModifier::~PlainTextEditModifier()
{
}

void PlainTextEditModifier::save(QIODevice *device)
{
    device->write(m_textEdit->toPlainText().toLatin1());
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

    m_textEdit->textCursor().beginEditBlock();
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

    m_textEdit->textCursor().endEditBlock();
}

void PlainTextEditModifier::textEditChanged()
{
    if (!m_ongoingTextChange && m_changeSignalsEnabled) {
        emit textChanged();
    } else {
        m_pendingChangeSignal = true;
    }
}

void PlainTextEditModifier::runRewriting(ChangeSet *changeSet)
{
    m_ongoingTextChange = true;
    QTextCursor cursor = m_textEdit->textCursor();
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
