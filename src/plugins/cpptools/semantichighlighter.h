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

#ifndef SEMANTICHIGHLIGHTER_H
#define SEMANTICHIGHLIGHTER_H

#include "cppsemanticinfo.h"
#include "cpptools_global.h"

#include <texteditor/textdocument.h>
#include <texteditor/semantichighlighter.h>

#include <QFutureWatcher>
#include <QScopedPointer>
#include <QTextEdit>

#include <functional>

namespace CppTools {

class CPPTOOLS_EXPORT SemanticHighlighter : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(SemanticHighlighter)

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

    void run();

private slots:
    void onDocumentFontSettingsChanged();

    void onHighlighterResultAvailable(int from, int to);
    void onHighlighterFinished();

private:
    void connectWatcher();
    void disconnectWatcher();

    unsigned documentRevision() const;
    void updateFormatMapFromFontSettings();

private:
    TextEditor::TextDocument *m_baseTextDocument;

    unsigned m_revision;
    QScopedPointer<QFutureWatcher<TextEditor::HighlightingResult>> m_watcher;
    QHash<int, QTextCharFormat> m_formatMap;

    HighlightingRunner m_highlightingRunner;
};

} // namespace CppTools

#endif // SEMANTICHIGHLIGHTER_H
