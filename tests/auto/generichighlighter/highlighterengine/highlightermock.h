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

#ifndef HIGHLIGHTERMOCK_H
#define HIGHLIGHTERMOCK_H

#include "highlighter.h"

#include <QList>

namespace TextEditor {
namespace Internal {
class Context;
class HighlightDefinition;
}
}

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

class HighlighterMock : public TextEditor::Internal::Highlighter
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

#endif // HIGHLIGHTERMOCK_H
