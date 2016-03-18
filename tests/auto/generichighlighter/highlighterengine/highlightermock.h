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

#pragma once

#include <texteditor/generichighlighter/highlighter.h>

#include <QList>

struct HighlightSequence
{
    HighlightSequence() {}
    HighlightSequence(int from, int to, const QTextCharFormat &format = QTextCharFormat())
    { add(from, to, format); }
    HighlightSequence(const HighlightSequence &sequence)
    { m_formats = sequence.m_formats; }

    void add(int from, int to, const QTextCharFormat &format = QTextCharFormat())
    {
        for (int i = from; i < to; ++i)
            m_formats.append(format);
    }

    QList<QTextCharFormat> m_formats;
};

class HighlighterMock : public TextEditor::Highlighter
{
public:
    HighlighterMock(QTextDocument *parent = 0);

    void reset();
    void showDebugDetails();
    void considerEmptyLines();

    void startNoTestCalls();
    void endNoTestCalls();

    void setExpectedBlockState(const int state);
    void setExpectedBlockStates(const QList<int> &states);
    void setExpectedHighlightSequence(const HighlightSequence &format);
    void setExpectedHighlightSequences(const QList<HighlightSequence> &formats);

protected:
    virtual void highlightBlock(const QString &text);

private:
    QList<int> m_states;
    int m_statesCounter;
    QList<HighlightSequence> m_formatSequence;
    int m_formatsCounter;
    bool m_debugDetails;
    bool m_noTestCall;
    bool m_considerEmptyLines;
};
