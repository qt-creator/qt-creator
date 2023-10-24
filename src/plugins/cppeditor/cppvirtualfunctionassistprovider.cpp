// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppvirtualfunctionassistprovider.h"

#include "cppvirtualfunctionproposalitem.h"

#include "cppeditortr.h"
#include "cpptoolsreuse.h"
#include "functionutils.h"
#include "symbolfinder.h"

#include <cplusplus/Icons.h>
#include <cplusplus/Overview.h>

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>

#include <texteditor/codeassist/assistinterface.h>
#include <texteditor/codeassist/asyncprocessor.h>
#include <texteditor/codeassist/genericproposalmodel.h>
#include <texteditor/codeassist/genericproposalwidget.h>
#include <texteditor/codeassist/iassistprocessor.h>
#include <texteditor/codeassist/iassistproposal.h>
#include <texteditor/texteditorconstants.h>

#include <utils/qtcassert.h>

using namespace CPlusPlus;
using namespace TextEditor;

namespace CppEditor {

/// Activate current item with the same shortcut that is configured for Follow Symbol Under Cursor.
/// This is limited to single-key shortcuts without modifiers.
class VirtualFunctionProposalWidget : public GenericProposalWidget
{
public:
    VirtualFunctionProposalWidget(bool openInSplit)
    {
        const char *id = openInSplit
            ? TextEditor::Constants::FOLLOW_SYMBOL_UNDER_CURSOR_IN_NEXT_SPLIT
            : TextEditor::Constants::FOLLOW_SYMBOL_UNDER_CURSOR;
        if (Core::Command *command = Core::ActionManager::command(id))
            m_sequence = command->keySequence();
        setFragile(true);
    }

protected:
    bool eventFilter(QObject *o, QEvent *e) override
    {
        if (e->type() == QEvent::ShortcutOverride && m_sequence.count() == 1) {
            auto ke = static_cast<const QKeyEvent*>(e);
            const QKeySequence seq(ke->key());
            if (seq == m_sequence) {
                activateCurrentProposalItem();
                e->accept();
                return true;
            }
        }
        return GenericProposalWidget::eventFilter(o, e);
    }

    void showProposal(const QString &prefix) override
    {
        GenericProposalModelPtr proposalModel = model();
        if (proposalModel && proposalModel->size() == 1) {
            const auto item = dynamic_cast<VirtualFunctionProposalItem *>(
                        proposalModel->proposalItem(0));
            if (item && item->link().hasValidTarget()) {
                emit proposalItemActivated(proposalModel->proposalItem(0));
                deleteLater();
                return;
            }
        }
        GenericProposalWidget::showProposal(prefix);
    }

private:
    QKeySequence m_sequence;
};



class VirtualFunctionAssistProcessor : public AsyncProcessor
{
public:
    VirtualFunctionAssistProcessor(const VirtualFunctionAssistProvider::Parameters &params)
        : m_params(params)
    {}

    IAssistProposal *immediateProposal() override
    {
        QTC_ASSERT(m_params.function, return nullptr);

        auto *hintItem = new VirtualFunctionProposalItem(Utils::Link());
        hintItem->setText(Tr::tr("collecting overrides..."));
        hintItem->setOrder(-1000);

        QList<AssistProposalItemInterface *> items;
        items << itemFromFunction(m_params.function);
        items << hintItem;
        return new VirtualFunctionProposal(m_params.cursorPosition, items, m_params.openInNextSplit);
    }

    IAssistProposal *performAsync() override
    {
        QTC_ASSERT(m_params.function, return nullptr);
        QTC_ASSERT(m_params.staticClass, return nullptr);
        QTC_ASSERT(!m_params.snapshot.isEmpty(), return nullptr);

        Class *functionsClass = m_finder.findMatchingClassDeclaration(m_params.function,
                                                                      m_params.snapshot);
        if (!functionsClass)
            return nullptr;

        const QList<Function *> overrides = Internal::FunctionUtils::overrides(
            m_params.function, functionsClass, m_params.staticClass, m_params.snapshot);
        if (overrides.isEmpty())
            return nullptr;

        QList<AssistProposalItemInterface *> items;
        for (Function *func : overrides)
            items << itemFromFunction(func);
        items.first()->setOrder(1000); // Ensure top position for function of static type

        return new VirtualFunctionProposal(m_params.cursorPosition, items, m_params.openInNextSplit);
    }

private:
    Function *maybeDefinitionFor(Function *func) const
    {
        if (Function *definition = m_finder.findMatchingDefinition(func, m_params.snapshot))
            return definition;
        return func;
    }

    VirtualFunctionProposalItem *itemFromFunction(Function *func) const
    {
        const Utils::Link link = maybeDefinitionFor(func)->toLink();
        QString text = m_overview.prettyName(LookupContext::fullyQualifiedName(func));
        if (func->isPureVirtual())
            text += QLatin1String(" = 0");

        auto *item = new VirtualFunctionProposalItem(link, m_params.openInNextSplit);
        item->setText(text);
        item->setIcon(Icons::iconForSymbol(func));

        return item;
    }

    VirtualFunctionAssistProvider::Parameters m_params;
    Overview m_overview;
    mutable SymbolFinder m_finder;
};

VirtualFunctionAssistProvider::VirtualFunctionAssistProvider() = default;

bool VirtualFunctionAssistProvider::configure(const Parameters &parameters)
{
    m_params = parameters;
    return true;
}

IAssistProcessor *VirtualFunctionAssistProvider::createProcessor(const AssistInterface *) const
{
    return new VirtualFunctionAssistProcessor(m_params);
}

VirtualFunctionProposal::VirtualFunctionProposal(
        int cursorPos, const QList<AssistProposalItemInterface *> &items, bool openInSplit)
    : GenericProposal(cursorPos, items), m_openInSplit(openInSplit)
{ }

IAssistProposalWidget *VirtualFunctionProposal::createWidget() const
{
    return new VirtualFunctionProposalWidget(m_openInSplit);
}

} // namespace CppEditor
