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

#ifndef CPPEDITORDOCUMENT_H
#define CPPEDITORDOCUMENT_H

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

    void recalculateSemanticInfoDetached();
    CppTools::SemanticInfo recalculateSemanticInfo(); // TODO: Remove me

    void setPreprocessorSettings(const CppTools::ProjectPart::Ptr &projectPart,
                                 const QByteArray &defines);

signals:
    void codeWarningsUpdated(unsigned contentsRevision,
                             const QList<QTextEdit::ExtraSelection> selections);

    void ifdefedOutBlocksUpdated(unsigned contentsRevision,
                                 const QList<TextEditor::BlockRange> ifdefedOutBlocks);

    void cppDocumentUpdated(const CPlusPlus::Document::Ptr document);    // TODO: Remove me
    void semanticInfoUpdated(const CppTools::SemanticInfo semanticInfo); // TODO: Remove me

    void preprocessorSettingsChanged(bool customSettings);

public slots:
    void scheduleProcessDocument();

protected:
    void applyFontSettings() override;

private slots:
    void invalidateFormatterCache();
    void onFilePathChanged(const Utils::FileName &oldPath, const Utils::FileName &newPath);
    void onMimeTypeChanged();

    void onAboutToReload();
    void onReloadFinished();

    void processDocument();

private:
    QByteArray contentsText() const;
    unsigned contentsRevision() const;

    CppTools::BaseEditorDocumentProcessor *processor();
    void resetProcessor();
    void updatePreprocessorSettings();
    void releaseResources();

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
};

} // namespace Internal
} // namespace CppEditor

#endif // CPPEDITORDOCUMENT_H
