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

#ifndef CPPINSERTQTPROPERTYMEMBERS_H
#define CPPINSERTQTPROPERTYMEMBERS_H

#include <cppeditor/cppquickfix.h>

#include <QString>

namespace CPlusPlus {
class QtPropertyDeclarationAST;
class Class;
}

namespace TextEditor {
class RefactoringFile;
typedef QSharedPointer<RefactoringFile> RefactoringFilePtr;
}

namespace Utils {
class ChangeSet;
}

namespace CppTools {
class InsertionLocation;
}

namespace CppEditor {
namespace Internal {

class InsertQtPropertyMembers : public CppQuickFixFactory
{
    Q_OBJECT

public:
    virtual QList<CppQuickFixOperation::Ptr>
        match(const QSharedPointer<const Internal::CppQuickFixAssistInterface> &interface);

private:
    enum GenerateFlag {
        GenerateGetter = 1 << 0,
        GenerateSetter = 1 << 1,
        GenerateSignal = 1 << 2,
        GenerateStorage = 1 << 3
    };

    class Operation: public CppQuickFixOperation
    {
    public:
        Operation(const QSharedPointer<const Internal::CppQuickFixAssistInterface> &interface,
                  int priority,
                  CPlusPlus::QtPropertyDeclarationAST *declaration, CPlusPlus::Class *klass,
                  int generateFlags,
                  const QString &getterName, const QString &setterName, const QString &signalName,
                  const QString &storageName);

        virtual void performChanges(const CppTools::CppRefactoringFilePtr &file,
                                    const CppTools::CppRefactoringChanges &refactoring);

    private:
        void insertAndIndent(const TextEditor::RefactoringFilePtr &file, Utils::ChangeSet *changeSet,
                             const CppTools::InsertionLocation &loc, const QString &text);

        CPlusPlus::QtPropertyDeclarationAST *m_declaration;
        CPlusPlus::Class *m_class;
        int m_generateFlags;
        QString m_getterName;
        QString m_setterName;
        QString m_signalName;
        QString m_storageName;
    };
};

} // namespace Internal
} // namespace CppEditor

#endif // CPPINSERTQTPROPERTYMEMBERS_H
