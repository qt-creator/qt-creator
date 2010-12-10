/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef CPPINSERTQTPROPERTYMEMBERS_H
#define CPPINSERTQTPROPERTYMEMBERS_H

#include <cppeditor/cppquickfix.h>

#include <QtCore/QString>

namespace CPlusPlus {
class QtPropertyDeclarationAST;
class Class;
}

namespace TextEditor {
class RefactoringFile;
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
    virtual QList<CppQuickFixOperation::Ptr> match(const CppQuickFixState &state);

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
        Operation(const CppQuickFixState &state, int priority,
                  CPlusPlus::QtPropertyDeclarationAST *declaration, CPlusPlus::Class *klass,
                  int generateFlags,
                  const QString &getterName, const QString &setterName, const QString &signalName,
                  const QString &storageName);

        virtual void performChanges(CppTools::CppRefactoringFile *file,
                                    CppTools::CppRefactoringChanges *refactoring);

    private:
        void insertAndIndent(TextEditor::RefactoringFile *file, Utils::ChangeSet *changeSet,
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
