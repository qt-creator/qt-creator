// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "converttocamelcase.h"

#include "../cppeditortr.h"
#include "../cppeditorwidget.h"
#include "../cpprefactoringchanges.h"
#include "cppquickfix.h"

#ifdef WITH_TESTS
#include "cppquickfix_test.h"
#endif

using namespace CPlusPlus;
using namespace Utils;

namespace CppEditor::Internal {
namespace {

class ConvertToCamelCaseOp: public CppQuickFixOperation
{
public:
    ConvertToCamelCaseOp(const CppQuickFixInterface &interface, const QString &name,
                         const AST *nameAst, bool test)
        : CppQuickFixOperation(interface, -1)
        , m_name(name)
        , m_nameAst(nameAst)
        , m_isAllUpper(name.isUpper())
        , m_test(test)
    {
        setDescription(Tr::tr("Convert to Camel Case"));
    }

    static bool isConvertibleUnderscore(const QString &name, int pos)
    {
        return name.at(pos) == QLatin1Char('_') && name.at(pos+1).isLetter()
               && !(pos == 1 && name.at(0) == QLatin1Char('m'));
    }

private:
    void perform() override
    {
        QString newName = m_isAllUpper ? m_name.toLower() : m_name;
        for (int i = 1; i < newName.length(); ++i) {
            const QChar c = newName.at(i);
            if (c.isUpper() && m_isAllUpper) {
                newName[i] = c.toLower();
            } else if (i < newName.length() - 1 && isConvertibleUnderscore(newName, i)) {
                newName.remove(i, 1);
                newName[i] = newName.at(i).toUpper();
            }
        }
        if (m_test)
            currentFile()->apply(ChangeSet::makeReplace(currentFile()->range(m_nameAst), newName));
        else
            editor()->renameUsages(newName);
    }

    const QString m_name;
    const AST * const m_nameAst;
    const bool m_isAllUpper;
    const bool m_test;
};

/*!
  Turns "an_example_symbol" into "anExampleSymbol" and
  "AN_EXAMPLE_SYMBOL" into "AnExampleSymbol".

  Activates on: identifiers
*/
class ConvertToCamelCase : public CppQuickFixFactory
{
    void doMatch(const CppQuickFixInterface &interface, QuickFixOperations &result) override
    {
        const QList<AST *> &path = interface.path();

        if (path.isEmpty())
            return;

        AST * const ast = path.last();
        const Name *name = nullptr;
        const AST *astForName = nullptr;
        if (const NameAST * const nameAst = ast->asName()) {
            if (nameAst->name && nameAst->name->asNameId()) {
                astForName = nameAst;
                name = nameAst->name;
            }
        } else if (const NamespaceAST * const namespaceAst = ast->asNamespace()) {
            astForName = namespaceAst;
            name = namespaceAst->symbol->name();
        }

        if (!name)
            return;

        QString nameString = QString::fromUtf8(name->identifier()->chars());
        if (nameString.length() < 3)
            return;
        for (int i = 1; i < nameString.length() - 1; ++i) {
            if (ConvertToCamelCaseOp::isConvertibleUnderscore(nameString, i)) {
                result << new ConvertToCamelCaseOp(interface, nameString, astForName, testMode());
                return;
            }
        }
    }
};

#ifdef WITH_TESTS
class ConvertToCamelCaseTest : public Tests::CppQuickFixTestObject
{
    Q_OBJECT
public:
    using CppQuickFixTestObject::CppQuickFixTestObject;
};
#endif

} // namespace

void registerConvertToCamelCaseQuickfix()
{
    REGISTER_QUICKFIX_FACTORY_WITH_STANDARD_TEST(ConvertToCamelCase);
}

} // namespace CppEditor::Internal

#ifdef WITH_TESTS
#include <converttocamelcase.moc>
#endif
