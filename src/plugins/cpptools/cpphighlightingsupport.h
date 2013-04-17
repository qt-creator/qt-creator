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

#ifndef CPPTOOLS_CPPHIGHLIGHTINGSUPPORT_H
#define CPPTOOLS_CPPHIGHLIGHTINGSUPPORT_H

#include "cpptools_global.h"

#include <texteditor/semantichighlighter.h>

#include <cplusplus/CppDocument.h>

#include <QFuture>

namespace TextEditor {
class ITextEditor;
}

namespace CppTools {

class CPPTOOLS_EXPORT CppHighlightingSupport
{
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
        PseudoKeywordUse
    };

public:
    CppHighlightingSupport(TextEditor::ITextEditor *editor);
    virtual ~CppHighlightingSupport() = 0;

    virtual bool requiresSemanticInfo() const = 0;

    virtual bool hightlighterHandlesDiagnostics() const = 0;

    virtual QFuture<TextEditor::HighlightingResult> highlightingFuture(
            const CPlusPlus::Document::Ptr &doc,
            const CPlusPlus::Snapshot &snapshot) const = 0;

protected:
    TextEditor::ITextEditor *editor() const
    { return m_editor; }

private:
    TextEditor::ITextEditor *m_editor;
};

class CPPTOOLS_EXPORT CppHighlightingSupportFactory
{
public:
    virtual ~CppHighlightingSupportFactory() = 0;

    virtual CppHighlightingSupport *highlightingSupport(TextEditor::ITextEditor *editor) = 0;
};

} // namespace CppTools

#endif // CPPTOOLS_CPPHIGHLIGHTINGSUPPORT_H
