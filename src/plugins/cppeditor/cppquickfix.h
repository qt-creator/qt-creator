/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef CPPQUICKFIX_H
#define CPPQUICKFIX_H

#include "cppeditor_global.h"
#include <texteditor/quickfix.h>

namespace CPlusPlus {
class CppModelManagerInterface;
}

namespace CppTools {
    class CppRefactoringFile;
    class CppRefactoringChanges;
    typedef QSharedPointer<CppRefactoringFile> CppRefactoringFilePtr;
} // namespace CppTools

namespace ExtensionSystem {
class IPlugin;
}

namespace CppEditor {

namespace Internal {
class CppQuickFixAssistInterface;
}

class CPPEDITOR_EXPORT CppQuickFixOperation: public TextEditor::QuickFixOperation
{
public:
    explicit CppQuickFixOperation(
        const QSharedPointer<const Internal::CppQuickFixAssistInterface> &interface,
        int priority = -1);
    virtual ~CppQuickFixOperation();

    virtual void perform();

protected:
    virtual void performChanges(const CppTools::CppRefactoringFilePtr &currentFile,
                                const CppTools::CppRefactoringChanges &refactoring) = 0;

    QString fileName() const;

    const Internal::CppQuickFixAssistInterface *assistInterface() const;

private:
    QSharedPointer<const Internal::CppQuickFixAssistInterface> m_interface;
};

class CPPEDITOR_EXPORT CppQuickFixFactory: public TextEditor::QuickFixFactory
{
    Q_OBJECT

public:
    CppQuickFixFactory();
    virtual ~CppQuickFixFactory();

    virtual QList<TextEditor::QuickFixOperation::Ptr>
        matchingOperations(const QSharedPointer<const TextEditor::IAssistInterface> &interface);

    /*!
        Implement this method to match and create the appropriate
        CppQuickFixOperation objects.
     */
    virtual QList<CppQuickFixOperation::Ptr> match(
        const QSharedPointer<const Internal::CppQuickFixAssistInterface> &interface) = 0;

protected:
    /*!
        Creates a list of 1 single element: the shared-pointer to the given
        operation. This shared-pointer takes over the ownership (meaning the
        responsibility to delete the operation).
     */
    static QList<CppQuickFixOperation::Ptr> singleResult(CppQuickFixOperation *operation);

    /// Utility method which creates an empty list.
    static QList<CppQuickFixOperation::Ptr> noResult();
};

} // namespace CppEditor

#endif // CPPQUICKFIX_H
