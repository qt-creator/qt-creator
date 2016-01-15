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

#include "formats.h"
#include "highlightermock.h"

#include <texteditor/generichighlighter/context.h>
#include <texteditor/generichighlighter/highlightdefinition.h>

#include <QDebug>
#include <QtTest>

QT_BEGIN_NAMESPACE
namespace QTest {
    template<> inline char *toString(const QTextCharFormat &format)
    {
        QByteArray ba = Formats::instance().name(format).toLatin1();
        return qstrdup(ba.data());
    }
}
QT_END_NAMESPACE

using namespace TextEditor;
using namespace Internal;

HighlighterMock::HighlighterMock(QTextDocument *parent) :
    Highlighter(parent),
    m_statesCounter(0),
    m_formatsCounter(0),
    m_debugDetails(false),
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
    const int observableState = currentBlockState() & 0xFFF;
    QCOMPARE(observableState, m_states.at(m_statesCounter++));

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
