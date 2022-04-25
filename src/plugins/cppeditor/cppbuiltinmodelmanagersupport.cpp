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

#include "cppbuiltinmodelmanagersupport.h"

#include "builtineditordocumentprocessor.h"
#include "cppcompletionassist.h"
#include "cppeditorwidget.h"
#include "cppelementevaluator.h"
#include "cppfollowsymbolundercursor.h"
#include "cppoverviewmodel.h"
#include "cpprefactoringengine.h"
#include "cpptoolsreuse.h"
#include "symbolfinder.h"

#include <app/app_version.h>
#include <texteditor/basehoverhandler.h>
#include <utils/executeondestruction.h>

#include <QCoreApplication>

using namespace Core;
using namespace TextEditor;

namespace CppEditor::Internal {
namespace {
class CppHoverHandler : public TextEditor::BaseHoverHandler
{
private:
    void identifyMatch(TextEditor::TextEditorWidget *editorWidget,
                       int pos,
                       ReportPriority report) override
    {
        Utils::ExecuteOnDestruction reportPriority([this, report](){ report(priority()); });

        QTextCursor tc(editorWidget->document());
        tc.setPosition(pos);

        CppElementEvaluator evaluator(editorWidget);
        evaluator.setTextCursor(tc);
        evaluator.execute();
        QString tip;
        if (evaluator.hasDiagnosis()) {
            tip += evaluator.diagnosis();
            setPriority(Priority_Diagnostic);
        }
        const QStringList fallback = identifierWordsUnderCursor(tc);
        if (evaluator.identifiedCppElement()) {
            const QSharedPointer<CppElement> &cppElement = evaluator.cppElement();
            const QStringList candidates = cppElement->helpIdCandidates;
            const HelpItem helpItem(candidates + fallback, cppElement->helpMark, cppElement->helpCategory);
            setLastHelpItemIdentified(helpItem);
            if (!helpItem.isValid())
                tip += cppElement->tooltip;
        } else {
            setLastHelpItemIdentified({fallback, {}, HelpItem::Unknown});
        }
        setToolTip(tip);
    }
};
} // anonymous namespace

QString BuiltinModelManagerSupportProvider::id() const
{
    return QLatin1String("CppEditor.BuiltinCodeModel");
}

QString BuiltinModelManagerSupportProvider::displayName() const
{
    return QCoreApplication::translate("ModelManagerSupportInternal::displayName",
                                       "%1 Built-in").arg(Core::Constants::IDE_DISPLAY_NAME);
}

ModelManagerSupport::Ptr BuiltinModelManagerSupportProvider::createModelManagerSupport()
{
    return ModelManagerSupport::Ptr(new BuiltinModelManagerSupport);
}

BuiltinModelManagerSupport::BuiltinModelManagerSupport()
    : m_completionAssistProvider(new InternalCompletionAssistProvider),
      m_followSymbol(new FollowSymbolUnderCursor),
      m_refactoringEngine(new CppRefactoringEngine)
{
}

BuiltinModelManagerSupport::~BuiltinModelManagerSupport() = default;

BaseEditorDocumentProcessor *BuiltinModelManagerSupport::createEditorDocumentProcessor(
        TextEditor::TextDocument *baseTextDocument)
{
    return new BuiltinEditorDocumentProcessor(baseTextDocument);
}

CppCompletionAssistProvider *BuiltinModelManagerSupport::completionAssistProvider()
{
    return m_completionAssistProvider.data();
}


CppCompletionAssistProvider *BuiltinModelManagerSupport::functionHintAssistProvider()
{
    return nullptr;
}

TextEditor::BaseHoverHandler *BuiltinModelManagerSupport::createHoverHandler()
{
    return new CppHoverHandler;
}

RefactoringEngineInterface &BuiltinModelManagerSupport::refactoringEngineInterface()
{
    return *m_refactoringEngine;
}

std::unique_ptr<AbstractOverviewModel> BuiltinModelManagerSupport::createOverviewModel()
{
    return std::make_unique<OverviewModel>();
}

void BuiltinModelManagerSupport::followSymbol(const CursorInEditor &data,
                                              Utils::ProcessLinkCallback &&processLinkCallback,
                                              bool resolveTarget, bool inNextSplit)
{
    SymbolFinder finder;
    m_followSymbol->findLink(data, std::move(processLinkCallback),
            resolveTarget, CppModelManager::instance()->snapshot(),
            data.editorWidget()->semanticInfo().doc, &finder, inNextSplit);
}

void BuiltinModelManagerSupport::switchDeclDef(const CursorInEditor &data,
                                               Utils::ProcessLinkCallback &&processLinkCallback)
{
    SymbolFinder finder;
    m_followSymbol->switchDeclDef(data, std::move(processLinkCallback),
            CppModelManager::instance()->snapshot(), data.editorWidget()->semanticInfo().doc,
            &finder);
}

} // namespace CppEditor::Internal
