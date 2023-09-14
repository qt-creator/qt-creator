// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/quickfix.h>
#include <qmljs/parser/qmljsastfwd_p.h>
#include <qmljs/qmljsdocument.h>
#include <qmljstools/qmljsrefactoringchanges.h>
#include <qmljstools/qmljssemanticinfo.h>

#include <QSharedPointer>

namespace QmlJSEditor {

namespace Internal { class QmlJSQuickFixAssistInterface; }

using TextEditor::QuickFixOperation;
using TextEditor::QuickFixOperations;
using TextEditor::QuickFixInterface;

/*!
    A quick-fix operation for the QML/JavaScript editor.
 */
class QmlJSQuickFixOperation: public TextEditor::QuickFixOperation
{
public:
    /*!
        Creates a new QmlJSQuickFixOperation.

        \param interface The interface on which the operation is performed.
        \param priority The priority for this operation.
     */
    explicit QmlJSQuickFixOperation(const Internal::QmlJSQuickFixAssistInterface *interface,
                                    int priority = -1);

    void perform() override;

protected:
    using Range = Utils::ChangeSet::Range;

    virtual void performChanges(QmlJSTools::QmlJSRefactoringFilePtr currentFile,
                                const QmlJSTools::QmlJSRefactoringChanges &refactoring,
                                const QString &imports = QString())
        = 0;

    const QmlJSTools::SemanticInfo &semanticInfo() const;

    /// Returns The name of the file for for which this operation is invoked.
    Utils::FilePath fileName() const;

private:
    const QmlJSTools::SemanticInfo m_semanticInfo;
};

TextEditor::QuickFixOperations findQmlJSQuickFixes(const TextEditor::AssistInterface *interface);

} // namespace QmlJSEditor
