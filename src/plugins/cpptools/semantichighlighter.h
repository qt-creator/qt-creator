/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef SEMANTICHIGHLIGHTER_H
#define SEMANTICHIGHLIGHTER_H

#include "cpptools_global.h"

#include <QFutureWatcher>
#include <QScopedPointer>
#include <QTextCharFormat>

#include <functional>

namespace TextEditor {
class HighlightingResult;
class TextDocument;
}

namespace CppTools {

class CPPTOOLS_EXPORT SemanticHighlighter : public QObject
{
    Q_OBJECT

public:
    enum Kind {
        Unknown = 0,
        TypeUse,
        LocalUse,
        FieldUse,
        EnumerationUse,
        VirtualMethodUse,
        LabelUse,
        MacroUse,
        FunctionUse,
        PseudoKeywordUse,
        StringUse
    };

    typedef std::function<QFuture<TextEditor::HighlightingResult> ()> HighlightingRunner;

public:
    explicit SemanticHighlighter(TextEditor::TextDocument *baseTextDocument);
    ~SemanticHighlighter();

    void setHighlightingRunner(HighlightingRunner highlightingRunner);
    void updateFormatMapFromFontSettings();

    void run();

private slots:
    void onHighlighterResultAvailable(int from, int to);
    void onHighlighterFinished();

private:
    void connectWatcher();
    void disconnectWatcher();

    unsigned documentRevision() const;

private:
    TextEditor::TextDocument *m_baseTextDocument;

    unsigned m_revision;
    QScopedPointer<QFutureWatcher<TextEditor::HighlightingResult>> m_watcher;
    QHash<int, QTextCharFormat> m_formatMap;

    HighlightingRunner m_highlightingRunner;
};

} // namespace CppTools

#endif // SEMANTICHIGHLIGHTER_H
