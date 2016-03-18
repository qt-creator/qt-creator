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

#include "qmljseditor.h"

#include <texteditor/quickfix.h>
#include <qmljs/parser/qmljsastfwd_p.h>
#include <qmljs/qmljsdocument.h>
#include <qmljstools/qmljsrefactoringchanges.h>

#include <QSharedPointer>

namespace QmlJS { class ModelManagerInterface; }

namespace QmlJSEditor {

namespace Internal { class QmlJSQuickFixAssistInterface; }

typedef QSharedPointer<const Internal::QmlJSQuickFixAssistInterface> QmlJSQuickFixInterface;
typedef TextEditor::QuickFixOperation QuickFixOperation;
typedef TextEditor::QuickFixOperations QuickFixOperations;
typedef TextEditor::QuickFixInterface QuickFixInterface;

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
    explicit QmlJSQuickFixOperation(const QmlJSQuickFixInterface &interface, int priority = -1);

    virtual void perform();

protected:
    typedef Utils::ChangeSet::Range Range;

    virtual void performChanges(QmlJSTools::QmlJSRefactoringFilePtr currentFile,
                                const QmlJSTools::QmlJSRefactoringChanges &refactoring) = 0;

    const Internal::QmlJSQuickFixAssistInterface *assistInterface() const;

    /// \returns The name of the file for for which this operation is invoked.
    QString fileName() const;

private:
    QmlJSQuickFixInterface m_interface;
};

class QmlJSQuickFixFactory: public TextEditor::QuickFixFactory
{
    Q_OBJECT

protected:
    QmlJSQuickFixFactory() {}

    void matchingOperations(const QuickFixInterface &interface, QuickFixOperations &result);

    /*!
        Implement this function to match and create the appropriate
        QmlJSQuickFixOperation objects.
     */
    virtual void match(const QmlJSQuickFixInterface &interface, TextEditor::QuickFixOperations &result) = 0;
};

} // namespace QmlJSEditor
