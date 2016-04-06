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

#include "cppvirtualfunctionassistprovider.h"

#include "cppeditorconstants.h"
#include "cppvirtualfunctionproposalitem.h"

#include <cplusplus/Icons.h>
#include <cplusplus/Overview.h>

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>

#include <cpptools/cpptoolsreuse.h>
#include <cpptools/functionutils.h>
#include <cpptools/symbolfinder.h>
#include <cpptools/typehierarchybuilder.h>

#include <texteditor/codeassist/genericproposalmodel.h>
#include <texteditor/codeassist/genericproposal.h>
#include <texteditor/codeassist/genericproposalwidget.h>
#include <texteditor/codeassist/assistinterface.h>
#include <texteditor/codeassist/iassistprocessor.h>
#include <texteditor/codeassist/iassistproposal.h>
#include <texteditor/texteditorconstants.h>

#include <utils/qtcassert.h>

using namespace CPlusPlus;
using namespace CppEditor::Internal;
using namespace CppTools;
using namespace TextEditor;

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
    }

protected:
    bool eventFilter(QObject *o, QEvent *e) override
    {
        if (e->type() == QEvent::ShortcutOverride && m_sequence.count() == 1) {
            QKeyEvent *ke = static_cast<QKeyEvent *>(e);
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
        GenericProposalModel *proposalModel = model();
        if (proposalModel && proposalModel->size() == 1) {
            emit proposalItemActivated(proposalModel->proposalItem(0));
            deleteLater();
            return;
        }
        GenericProposalWidget::showProposal(prefix);
    }

private:
    QKeySequence m_sequence;
};

class VirtualFunctionProposal : public GenericProposal
{
public:
    VirtualFunctionProposal(int cursorPos,
                            const QList<AssistProposalItemInterface *> &items,
                            bool openInSplit)
        : GenericProposal(cursorPos, items)
        , m_openInSplit(openInSplit)
    {}

    bool isFragile() const override { return true; }

    IAssistProposalWidget *createWidget() const override
    { return new VirtualFunctionProposalWidget(m_openInSplit); }

private:
    bool m_openInSplit;
};

class VirtualFunctionAssistProcessor : public IAssistProcessor
{
public:
    VirtualFunctionAssistProcessor(const VirtualFunctionAssistProvider::Parameters &params)
        : m_params(params)
    {}

    IAssistProposal *immediateProposal(const AssistInterface *) override
    {
        QTC_ASSERT(m_params.function, return 0);

        auto *hintItem = new VirtualFunctionProposalItem(TextEditorWidget::Link());
        hintItem->setText(QCoreApplication::translate("VirtualFunctionsAssistProcessor",
                                                      "...searching overrides"));
        hintItem->setOrder(-1000);

        QList<AssistProposalItemInterface *> items;
        items << itemFromFunction(m_params.function);
        items << hintItem;
        return new VirtualFunctionProposal(m_params.cursorPosition, items, m_params.openInNextSplit);
    }

    IAssistProposal *perform(const AssistInterface *assistInterface) override
    {
        delete assistInterface;

        QTC_ASSERT(m_params.function, return 0);
        QTC_ASSERT(m_params.staticClass, return 0);
        QTC_ASSERT(!m_params.snapshot.isEmpty(), return 0);

        Class *functionsClass = m_finder.findMatchingClassDeclaration(m_params.function,
                                                                      m_params.snapshot);
        if (!functionsClass)
            return 0;

        const QList<Function *> overrides = FunctionUtils::overrides(
            m_params.function, functionsClass, m_params.staticClass, m_params.snapshot);
        if (overrides.isEmpty())
            return 0;

        QList<AssistProposalItemInterface *> items;
        foreach (Function *func, overrides)
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
        const TextEditorWidget::Link link = CppTools::linkToSymbol(maybeDefinitionFor(func));
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

VirtualFunctionAssistProvider::VirtualFunctionAssistProvider()
{
}

bool VirtualFunctionAssistProvider::configure(const Parameters &parameters)
{
    m_params = parameters;
    return true;
}

IAssistProvider::RunType VirtualFunctionAssistProvider::runType() const
{
    return AsynchronousWithThread;
}

bool VirtualFunctionAssistProvider::supportsEditor(Core::Id editorId) const
{
    return editorId == Constants::CPPEDITOR_ID;
}

IAssistProcessor *VirtualFunctionAssistProvider::createProcessor() const
{
    return new VirtualFunctionAssistProcessor(m_params);
}
