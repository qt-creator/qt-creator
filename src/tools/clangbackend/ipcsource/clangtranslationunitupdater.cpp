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

#include "clangtranslationunitupdater.h"

#include "clangfilepath.h"
#include "clangstring.h"
#include "clangunsavedfilesshallowarguments.h"

#include <QLoggingCategory>

static Q_LOGGING_CATEGORY(verboseLibLog, "qtc.clangbackend.verboselib");

static bool isVerboseModeEnabled()
{
    return verboseLibLog().isDebugEnabled();
}

namespace ClangBackEnd {

TranslationUnitUpdater::TranslationUnitUpdater(const Utf8String translationUnitId,
                                               CXIndex &index,
                                               CXTranslationUnit &cxTranslationUnit,
                                               const TranslationUnitUpdateInput &updateData)
    : m_cxIndex(index)
    , m_cxTranslationUnit(cxTranslationUnit)
    , m_in(updateData)
{
    m_out.translationUnitId = translationUnitId;
}

TranslationUnitUpdateResult TranslationUnitUpdater::update(UpdateMode mode)
{
    createIndexIfNeeded();

    switch (mode) {
    case UpdateMode::AsNeeded:
        recreateAndParseIfNeeded();
        reparseIfNeeded();
        break;
    case UpdateMode::ParseIfNeeded:
        recreateAndParseIfNeeded();
        break;
    case UpdateMode::ForceReparse:
        reparse();
        break;
    }

    return m_out;
}

void TranslationUnitUpdater::recreateAndParseIfNeeded()
{
    removeTranslationUnitIfProjectPartWasChanged();
    createTranslationUnitIfNeeded();
}

void TranslationUnitUpdater::removeTranslationUnitIfProjectPartWasChanged()
{
    if (m_in.parseNeeded) {
        clang_disposeTranslationUnit(m_cxTranslationUnit);
        m_cxTranslationUnit = nullptr;
    }
}

#define RETURN_TEXT_FOR_CASE(enumValue) case enumValue: return #enumValue
static const char *errorCodeToText(CXErrorCode errorCode)
{
    switch (errorCode) {
        RETURN_TEXT_FOR_CASE(CXError_Success);
        RETURN_TEXT_FOR_CASE(CXError_Failure);
        RETURN_TEXT_FOR_CASE(CXError_Crashed);
        RETURN_TEXT_FOR_CASE(CXError_InvalidArguments);
        RETURN_TEXT_FOR_CASE(CXError_ASTReadError);
    }

    return "UnknownCXErrorCode";
}
#undef RETURN_TEXT_FOR_CASE

void TranslationUnitUpdater::createTranslationUnitIfNeeded()
{
    if (!m_cxTranslationUnit) {
        m_cxTranslationUnit = CXTranslationUnit();

        const auto args = commandLineArguments();
        if (isVerboseModeEnabled())
            args.print();

        UnsavedFilesShallowArguments unsaved = m_in.unsavedFiles.shallowArguments();

        m_parseErrorCode = clang_parseTranslationUnit2(m_cxIndex,
                                                     NULL,
                                                     args.data(),
                                                     args.count(),
                                                     unsaved.data(),
                                                     unsaved.count(),
                                                     defaultParseOptions(),
                                                     &m_cxTranslationUnit);



        if (parseWasSuccessful()) {
            updateIncludeFilePaths();
            m_out.parseTimePoint = Clock::now();
        } else {
            qWarning() << "Parsing" << m_in.filePath << "failed:"
                       << errorCodeToText(m_parseErrorCode);
            m_out.hasParseOrReparseFailed = true;
        }
    }
}

void TranslationUnitUpdater::reparseIfNeeded()
{
    if (m_in.reparseNeeded)
        reparse();
}

void TranslationUnitUpdater::reparse()
{
    UnsavedFilesShallowArguments unsaved = m_in.unsavedFiles.shallowArguments();

    m_reparseErrorCode = clang_reparseTranslationUnit(
                            m_cxTranslationUnit,
                            unsaved.count(),
                            unsaved.data(),
                            clang_defaultReparseOptions(m_cxTranslationUnit));


    if (reparseWasSuccessful()) {
        updateIncludeFilePaths();

        m_out.reparseTimePoint = Clock::now();
        m_out.needsToBeReparsedChangeTimePoint = m_in.needsToBeReparsedChangeTimePoint;
    } else {
        qWarning() << "Reparsing" << m_in.filePath << "failed:" << m_reparseErrorCode;
        m_out.hasParseOrReparseFailed = true;
    }
}

void TranslationUnitUpdater::updateIncludeFilePaths()
{
    m_out.dependedOnFilePaths.clear();
    m_out.dependedOnFilePaths.insert(m_in.filePath);

    clang_getInclusions(m_cxTranslationUnit,
                        includeCallback,
                        const_cast<TranslationUnitUpdater *>(this));
}

uint TranslationUnitUpdater::defaultParseOptions()
{
    return CXTranslationUnit_CacheCompletionResults
         | CXTranslationUnit_PrecompiledPreamble
         | CXTranslationUnit_IncludeBriefCommentsInCodeCompletion
         | CXTranslationUnit_DetailedPreprocessingRecord;
}

void TranslationUnitUpdater::createIndexIfNeeded()
{
    if (!m_cxIndex) {
        const bool displayDiagnostics = isVerboseModeEnabled();
        m_cxIndex = clang_createIndex(1, displayDiagnostics);
    }
}

void TranslationUnitUpdater::includeCallback(CXFile included_file,
                                             CXSourceLocation *,
                                             unsigned, CXClientData clientData)
{
    ClangString includeFilePath(clang_getFileName(included_file));

    TranslationUnitUpdater *updater = static_cast<TranslationUnitUpdater *>(clientData);

    updater->m_out.dependedOnFilePaths.insert(FilePath::fromNativeSeparators(includeFilePath));
}

bool TranslationUnitUpdater::parseWasSuccessful() const
{
    return m_parseErrorCode == CXError_Success;
}

bool TranslationUnitUpdater::reparseWasSuccessful() const
{
    return m_reparseErrorCode == 0;
}

CommandLineArguments TranslationUnitUpdater::commandLineArguments() const
{
    return CommandLineArguments(m_in.filePath.constData(),
                                m_in.projectArguments,
                                m_in.fileArguments,
                                isVerboseModeEnabled());
}

} // namespace ClangBackEnd
