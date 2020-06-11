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

#include "gtest-creator-printing.h"
#include "gtest-std-printing.h"

#ifdef CLANG_UNIT_TESTS
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>

#include <clangdocumentsuspenderresumer.h>
#include <clangreferencescollector.h>
#include <filepathview.h>
#include <fulltokeninfo.h>
#include <tokenprocessor.h>

#endif

#include <gtest/gtest-printers.h>

namespace TestGlobal {

const clang::SourceManager *globalSourceManager = nullptr;

void setSourceManager(const clang::SourceManager *sourceManager)
{
    globalSourceManager = sourceManager;
}

}

namespace llvm {
std::ostream &operator<<(std::ostream &out, const StringRef stringReference)
{
    out.write(stringReference.data(), std::streamsize(stringReference.size()));

    return out;
}
}

namespace clang {

void PrintTo(const FullSourceLoc &sourceLocation, ::std::ostream *os)
{
    auto &&sourceManager = sourceLocation.getManager();
    auto fileName = sourceManager.getFileEntryForID(sourceLocation.getFileID())->getName();

    *os << "(\""
        << fileName << ", "
        << sourceLocation.getSpellingLineNumber() << ", "
        << sourceLocation.getSpellingColumnNumber() << ")";
}

void PrintTo(const SourceRange &sourceRange, ::std::ostream *os)
{
    if (TestGlobal::globalSourceManager) {
        *os << "("
            << sourceRange.getBegin().printToString(*TestGlobal::globalSourceManager) << ", "
            << sourceRange.getEnd().printToString(*TestGlobal::globalSourceManager) << ")";
    } else {
        *os << "("
            << sourceRange.getBegin().getRawEncoding() << ", "
            << sourceRange.getEnd().getRawEncoding() << ")";
    }
}

}

namespace ClangBackEnd {
std::ostream &operator<<(std::ostream &os, const TokenInfo &tokenInfo)
{
    os << "(type: " << tokenInfo.types() << ", "
       << " line: " << tokenInfo.line() << ", "
       << " column: " << tokenInfo.column() << ", "
       << " length: " << tokenInfo.length() << ")";

    return os;
}

template<class T>
std::ostream &operator<<(std::ostream &out, const TokenProcessor<T> &tokenInfos)
{
    out << "[";

    for (const T &entry : tokenInfos)
        out << entry;

    out << "]";

    return out;
}

template std::ostream &operator<<(std::ostream &out, const TokenProcessor<TokenInfo> &tokenInfos);
template std::ostream &operator<<(std::ostream &out, const TokenProcessor<FullTokenInfo> &tokenInfos);

std::ostream &operator<<(std::ostream &out, const SuspendResumeJobsEntry &entry)
{
    return out << "(" << entry.document.filePath() << ", " << entry.jobRequestType << ", "
               << entry.preferredTranslationUnit << ")";
}

std::ostream &operator<<(std::ostream &os, const ReferencesResult &value)
{
    os << "ReferencesResult(";
    os << value.isLocalVariable << ", {";
    for (const SourceRangeContainer &r : value.references) {
        os << r.start.line << ",";
        os << r.start.column << ",";
        os << r.end.column - r.start.column << ",";
    }
    os << "})";

    return os;
}

} // namespace ClangBackEnd
