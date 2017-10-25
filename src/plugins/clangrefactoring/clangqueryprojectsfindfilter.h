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

#include "searchhandle.h"

#include <cpptools/projectpart.h>

#include <coreplugin/find/ifindfilter.h>

#include <filecontainerv2.h>

#include <utils/smallstringvector.h>
#include <utils/temporaryfile.h>

#include <memory>

namespace ClangBackEnd {
class RefactoringServerInterface;
class RequestSourceRangesAndDiagnosticsForQueryMessage;
}

namespace ClangRefactoring {

class RefactoringClient;
class SearchInterface;

class ClangQueryProjectsFindFilter : public Core::IFindFilter
{
    Q_OBJECT

public:
    ClangQueryProjectsFindFilter(ClangBackEnd::RefactoringServerInterface &server,
                                SearchInterface &searchInterface,
                                RefactoringClient &refactoringClient);

    QString id() const override;
    QString displayName() const override;
    bool isEnabled() const override;
    void requestSourceRangesAndDiagnostics(const QString &queryText, const QString &exampleContent);
    SearchHandle *find(const QString &queryText);
    void findAll(const QString &queryText, Core::FindFlags findFlags = 0) override;
    bool showSearchTermInput() const override;
    Core::FindFlags supportedFindFlags() const override;

    void setProjectParts(const std::vector<CppTools::ProjectPart::Ptr> &m_projectParts);

    bool isAvailable() const;
    void setAvailable(bool isAvailable);

    SearchHandle* searchHandleForTestOnly() const;

    void setUnsavedContent(std::vector<ClangBackEnd::V2::FileContainer> &&m_unsavedContent);

    static Utils::SmallStringVector compilerArguments(CppTools::ProjectPart *projectPart,
                                                      CppTools::ProjectFile::Kind fileKind);

protected:
    virtual QWidget *widget() const;
    virtual QString queryText() const;
    RefactoringClient &refactoringClient();

private:
    ClangBackEnd::RequestSourceRangesForQueryMessage createMessage(
            const QString &queryText) const;

private:
    std::vector<ClangBackEnd::V2::FileContainer> m_unsavedContent;
    std::unique_ptr<SearchHandle> m_searchHandle;
    std::vector<CppTools::ProjectPart::Ptr> m_projectParts;
    Utils::TemporaryFile temporaryFile{"clangQuery-XXXXXX.cpp"};
    ClangBackEnd::RefactoringServerInterface &m_server;
    SearchInterface &m_searchInterface;
    RefactoringClient &m_refactoringClient;
};

} // namespace ClangRefactoring
