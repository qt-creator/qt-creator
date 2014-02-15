/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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


#include "cppvirtualfunctionassistprovider.h"

#include "cppeditor.h"
#include "cppeditorconstants.h"
#include "cppvirtualfunctionproposalitem.h"

#include <cplusplus/Icons.h>
#include <cplusplus/Overview.h>

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>

#include <cpptools/functionutils.h>
#include <cpptools/symbolfinder.h>
#include <cpptools/typehierarchybuilder.h>

#include <texteditor/codeassist/basicproposalitemlistmodel.h>
#include <texteditor/codeassist/genericproposal.h>
#include <texteditor/codeassist/genericproposalwidget.h>
#include <texteditor/codeassist/iassistinterface.h>
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
    bool eventFilter(QObject *o, QEvent *e)
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

private:
    QKeySequence m_sequence;
};

class VirtualFunctionProposal : public GenericProposal
{
public:
    VirtualFunctionProposal(int cursorPos, IGenericProposalModel *model, bool openInSplit)
        : GenericProposal(cursorPos, model)
        , m_openInSplit(openInSplit)
    {}

    bool isFragile() const { return true; }

    IAssistProposalWidget *createWidget() const
    { return new VirtualFunctionProposalWidget(m_openInSplit); }

private:
    bool m_openInSplit;
};

class VirtualFunctionsAssistProcessor : public IAssistProcessor
{
public:
    VirtualFunctionsAssistProcessor(const VirtualFunctionAssistProvider::Parameters &params)
        : m_params(params)
    {}

    IAssistProposal *immediateProposal(const TextEditor::IAssistInterface *)
    {
        QTC_ASSERT(m_params.function, return 0);

        BasicProposalItem *hintItem = new VirtualFunctionProposalItem(CPPEditorWidget::Link());
        hintItem->setText(QCoreApplication::translate("VirtualFunctionsAssistProcessor",
                                                      "...searching overrides"));
        hintItem->setOrder(-1000);

        QList<BasicProposalItem *> items;
        items << itemFromSymbol(maybeDefinitionFor(m_params.function));
        items << hintItem;
        return new VirtualFunctionProposal(m_params.cursorPosition,
                                           new BasicProposalItemListModel(items),
                                           m_params.openInNextSplit);
    }

    IAssistProposal *perform(const IAssistInterface *)
    {
        QTC_ASSERT(m_params.function, return 0);
        QTC_ASSERT(m_params.staticClass, return 0);
        QTC_ASSERT(!m_params.snapshot.isEmpty(), return 0);

        Class *functionsClass = m_finder.findMatchingClassDeclaration(m_params.function,
                                                                      m_params.snapshot);
        if (!functionsClass)
            return 0;

        const QList<Symbol *> overrides = FunctionUtils::overrides(
            m_params.function, functionsClass, m_params.staticClass, m_params.snapshot);
        if (overrides.isEmpty())
            return 0;

        QList<BasicProposalItem *> items;
        foreach (Symbol *symbol, overrides)
            items << itemFromSymbol(maybeDefinitionFor(symbol));
        items.first()->setOrder(1000); // Ensure top position for function of static type

        return new VirtualFunctionProposal(m_params.cursorPosition,
                                           new BasicProposalItemListModel(items),
                                           m_params.openInNextSplit);
    }

private:
    Symbol *maybeDefinitionFor(Symbol *symbol)
    {
        if (Function *definition = m_finder.findMatchingDefinition(symbol, m_params.snapshot))
            return definition;
        return symbol;
    }

    BasicProposalItem *itemFromSymbol(Symbol *symbol) const
    {
        const QString text = m_overview.prettyName(LookupContext::fullyQualifiedName(symbol));
        const CPPEditorWidget::Link link = CPPEditorWidget::linkToSymbol(symbol);

        BasicProposalItem *item = new VirtualFunctionProposalItem(link, m_params.openInNextSplit);
        item->setText(text);
        item->setIcon(m_icons.iconForSymbol(symbol));

        return item;
    }

    VirtualFunctionAssistProvider::Parameters m_params;
    Overview m_overview;
    Icons m_icons;
    CppTools::SymbolFinder m_finder;
};

VirtualFunctionAssistProvider::VirtualFunctionAssistProvider()
{
}

bool VirtualFunctionAssistProvider::configure(const Parameters &parameters)
{
    m_params = parameters;
    return true;
}

bool VirtualFunctionAssistProvider::isAsynchronous() const
{
    return true;
}

bool VirtualFunctionAssistProvider::supportsEditor(const Core::Id &editorId) const
{
    return editorId == CppEditor::Constants::CPPEDITOR_ID;
}

IAssistProcessor *VirtualFunctionAssistProvider::createProcessor() const
{
    return new VirtualFunctionsAssistProcessor(m_params);
}
