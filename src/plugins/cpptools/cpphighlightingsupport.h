/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef CPPTOOLS_CPPHIGHLIGHTINGSUPPORT_H
#define CPPTOOLS_CPPHIGHLIGHTINGSUPPORT_H

#include "cpptools_global.h"

#include <cplusplus/CppDocument.h>
#include <texteditor/semantichighlighter.h>

#include <QFuture>

namespace TextEditor {
class ITextEditor;
}

namespace CppTools {

class CPPTOOLS_EXPORT CppHighlightingSupport
{
public:
    typedef TextEditor::SemanticHighlighter::Result Use;

public:
    CppHighlightingSupport(TextEditor::ITextEditor *editor);
    virtual ~CppHighlightingSupport() = 0;

    virtual QFuture<Use> highlightingFuture(const CPlusPlus::Document::Ptr &doc,
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

    virtual bool hightlighterHandlesDiagnostics() const = 0;
};

} // namespace CppTools

#endif // CPPTOOLS_CPPHIGHLIGHTINGSUPPORT_H
