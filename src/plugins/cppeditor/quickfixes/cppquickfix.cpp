// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppquickfix.h"

#include "../baseeditordocumentprocessor.h"
#include "../cppeditortr.h"
#include "../cppeditorwidget.h"
#include "../cppfunctiondecldeflink.h"
#include "../cpprefactoringchanges.h"
#include "assigntolocalvariable.h"
#include "bringidentifierintoscope.h"
#include "completeswitchstatement.h"
#include "convertfromandtopointer.h"
#include "convertnumericliteral.h"
#include "convertqt4connect.h"
#include "convertstringliteral.h"
#include "converttocamelcase.h"
#include "converttometamethodcall.h"
#include "cppcodegenerationquickfixes.h"
#include "cppinsertvirtualmethods.h"
#include "cppquickfixassistant.h"
#include "createdeclarationfromuse.h"
#include "extractfunction.h"
#include "extractliteralasparameter.h"
#include "insertfunctiondefinition.h"
#include "logicaloperationquickfixes.h"
#include "moveclasstoownfile.h"
#include "movefunctiondefinition.h"
#include "rearrangeparamdeclarationlist.h"
#include "reformatpointerdeclaration.h"
#include "removeusingnamespace.h"
#include "rewritecomment.h"
#include "rewritecontrolstatements.h"
#include "splitsimpledeclaration.h"
#include "synchronizememberfunctionorder.h"

#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/pluginspec.h>

using namespace CPlusPlus;
using namespace TextEditor;

namespace CppEditor {
namespace Internal {
namespace {

class ApplyDeclDefLinkOperation : public CppQuickFixOperation
{
public:
    explicit ApplyDeclDefLinkOperation(const CppQuickFixInterface &interface,
                                       const std::shared_ptr<FunctionDeclDefLink> &link)
        : CppQuickFixOperation(interface, 100)
        , m_link(link)
    {}

    void perform() override
    {
        if (editor()->declDefLink() == m_link)
            editor()->applyDeclDefLinkChanges(/*don't jump*/false);
    }

private:
    std::shared_ptr<FunctionDeclDefLink> m_link;
};

class ExtraRefactoringOperations : public CppQuickFixFactory
{
public:
    void doMatch(const CppQuickFixInterface &interface, QuickFixOperations &result) override
    {
        const auto processor = CppModelManager::cppEditorDocumentProcessor(interface.filePath());
        if (processor) {
            const auto clangFixItOperations = processor->extraRefactoringOperations(interface);
            result.append(clangFixItOperations);
        }
    }
};

//! Applies function signature changes
class ApplyDeclDefLinkChanges: public CppQuickFixFactory
{
public:
    void doMatch(const CppQuickFixInterface &interface, TextEditor::QuickFixOperations &result) override
    {
        std::shared_ptr<FunctionDeclDefLink> link = interface.editor()->declDefLink();
        if (!link || !link->isMarkerVisible())
            return;

        auto op = new ApplyDeclDefLinkOperation(interface, link);
        op->setDescription(Tr::tr("Apply Function Signature Changes"));
        result << op;
    }
};

} // namespace

static ExtensionSystem::IPlugin *getCppEditor()
{
    using namespace ExtensionSystem;
    for (PluginSpec * const spec : PluginManager::plugins()) {
        if (spec->name() == "CppEditor")
            return spec->plugin();
    }
    QTC_ASSERT(false, return nullptr);
}

CppQuickFixOperation::~CppQuickFixOperation() = default;

void createCppQuickFixFactories()
{
    new ApplyDeclDefLinkChanges;
    new ExtraRefactoringOperations;

    registerAssignToLocalVariableQuickfix();
    registerBringIdentifierIntoScopeQuickfixes();
    registerCodeGenerationQuickfixes();
    registerCompleteSwitchStatementQuickfix();
    registerConvertFromAndToPointerQuickfix();
    registerConvertNumericLiteralQuickfix();
    registerConvertQt4ConnectQuickfix();
    registerConvertStringLiteralQuickfixes();
    registerConvertToCamelCaseQuickfix();
    registerConvertToMetaMethodCallQuickfix();
    registerCreateDeclarationFromUseQuickfixes();
    registerExtractFunctionQuickfix();
    registerExtractLiteralAsParameterQuickfix();
    registerInsertFunctionDefinitionQuickfixes();
    registerInsertVirtualMethodsQuickfix();
    registerLogicalOperationQuickfixes();
    registerMoveClassToOwnFileQuickfix();
    registerMoveFunctionDefinitionQuickfixes();
    registerRearrangeParamDeclarationListQuickfix();
    registerReformatPointerDeclarationQuickfix();
    registerRemoveUsingNamespaceQuickfix();
    registerRewriteCommentQuickfixes();
    registerRewriteControlStatementQuickfixes();
    registerSplitSimpleDeclarationQuickfix();
    registerSynchronizeMemberFunctionOrderQuickfix();
}

static QList<CppQuickFixFactory *> g_cppQuickFixFactories;

void destroyCppQuickFixFactories()
{
    for (int i = g_cppQuickFixFactories.size(); --i >= 0; )
        delete g_cppQuickFixFactories.at(i);
}

} // namespace Internal

CppQuickFixFactory::CppQuickFixFactory()
{
    Internal::g_cppQuickFixFactories.append(this);
}

CppQuickFixFactory::~CppQuickFixFactory()
{
    Internal::g_cppQuickFixFactories.removeOne(this);
}

ExtensionSystem::IPlugin *CppQuickFixFactory::cppEditor()
{
    static ExtensionSystem::IPlugin * const plugin = Internal::getCppEditor();
    return plugin;
}

void CppQuickFixFactory::match(const Internal::CppQuickFixInterface &interface,
                               QuickFixOperations &result)
{
    if (m_clangdReplacement) {
        if (const auto clangdVersion = CppModelManager::usesClangd(
                interface.currentFile()->editor()->textDocument());
            clangdVersion && clangdVersion >= m_clangdReplacement) {
            return;
        }
    }

    doMatch(interface, result);
}

const QList<CppQuickFixFactory *> &CppQuickFixFactory::cppQuickFixFactories()
{
    return Internal::g_cppQuickFixFactories;
}

} // namespace CppEditor
