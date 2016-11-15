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

#include "searchhandleinterface.h"

#include <cpptools/projectpart.h>

#include <coreplugin/find/ifindfilter.h>

#include <utils/smallstringvector.h>

namespace ClangBackEnd {
class RefactoringServerInterface;
class RequestSourceRangesAndDiagnosticsForQueryMessage;
}

namespace ClangRefactoring {

class RefactoringClient;
class SearchInterface;

class ClangQueryCurrentFileFindFilter : public Core::IFindFilter
{
public:
    ClangQueryCurrentFileFindFilter(ClangBackEnd::RefactoringServerInterface &server,
                                    SearchInterface &searchInterface,
                                    RefactoringClient &refactoringClient);
    QString id() const;
    QString displayName() const;
    bool isEnabled() const;
    void findAll(const QString &queryText, Core::FindFlags findFlags = 0);
    Core::FindFlags supportedFindFlags() const;

    void setCurrentDocumentFilePath(const QString &filePath);
    void setUnsavedDocumentContent(const QString &unsavedContent);
    void setCurrentDocumentRevision(int revision);
    void setProjectPart(const CppTools::ProjectPart::Ptr &projectPart);

    void setUsable(bool isUsable);
    bool isUsable() const;

private:
    ClangBackEnd::RequestSourceRangesAndDiagnosticsForQueryMessage createMessage(
            const QString &queryText) const;

    Utils::SmallStringVector createCommandLine() const;

private:
    QString currentDocumentFilePath;
    QString unsavedDocumentContent;
    std::unique_ptr<SearchHandleInterface> searchHandle;
    CppTools::ProjectPart::Ptr projectPart;
    ClangBackEnd::RefactoringServerInterface &server;
    SearchInterface &searchInterface;
    RefactoringClient &refactoringClient;
};

} // namespace ClangRefactoring
