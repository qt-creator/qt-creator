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

#include "cppminimizableinfobars.h"
#include "cppparsecontext.h"

#include <cpptools/baseeditordocumentprocessor.h>
#include <cpptools/cppcompletionassistprovider.h>
#include <cpptools/cppmodelmanager.h>
#include <cpptools/cppsemanticinfo.h>
#include <cpptools/editordocumenthandle.h>

#include <texteditor/textdocument.h>

#include <QMutex>
#include <QTimer>

namespace CppEditor {
namespace Internal {

class CppEditorDocument : public TextEditor::TextDocument
{
    Q_OBJECT

    friend class CppEditorDocumentHandleImpl;

public:
    explicit CppEditorDocument();

    bool isObjCEnabled() const;
    TextEditor::CompletionAssistProvider *completionAssistProvider() const override;
    TextEditor::QuickFixAssistProvider *quickFixAssistProvider() const override;

    void recalculateSemanticInfoDetached();
    CppTools::SemanticInfo recalculateSemanticInfo(); // TODO: Remove me

    void setPreferredParseContext(const QString &parseContextId);
    void setExtraPreprocessorDirectives(const QByteArray &directives);

    void scheduleProcessDocument();

    const MinimizableInfoBars &minimizableInfoBars() const;
    ParseContextModel &parseContextModel();

    QFuture<CppTools::CursorInfo> cursorInfo(const CppTools::CursorInfoParams &params);

signals:
    void codeWarningsUpdated(unsigned contentsRevision,
                             const QList<QTextEdit::ExtraSelection> selections,
                             const TextEditor::RefactorMarkers &refactorMarkers);

    void ifdefedOutBlocksUpdated(unsigned contentsRevision,
                                 const QList<TextEditor::BlockRange> ifdefedOutBlocks);

    void cppDocumentUpdated(const CPlusPlus::Document::Ptr document);    // TODO: Remove me
    void semanticInfoUpdated(const CppTools::SemanticInfo semanticInfo); // TODO: Remove me

    void preprocessorSettingsChanged(bool customSettings);

protected:
    void applyFontSettings() override;

private:
    void invalidateFormatterCache();
    void onFilePathChanged(const Utils::FileName &oldPath, const Utils::FileName &newPath);
    void onMimeTypeChanged();

    void onAboutToReload();
    void onReloadFinished();

    void reparseWithPreferredParseContext(const QString &id);

    void processDocument();

    QByteArray contentsText() const;
    unsigned contentsRevision() const;

    CppTools::BaseEditorDocumentProcessor *processor();
    void resetProcessor();
    void applyPreferredParseContextFromSettings();
    void applyExtraPreprocessorDirectivesFromSettings();
    void releaseResources();

    void showHideInfoBarAboutMultipleParseContexts(bool show);

    void initializeTimer();

private:
    bool m_fileIsBeingReloaded;
    bool m_isObjCEnabled;

    // Caching contents
    mutable QMutex m_cachedContentsLock;
    mutable QByteArray m_cachedContents;
    mutable int m_cachedContentsRevision;

    unsigned m_processorRevision;
    QTimer m_processorTimer;
    QScopedPointer<CppTools::BaseEditorDocumentProcessor> m_processor;

    CppTools::CppCompletionAssistProvider *m_completionAssistProvider;

    // (Un)Registration in CppModelManager
    QScopedPointer<CppTools::CppEditorDocumentHandle> m_editorDocumentHandle;

    MinimizableInfoBars m_minimizableInfoBars;
    ParseContextModel m_parseContextModel;
};

} // namespace Internal
} // namespace CppEditor
