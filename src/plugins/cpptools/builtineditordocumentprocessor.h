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

#ifndef BUILTINEDITORDOCUMENTPROCESSOR_H
#define BUILTINEDITORDOCUMENTPROCESSOR_H

#include "baseeditordocumentprocessor.h"
#include "builtineditordocumentparser.h"
#include "cppsemanticinfoupdater.h"
#include "cpptools_global.h"
#include "semantichighlighter.h"

#include <utils/qtcoverride.h>

namespace CppTools {

class CPPTOOLS_EXPORT BuiltinEditorDocumentProcessor : public BaseEditorDocumentProcessor
{
    Q_OBJECT
    BuiltinEditorDocumentProcessor();

public:
    BuiltinEditorDocumentProcessor(TextEditor::BaseTextDocument *document,
                                   bool enableSemanticHighlighter = true);
    ~BuiltinEditorDocumentProcessor();

    // BaseEditorDocumentProcessor interface
    void run() QTC_OVERRIDE;
    void semanticRehighlight(bool force) QTC_OVERRIDE;
    CppTools::SemanticInfo recalculateSemanticInfo() QTC_OVERRIDE;
    BaseEditorDocumentParser *parser() QTC_OVERRIDE;
    bool isParserRunning() const QTC_OVERRIDE;

public:
    static BuiltinEditorDocumentProcessor *get(const QString &filePath);

private:
    void onDocumentUpdated(CPlusPlus::Document::Ptr document);
    void onSemanticInfoUpdated(const CppTools::SemanticInfo semanticInfo);

    SemanticInfo::Source createSemanticInfoSource(bool force) const;

private:
    QScopedPointer<BuiltinEditorDocumentParser> m_parser;
    QFuture<void> m_parserFuture;

    SemanticInfoUpdater m_semanticInfoUpdater;
    QScopedPointer<SemanticHighlighter> m_semanticHighlighter;
};

} // namespace CppTools

#endif // BUILTINEDITORDOCUMENTPROCESSOR_H
