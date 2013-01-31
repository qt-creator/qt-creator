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

#ifndef QMLJSQUICKFIX_H
#define QMLJSQUICKFIX_H

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
        Implement this method to match and create the appropriate
        QmlJSQuickFixOperation objects.
     */
    virtual void match(const QmlJSQuickFixInterface &interface, TextEditor::QuickFixOperations &result) = 0;
};

} // namespace QmlJSEditor

#endif // QMLJSQUICKFIX_H
