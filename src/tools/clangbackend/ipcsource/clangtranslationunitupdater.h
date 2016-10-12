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

#include "clangclock.h"

#include "commandlinearguments.h"
#include "unsavedfiles.h"
#include "utf8stringvector.h"

#include <clang-c/Index.h>

#include <QSet>

namespace ClangBackEnd {

class TranslationUnitUpdateInput {
public:
    bool parseNeeded = false;
    bool reparseNeeded = false;

    TimePoint needsToBeReparsedChangeTimePoint;
    Utf8String filePath;
    Utf8StringVector fileArguments;

    UnsavedFiles unsavedFiles;

    Utf8String projectId;
    Utf8StringVector projectArguments;
};

class TranslationUnitUpdateResult {
public:
    bool hasParsed() const
    { return parseTimePoint != TimePoint(); }

    bool hasReparsed() const
    { return reparseTimePoint != TimePoint(); }

public:
    Utf8String translationUnitId;

    bool hasParseOrReparseFailed = false;
    TimePoint parseTimePoint;
    TimePoint reparseTimePoint;
    TimePoint needsToBeReparsedChangeTimePoint;

    QSet<Utf8String> dependedOnFilePaths;
};

class TranslationUnitUpdater {
public:
    enum class UpdateMode {
        AsNeeded,
        ParseIfNeeded,
        ForceReparse,
    };

public:
    TranslationUnitUpdater(const Utf8String translationUnitId,
                           CXIndex &index,
                           CXTranslationUnit &cxTranslationUnit,
                           const TranslationUnitUpdateInput &in);

    TranslationUnitUpdateResult update(UpdateMode mode);

    CommandLineArguments commandLineArguments() const;
    static uint defaultParseOptions();

private:
    void createIndexIfNeeded();
    void createTranslationUnitIfNeeded();
    void removeTranslationUnitIfProjectPartWasChanged();
    void reparseIfNeeded();
    void recreateAndParseIfNeeded();
    void reparse();

    void updateIncludeFilePaths();
    static void includeCallback(CXFile included_file,
                                CXSourceLocation *,
                                unsigned,
                                CXClientData clientData);

    bool parseWasSuccessful() const;
    bool reparseWasSuccessful() const;

private:
    CXIndex &m_cxIndex;
    CXTranslationUnit &m_cxTranslationUnit;

    CXErrorCode m_parseErrorCode = CXError_Success;
    int m_reparseErrorCode = 0;

    const TranslationUnitUpdateInput m_in;
    TranslationUnitUpdateResult m_out;
};

} // namespace ClangBackEnd
