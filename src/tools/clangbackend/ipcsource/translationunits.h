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

#ifndef CLANGBACKEND_TRANSLATIONUNITS_H
#define CLANGBACKEND_TRANSLATIONUNITS_H

#include "clangfilesystemwatcher.h"
#include "clangtranslationunit.h"

#include <filecontainer.h>

#include <QVector>

#include <functional>
#include <vector>

namespace ClangBackEnd {

class ProjectParts;
class UnsavedFiles;
class DiagnosticsChangedMessage;
class HighlightingChangedMessage;

enum class DocumentAnnotationsSendState
{
    NoDocumentAnnotationsSent,
    MaybeThereAreDocumentAnnotations,
};

class TranslationUnits
{
public:
    using SendDocumentAnnotationsCallback
        = std::function<void (const DiagnosticsChangedMessage &,
                              const HighlightingChangedMessage &)>;

public:
    TranslationUnits(ProjectParts &projectParts, UnsavedFiles &unsavedFiles);

    std::vector<TranslationUnit> create(const QVector<FileContainer> &fileContainers);
    void update(const QVector<FileContainer> &fileContainers);
    void remove(const QVector<FileContainer> &fileContainers);

    void setUsedByCurrentEditor(const Utf8String &filePath);
    void setVisibleInEditors(const Utf8StringVector &filePaths);

    const TranslationUnit &translationUnit(const Utf8String &filePath, const Utf8String &projectPartId) const;
    const TranslationUnit &translationUnit(const FileContainer &fileContainer) const;
    bool hasTranslationUnit(const Utf8String &filePath) const;

    const std::vector<TranslationUnit> &translationUnits() const;

    UnsavedFiles &unsavedFiles() const;

    void addWatchedFiles(QSet<Utf8String> &filePaths);

    void updateTranslationUnitsWithChangedDependency(const Utf8String &filePath);
    void updateTranslationUnitsWithChangedDependencies(const QVector<FileContainer> &fileContainers);
    void setTranslationUnitsDirtyIfProjectPartChanged();

    DocumentAnnotationsSendState sendDocumentAnnotationsForCurrentEditor();
    DocumentAnnotationsSendState sendDocumentAnnotationsForVisibleEditors();
    DocumentAnnotationsSendState sendDocumentAnnotationsForAll();
    DocumentAnnotationsSendState sendDocumentAnnotations();

    void setSendDocumentAnnotationsCallback(SendDocumentAnnotationsCallback &&callback);

    QVector<FileContainer> newerFileContainers(const QVector<FileContainer> &fileContainers) const;

    const ClangFileSystemWatcher *clangFileSystemWatcher() const;

private:
    TranslationUnit createTranslationUnit(const FileContainer &fileContainer);
    void updateTranslationUnit(const FileContainer &fileContainer);
    std::vector<TranslationUnit>::iterator findTranslationUnit(const FileContainer &fileContainer);
    std::vector<TranslationUnit>::iterator findAllTranslationUnitWithFilePath(const Utf8String &filePath);
    std::vector<TranslationUnit>::const_iterator findTranslationUnit(const Utf8String &filePath, const Utf8String &projectPartId) const;
    bool hasTranslationUnit(const FileContainer &fileContainer) const;
    bool hasTranslationUnitWithFilePath(const Utf8String &filePath) const;
    void checkIfProjectPartExists(const Utf8String &projectFileName) const;
    void checkIfProjectPartsExists(const QVector<FileContainer> &fileContainers) const;
    void checkIfTranslationUnitsDoesNotExists(const QVector<FileContainer> &fileContainers) const;
    void checkIfTranslationUnitsForFilePathsDoesExists(const QVector<FileContainer> &fileContainers) const;

    void removeTranslationUnits(const QVector<FileContainer> &fileContainers);

    template<class Predicate>
    DocumentAnnotationsSendState sendDocumentAnnotations(Predicate predicate);
    void sendDocumentAnnotations(const TranslationUnit &translationUnit);

private:
    ClangFileSystemWatcher fileSystemWatcher;
    SendDocumentAnnotationsCallback sendDocumentAnnotationsCallback;
    std::vector<TranslationUnit> translationUnits_;
    ProjectParts &projectParts;
    UnsavedFiles &unsavedFiles_;
};

} // namespace ClangBackEnd

#endif // CLANGBACKEND_TRANSLATIONUNITS_H
