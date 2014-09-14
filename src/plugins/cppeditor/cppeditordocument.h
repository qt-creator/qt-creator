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

#ifndef CPPEDITORDOCUMENT_H
#define CPPEDITORDOCUMENT_H

#include <cpptools/baseeditordocumentprocessor.h>
#include <cpptools/cppcompletionassistprovider.h>
#include <cpptools/cppmodelmanager.h>
#include <cpptools/cppsemanticinfo.h>
#include <cpptools/editordocumenthandle.h>

#include <texteditor/basetextdocument.h>

#include <QMutex>
#include <QTimer>

namespace CppEditor {
namespace Internal {

class CppEditorDocument : public TextEditor::BaseTextDocument
{
    Q_OBJECT

    friend class CppEditorDocumentHandle;

public:
    explicit CppEditorDocument();
    ~CppEditorDocument();

    bool isObjCEnabled() const;
    CppTools::CppCompletionAssistProvider *completionAssistProvider() const;

    void semanticRehighlight();
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
    void applyFontSettings();

private slots:
    void invalidateFormatterCache();
    void onFilePathChanged(const QString &oldPath, const QString &newPath);
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
    QScopedPointer<CppTools::EditorDocumentHandle> m_editorDocumentHandle;
};

} // namespace Internal
} // namespace CppEditor

#endif // CPPEDITORDOCUMENT_H
