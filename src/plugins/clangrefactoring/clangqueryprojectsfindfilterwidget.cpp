/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "clangqueryprojectsfindfilterwidget.h"

#include "clangqueryhighlighter.h"

#include <texteditor/textdocument.h>

#include <QTextDocument>

namespace ClangRefactoring {

ClangQueryProjectsFindFilterWidget::ClangQueryProjectsFindFilterWidget()
{
    m_form.setupUi(this);
}

QPlainTextEdit *ClangQueryProjectsFindFilterWidget::queryTextEdit() const
{
    return m_form.queryTextEdit;
}

QPlainTextEdit *ClangQueryProjectsFindFilterWidget::queryExampleTextEdit() const
{
    return m_form.exampleSourceTextEdit;
}

ClangQueryExampleHighlighter *ClangQueryProjectsFindFilterWidget::clangQueryExampleHighlighter() const
{
    return m_form.exampleSourceTextEdit->syntaxHighlighter();
}

ClangQueryHighlighter *ClangQueryProjectsFindFilterWidget::clangQueryHighlighter() const
{
    return m_form.queryTextEdit->syntaxHighlighter();
}

bool ClangQueryProjectsFindFilterWidget::isValid() const
{
    return !m_form.queryTextEdit->textDocument()->document()->isEmpty()
        && !clangQueryHighlighter()->hasDiagnostics();

}

} // namespace ClangRefactoring
