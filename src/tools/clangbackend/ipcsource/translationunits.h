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
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CLANGBACKEND_TRANSLATIONUNITS_H
#define CLANGBACKEND_TRANSLATIONUNITS_H

#include "clangfilesystemwatcher.h"
#include "translationunit.h"

#include <filecontainer.h>

#include <QVector>

#include <functional>
#include <vector>

namespace ClangBackEnd {

class ProjectParts;
class UnsavedFiles;
class DiagnosticsChangedMessage;

enum class DiagnosticSendState
{
    NoDiagnosticSend,
    MaybeThereAreMoreDiagnostics,
};

class TranslationUnits
{
public:
    TranslationUnits(ProjectParts &projectParts, UnsavedFiles &unsavedFiles);

    void create(const QVector<FileContainer> &fileContainers);
    void update(const QVector<FileContainer> &fileContainers);
    void remove(const QVector<FileContainer> &fileContainers);

    void setCurrentEditor(const Utf8String &filePath);
    void setVisibleEditors(const Utf8StringVector &filePaths);

    const TranslationUnit &translationUnit(const Utf8String &filePath, const Utf8String &projectPartId) const;
    const TranslationUnit &translationUnit(const FileContainer &fileContainer) const;
    bool hasTranslationUnit(const Utf8String &filePath) const;

    const std::vector<TranslationUnit> &translationUnits() const;

    UnsavedFiles &unsavedFiles() const;

    void addWatchedFiles(QSet<Utf8String> &filePaths);

    void updateTranslationUnitsWithChangedDependency(const Utf8String &filePath);
    void updateTranslationUnitsWithChangedDependencies(const QVector<FileContainer> &fileContainers);

    DiagnosticSendState sendChangedDiagnostics();
    DiagnosticSendState sendChangedDiagnosticsForCurrentEditor();
    DiagnosticSendState sendChangedDiagnosticsForVisibleEditors();
    DiagnosticSendState sendChangedDiagnosticsForAll();

    void setSendChangeDiagnosticsCallback(std::function<void(const DiagnosticsChangedMessage&)> &&callback);

    QVector<FileContainer> newerFileContainers(const QVector<FileContainer> &fileContainers) const;

    const ClangFileSystemWatcher *clangFileSystemWatcher() const;

private:
    void createTranslationUnit(const FileContainer &fileContainer);
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

    void sendDiagnosticChangedMessage(const TranslationUnit &translationUnit);
    void removeTranslationUnits(const QVector<FileContainer> &fileContainers);

    template<class Predicate>
    DiagnosticSendState sendChangedDiagnostics(Predicate predicate);

private:
    ClangFileSystemWatcher fileSystemWatcher;
    std::function<void(const DiagnosticsChangedMessage&)> sendDiagnosticsChangedCallback;
    std::vector<TranslationUnit> translationUnits_;
    ProjectParts &projectParts;
    UnsavedFiles &unsavedFiles_;
};

} // namespace ClangBackEnd

#endif // CLANGBACKEND_TRANSLATIONUNITS_H
