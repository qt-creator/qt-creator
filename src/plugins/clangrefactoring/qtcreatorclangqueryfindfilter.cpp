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

#include "qtcreatorclangqueryfindfilter.h"

#include "clangqueryprojectsfindfilterwidget.h"
#include "refactoringclient.h"

#include <cpptools/abstracteditorsupport.h>
#include <cpptools/cppmodelmanager.h>
#include <cpptools/projectinfo.h>

#include <projectexplorer/session.h>

#include <utils/smallstring.h>

namespace ClangRefactoring {

QtCreatorClangQueryFindFilter::QtCreatorClangQueryFindFilter(ClangBackEnd::RefactoringServerInterface &server,
                                                             SearchInterface &searchInterface,
                                                             RefactoringClient &refactoringClient)
    : ClangQueryProjectsFindFilter(server, searchInterface, refactoringClient)
{
}

void QtCreatorClangQueryFindFilter::findAll(const QString &queryText, Core::FindFlags findFlags)
{
    prepareFind();

    ClangQueryProjectsFindFilter::findAll(queryText, findFlags);
}

void QtCreatorClangQueryFindFilter::handleQueryOrExampleTextChanged()
{
    const QString queryText = this->queryText();
    const QString queryExampleText = this->queryExampleText();
    if (!queryText.isEmpty() && !queryExampleText.isEmpty())
        requestSourceRangesAndDiagnostics(queryText, queryExampleText);
}

QWidget *QtCreatorClangQueryFindFilter::createConfigWidget()
{
    m_widget = new ClangQueryProjectsFindFilterWidget;

    refactoringClient().setClangQueryExampleHighlighter(m_widget->clangQueryExampleHighlighter());
    refactoringClient().setClangQueryHighlighter(m_widget->clangQueryHighlighter());

    QObject::connect(m_widget->queryExampleTextEdit(),
                     &QPlainTextEdit::textChanged,
                     this,
                     &QtCreatorClangQueryFindFilter::handleQueryOrExampleTextChanged);

    QObject::connect(m_widget->queryTextEdit(),
                     &QPlainTextEdit::textChanged,
                     this,
                     &QtCreatorClangQueryFindFilter::handleQueryOrExampleTextChanged);

    return m_widget;
}

bool ClangRefactoring::QtCreatorClangQueryFindFilter::isValid() const
{
    return true;
}

QWidget *QtCreatorClangQueryFindFilter::widget() const
{
    return m_widget;
}

QString QtCreatorClangQueryFindFilter::queryText() const
{
    return m_widget->queryTextEdit()->toPlainText();
}

QString QtCreatorClangQueryFindFilter::queryExampleText() const
{
    return m_widget->queryExampleTextEdit()->toPlainText();
}

namespace {

std::vector<ClangBackEnd::V2::FileContainer> createUnsavedContents()
{
    auto abstractEditors = CppTools::CppModelManager::instance()->abstractEditorSupports();
    std::vector<ClangBackEnd::V2::FileContainer> unsavedContents;
    unsavedContents.reserve(std::size_t(abstractEditors.size()));

    auto toFileContainer = [] (const CppTools::AbstractEditorSupport *abstractEditor) {
        return  ClangBackEnd::V2::FileContainer(ClangBackEnd::FilePath(abstractEditor->fileName()),
                                                Utils::SmallString::fromQByteArray(abstractEditor->contents()),
                                                {});
    };

    std::transform(abstractEditors.begin(),
                   abstractEditors.end(),
                   std::back_inserter(unsavedContents),
                   toFileContainer);

    return unsavedContents;
}

}

void QtCreatorClangQueryFindFilter::prepareFind()
{
   ProjectExplorer::Project *currentProject = ProjectExplorer::SessionManager::startupProject();

    const CppTools::ProjectInfo projectInfo = CppTools::CppModelManager::instance()->projectInfo(currentProject);

    setProjectParts(projectInfo.projectParts().toStdVector());

    setUnsavedContent(createUnsavedContents());
}

} // namespace ClangRefactoring
