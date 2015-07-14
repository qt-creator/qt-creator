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

#ifndef CLANGEDITORDOCUMENTPROCESSOR_H
#define CLANGEDITORDOCUMENTPROCESSOR_H

#include "clangeditordocumentparser.h"

#include <cpptools/baseeditordocumentprocessor.h>
#include <cpptools/builtineditordocumentprocessor.h>
#include <cpptools/semantichighlighter.h>

#include <QFutureWatcher>
#include <QPointer>

namespace ClangCodeModel {
namespace Internal {

class ModelManagerSupportClang;

class ClangEditorDocumentProcessor : public CppTools::BaseEditorDocumentProcessor
{
    Q_OBJECT

public:
    ClangEditorDocumentProcessor(ModelManagerSupportClang *modelManagerSupport,
                                 TextEditor::TextDocument *document);
    ~ClangEditorDocumentProcessor();

    // BaseEditorDocumentProcessor interface
    void run() override;
    void semanticRehighlight() override;
    void recalculateSemanticInfoDetached(bool force) override;
    CppTools::SemanticInfo recalculateSemanticInfo() override;
    CppTools::BaseEditorDocumentParser *parser() override;
    CPlusPlus::Snapshot snapshot() override;
    bool isParserRunning() const override;

    CppTools::ProjectPart::Ptr projectPart() const;

public:
    static ClangEditorDocumentProcessor *get(const QString &filePath);

private slots:
    void onParserFinished();
    void onProjectPartsRemoved(const QStringList &projectPartIds);

private:
    void updateProjectPartAndTranslationUnitForCompletion();
    void updateTranslationUnitForCompletion(CppTools::ProjectPart &projectPart);

    QPointer<ModelManagerSupportClang> m_modelManagerSupport;

    ClangEditorDocumentParser m_parser;
    CppTools::ProjectPart::Ptr m_projectPart;
    QFutureWatcher<void> m_parserWatcher;
    unsigned m_parserRevision;

    CppTools::SemanticHighlighter m_semanticHighlighter;
    CppTools::BuiltinEditorDocumentProcessor m_builtinProcessor;
};

} // namespace Internal
} // namespace ClangCodeModel

#endif // CLANGEDITORDOCUMENTPROCESSOR_H
