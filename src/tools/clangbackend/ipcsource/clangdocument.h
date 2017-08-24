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

#include "clangtranslationunitupdater.h"

#include "clangbackend_global.h"
#include "clangtranslationunit.h"

#include <utf8stringvector.h>

#include <clang-c/Index.h>

#include <QSet>
#include <QtGlobal>

#include <memory>

class Utf8String;

namespace ClangBackEnd {

class TranslationUnit;
class TranslationUnits;
class DocumentData;
class TranslationUnitUpdateResult;
class ProjectPart;
class FileContainer;
class Documents;

class Document
{
public:
    enum class FileExistsCheck {
        Check,
        DoNotCheck
    };

    Document() = default;
    Document(const Utf8String &filePath,
             const ProjectPart &projectPart,
             const Utf8StringVector &fileArguments,
             Documents &documents,
             FileExistsCheck fileExistsCheck = FileExistsCheck::Check);
    ~Document();

    Document(const Document &document);
    Document &operator=(const Document &document);

    Document(Document &&document);
    Document &operator=(Document &&document);

    void reset();

    bool isNull() const;
    bool isIntact() const;
    bool isParsed() const;

    Utf8String filePath() const;
    Utf8StringVector fileArguments() const;
    FileContainer fileContainer() const;

    const ProjectPart &projectPart() const;
    const TimePoint lastProjectPartChangeTimePoint() const;
    bool isProjectPartOutdated() const;

    uint documentRevision() const;
    void setDocumentRevision(uint revision);

    bool isResponsivenessIncreased() const;
    bool isResponsivenessIncreaseNeeded() const;
    void setResponsivenessIncreaseNeeded(bool responsivenessIncreaseNeeded);

    bool isSuspended() const;
    void setIsSuspended(bool isSuspended);

    bool isUsedByCurrentEditor() const;
    void setIsUsedByCurrentEditor(bool isUsedByCurrentEditor);

    bool isVisibleInEditor() const;
    void setIsVisibleInEditor(bool isVisibleInEditor, const TimePoint &timePoint);
    TimePoint visibleTimePoint() const;

    bool isDirty() const;
    TimePoint isDirtyTimeChangePoint() const;
    bool setDirtyIfProjectPartIsOutdated();
    void setDirtyIfDependencyIsMet(const Utf8String &filePath);

    TranslationUnitUpdateInput createUpdateInput() const;
    void incorporateUpdaterResult(const TranslationUnitUpdateResult &result) const;

    TranslationUnit translationUnit(PreferredTranslationUnit preferredTranslationUnit
                                        = PreferredTranslationUnit::RecentlyParsed) const;
    TranslationUnits &translationUnits() const;

public: // for tests
    void parse() const;
    void reparse() const;
    const QSet<Utf8String> dependedFilePaths() const;
    void setDependedFilePaths(const QSet<Utf8String> &filePaths);
    TranslationUnitUpdater createUpdater() const;
    void setHasParseOrReparseFailed(bool hasFailed);

private:
    void setDirty();

    void checkIfNull() const;
    void checkIfFileExists() const;

    bool fileExists() const;
    bool isMainFileAndExistsOrIsOtherFile(const Utf8String &filePath) const;

private:
    mutable std::shared_ptr<DocumentData> d;
};

bool operator==(const Document &first, const Document &second);
std::ostream &operator<<(std::ostream &os, const Document &document);
} // namespace ClangBackEnd
