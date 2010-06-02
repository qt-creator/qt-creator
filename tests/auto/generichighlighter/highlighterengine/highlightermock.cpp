/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "highlightermock.h"
#include "context.h"
#include "highlightdefinition.h"
#include "formats.h"

#include <QtCore/QDebug>
#include <QtTest/QtTest>

namespace QTest {
template<>
char *toString(const QTextCharFormat &format)
{
    QByteArray ba = Formats::instance().name(format).toLatin1();
    return qstrdup(ba.data());
}
}

using namespace TextEditor;
using namespace Internal;

HighlighterMock::HighlighterMock(const QSharedPointer<Context> &defaultContext,
                                 QTextDocument *parent) :
    Highlighter(defaultContext, parent),
    m_debugDetails(false),
    m_statesCounter(0),
    m_formatsCounter(0),
    m_noTestCall(false),
    m_considerEmptyLines(false)
{}

void HighlighterMock::reset()
{
    m_states.clear();
    m_statesCounter = 0;
    m_formatSequence.clear();
    m_formatsCounter = 0;
    m_debugDetails = false;
    m_noTestCall = false;
    m_considerEmptyLines = false;
}

void HighlighterMock::showDebugDetails()
{ m_debugDetails = true; }

void HighlighterMock::considerEmptyLines()
{ m_considerEmptyLines = true; }

void HighlighterMock::startNoTestCalls()
{ m_noTestCall = true; }

void HighlighterMock::endNoTestCalls()
{ m_noTestCall = false; }

void HighlighterMock::setExpectedBlockState(const int state)
{ m_states << state; }

void HighlighterMock::setExpectedBlockStates(const QList<int> &states)
{ m_states = states; }

void HighlighterMock::setExpectedHighlightSequence(const HighlightSequence &format)
{ m_formatSequence << format; }

void HighlighterMock::setExpectedHighlightSequences(const QList<HighlightSequence> &formats)
{ m_formatSequence = formats; }

void HighlighterMock::highlightBlock(const QString &text)
{
    Highlighter::highlightBlock(text);

    if (text.isEmpty() && !m_considerEmptyLines)
        return;

    if (m_noTestCall)
        return;

    if (m_states.isEmpty() || m_formatSequence.isEmpty())
        QFAIL("No expected data setup.");

    if (m_debugDetails)
        qDebug() << "Highlighting" << text;

    if (m_states.size() <= m_statesCounter)
        QFAIL("Expected state for current block not set.");
    QCOMPARE(currentBlockState(), m_states.at(m_statesCounter++));

    if (m_formatSequence.size() <= m_formatsCounter)
        QFAIL("Expected highlight sequence for current block not set.");
    for (int i = 0; i < text.length(); ++i) {
        if (text.at(i).isSpace())
            continue;
        if (m_debugDetails)
            qDebug() << "at offset" << i;
        QCOMPARE(format(i), m_formatSequence.at(m_formatsCounter).m_formats.at(i));
    }
    ++m_formatsCounter;
}
