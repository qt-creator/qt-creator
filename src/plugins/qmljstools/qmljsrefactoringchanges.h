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

#include "qmljstools_global.h"

#include <qmljs/qmljsdocument.h>

#include <texteditor/refactoringchanges.h>

namespace QmlJS { class ModelManagerInterface; }

namespace QmlJSTools {

class QmlJSRefactoringChanges;
class QmlJSRefactoringFile;
class QmlJSRefactoringChangesData;
typedef QSharedPointer<QmlJSRefactoringFile> QmlJSRefactoringFilePtr;

class QMLJSTOOLS_EXPORT QmlJSRefactoringFile: public TextEditor::RefactoringFile
{
public:
    QmlJS::Document::Ptr qmljsDocument() const;

    /*!
        \returns the offset in the document for the start position of the given
                 source location.
     */
    unsigned startOf(const QmlJS::AST::SourceLocation &loc) const;

    bool isCursorOn(QmlJS::AST::UiObjectMember *ast) const;
    bool isCursorOn(QmlJS::AST::UiQualifiedId *ast) const;
    bool isCursorOn(QmlJS::AST::SourceLocation loc) const;

protected:
    QmlJSRefactoringFile(const QString &fileName, const QSharedPointer<TextEditor::RefactoringChangesData> &data);
    QmlJSRefactoringFile(TextEditor::TextEditorWidget *editor, QmlJS::Document::Ptr document);

    QmlJSRefactoringChangesData *data() const;
    virtual void fileChanged();

    mutable QmlJS::Document::Ptr m_qmljsDocument;

    friend class QmlJSRefactoringChanges;
};


class QMLJSTOOLS_EXPORT QmlJSRefactoringChanges: public TextEditor::RefactoringChanges
{
public:
    QmlJSRefactoringChanges(QmlJS::ModelManagerInterface *modelManager,
                            const QmlJS::Snapshot &snapshot);

    static QmlJSRefactoringFilePtr file(TextEditor::TextEditorWidget *editor,
                                        const QmlJS::Document::Ptr &document);
    QmlJSRefactoringFilePtr file(const QString &fileName) const;

    const QmlJS::Snapshot &snapshot() const;

private:
    QmlJSRefactoringChangesData *data() const;
};

} // namespace QmlJSTools
