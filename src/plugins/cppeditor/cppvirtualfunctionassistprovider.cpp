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


#include "cppvirtualfunctionassistprovider.h"

#include "cppeditorconstants.h"
#include "cppelementevaluator.h"

#include <cplusplus/Icons.h>
#include <cplusplus/Overview.h>

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>

#include <texteditor/codeassist/basicproposalitem.h>
#include <texteditor/codeassist/basicproposalitemlistmodel.h>
#include <texteditor/codeassist/genericproposal.h>
#include <texteditor/codeassist/genericproposalwidget.h>
#include <texteditor/codeassist/iassistinterface.h>
#include <texteditor/codeassist/iassistprocessor.h>
#include <texteditor/codeassist/iassistproposal.h>

#include <utils/qtcassert.h>

using namespace CPlusPlus;
using namespace CppEditor::Internal;
using namespace TextEditor;

class VirtualFunctionProposalItem: public BasicProposalItem {
public:
    VirtualFunctionProposalItem(const BaseTextEditorWidget::Link &link, bool openInSplit = true)
        : m_link(link), m_openInSplit(openInSplit) {}

    void apply(BaseTextEditor * /* editor */, int /* basePosition */) const
    {
        if (!m_link.hasValidTarget())
            return;

        Core::EditorManager::OpenEditorFlags flags;
        if (m_openInSplit)
            flags |= Core::EditorManager::OpenInOtherSplit;
        Core::EditorManager::openEditorAt(m_link.targetFileName,
                                          m_link.targetLine,
                                          m_link.targetColumn,
                                          CppEditor::Constants::CPPEDITOR_ID,
                                          flags);
    }

private:
    BaseTextEditorWidget::Link m_link;
    bool m_openInSplit;
};

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

    bool isFragile() const
    { return true; }

    IAssistProposalWidget *createWidget() const
    { return new VirtualFunctionProposalWidget(m_openInSplit); }

private:
    bool m_openInSplit;
};

class VirtualFunctionsAssistProcessor : public IAssistProcessor
{
public:
    VirtualFunctionsAssistProcessor(const VirtualFunctionAssistProvider *provider)
        : m_startClass(provider->startClass())
        , m_function(provider->function())
        , m_snapshot(provider->snapshot())
        , m_openInNextSplit(provider->openInNextSplit())
    {}

    IAssistProposal *immediateProposal(const TextEditor::IAssistInterface *interface)
    {
        QTC_ASSERT(m_function, return 0);

        BasicProposalItem *hintItem = new VirtualFunctionProposalItem(CPPEditorWidget::Link());
        hintItem->setText(QCoreApplication::translate("VirtualFunctionsAssistProcessor",
                                                      "...searching overrides"));
        hintItem->setOrder(-1000);

        QList<BasicProposalItem *> items;
        items << itemFromSymbol(m_function, m_function);
        items << hintItem;
        return new VirtualFunctionProposal(interface->position(),
                                           new BasicProposalItemListModel(items),
                                           m_openInNextSplit);
    }

    IAssistProposal *perform(const IAssistInterface *interface)
    {
        if (!interface)
            return 0;

        QTC_ASSERT(m_startClass, return 0);
        QTC_ASSERT(m_function, return 0);
        QTC_ASSERT(!m_snapshot.isEmpty(), return 0);

        const QList<Symbol *> overrides = FunctionHelper::overrides(m_startClass, m_function,
                                                                     m_snapshot);
        QList<BasicProposalItem *> items;
        foreach (Symbol *symbol, overrides)
            items << itemFromSymbol(symbol, m_function);

        return new VirtualFunctionProposal(interface->position(),
                                           new BasicProposalItemListModel(items),
                                           m_openInNextSplit);
    }

    BasicProposalItem *itemFromSymbol(Symbol *symbol, Symbol *firstSymbol) const
    {
        const QString text = m_overview.prettyName(LookupContext::fullyQualifiedName(symbol));
        const CPPEditorWidget::Link link = CPPEditorWidget::linkToSymbol(symbol);

        BasicProposalItem *item = new VirtualFunctionProposalItem(link, m_openInNextSplit);
        item->setText(text);
        item->setIcon(m_icons.iconForSymbol(symbol));
        if (symbol == firstSymbol)
            item->setOrder(1000); // Ensure top position for function of static type

        return item;
    }

private:
    Class *m_startClass;
    Function *m_function;
    Snapshot m_snapshot;
    bool m_openInNextSplit;
    Overview m_overview;
    Icons m_icons;
};

VirtualFunctionAssistProvider::VirtualFunctionAssistProvider()
    : m_function(0)
    , m_openInNextSplit(false)
{
}

bool VirtualFunctionAssistProvider::configure(Class *startClass, Function *function,
                                              const Snapshot &snapshot, bool openInNextSplit)
{
    m_startClass = startClass;
    m_function = function;
    m_snapshot = snapshot;
    m_openInNextSplit = openInNextSplit;
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
    return new VirtualFunctionsAssistProcessor(this);
}

enum VirtualType { Virtual, PureVirtual };

static bool isVirtualFunction_helper(Function *function, const Snapshot &snapshot,
                                     VirtualType virtualType)
{
    if (!function)
        return false;

    if ((virtualType == Virtual && function->isVirtual())
            || (virtualType == PureVirtual && function->isPureVirtual())) {
        return true;
    }

    const QString filePath = QString::fromUtf8(function->fileName(), function->fileNameLength());
    if (Document::Ptr document = snapshot.document(filePath)) {
        LookupContext context(document, snapshot);
        QList<LookupItem> results = context.lookup(function->name(), function->enclosingScope());
        if (!results.isEmpty()) {
            foreach (const LookupItem &item, results) {
                if (Symbol *symbol = item.declaration()) {
                    if (Function *functionType = symbol->type()->asFunctionType()) {
                        if (!functionType) {
                            if (Template *t = item.type()->asTemplateType())
                                if ((symbol = t->declaration()))
                                    functionType = symbol->type()->asFunctionType();
                        }
                        const bool foundSuitable = virtualType == Virtual
                            ? functionType->isVirtual()
                            : functionType->isPureVirtual();
                        if (foundSuitable)
                            return true;
                    }
                }
            }
        }
    }

    return false;
}

bool FunctionHelper::isVirtualFunction(Function *function, const Snapshot &snapshot)
{
    return isVirtualFunction_helper(function, snapshot, Virtual);
}

bool FunctionHelper::isPureVirtualFunction(Function *function, const Snapshot &snapshot)
{
    return isVirtualFunction_helper(function, snapshot, PureVirtual);
}

QList<Symbol *> FunctionHelper::overrides(Class *startClass, Function *function,
                                           const Snapshot &snapshot)
{
    QList<Symbol *> result;
    QTC_ASSERT(function && startClass, return result);

    FullySpecifiedType referenceType = function->type();
    const Name *referenceName = function->name();

    // Add itself
    result << function;

    // Find overrides
    CppEditor::Internal::CppClass cppClass = CppClass(startClass);
    cppClass.lookupDerived(startClass, snapshot);

    QList<CppClass> l;
    l << cppClass;

    while (!l.isEmpty()) {
        // Add derived
        CppClass clazz = l.takeFirst();
        foreach (const CppClass &d, clazz.derived) {
            if (!l.contains(d))
                l << d;
        }

        // Check member functions
        QTC_ASSERT(clazz.declaration, continue);
        Class *c = clazz.declaration->asClass();
        QTC_ASSERT(c, continue);
        for (int i = 0, total = c->memberCount(); i < total; ++i) {
            Symbol *candidate = c->memberAt(i);
            const Name *candidateName = candidate->name();
            FullySpecifiedType candidateType = candidate->type();
            if (candidateName->isEqualTo(referenceName) && candidateType.isEqualTo(referenceType))
                result << candidate;
        }
    }

    return result;
}
