/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QMLREFACTORINGCHANGES_H
#define QMLREFACTORINGCHANGES_H

#include "qmljstools_global.h"

#include <qmljs/qmljsdocument.h>

#include <texteditor/refactoringchanges.h>

namespace QmlJS {
class ModelManagerInterface;
} // namespace QmlJS

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
    QmlJSRefactoringFile(TextEditor::BaseTextEditorWidget *editor, QmlJS::Document::Ptr document);

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

    static QmlJSRefactoringFilePtr file(TextEditor::BaseTextEditorWidget *editor,
                                        const QmlJS::Document::Ptr &document);
    QmlJSRefactoringFilePtr file(const QString &fileName) const;

    const QmlJS::Snapshot &snapshot() const;

private:
    QmlJSRefactoringChangesData *data() const;
};

} // namespace QmlJSTools

#endif // QMLREFACTORINGCHANGES_H
