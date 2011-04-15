/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "cppquickfixassistant.h"
#include "cppeditorconstants.h"
#include "cppeditor.h"

// @TODO: temp
#include "cppquickfix.h"

#include <AST.h>
#include <TranslationUnit.h>
#include <Token.h>

#include <cplusplus/ASTPath.h>
#include <cplusplus/CppDocument.h>
#include <cplusplus/ResolveExpression.h>
#include <cplusplus/Overview.h>
#include <cplusplus/TypeOfExpression.h>
#include <cplusplus/DependencyTable.h>
#include <cplusplus/CppRewriter.h>

#include <cpptools/cpprefactoringchanges.h>

#include <extensionsystem/pluginmanager.h>

#include <QtCore/QFileInfo>
#include <QtGui/QTextBlock>

using namespace CppEditor;
using namespace CppEditor::Internal;
using namespace TextEditor;
using namespace CppTools;
using namespace CPlusPlus;

// -------------------------
// CppQuickFixAssistProvider
// -------------------------
bool CppQuickFixAssistProvider::supportsEditor(const QString &editorId) const
{
    return editorId == QLatin1String(CppEditor::Constants::CPPEDITOR_ID);
}

IAssistProcessor *CppQuickFixAssistProvider::createProcessor() const
{
    return new CppQuickFixAssistProcessor(this);
}

QList<TextEditor::QuickFixFactory *> CppQuickFixAssistProvider::quickFixFactories() const
{
    QList<TextEditor::QuickFixFactory *> results;
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    foreach (CppQuickFixFactory *f, pm->getObjects<CppEditor::CppQuickFixFactory>())
        results.append(f);
    return results;
}

// --------------------------
// CppQuickFixAssistProcessor
// --------------------------
CppQuickFixAssistProcessor::CppQuickFixAssistProcessor(const IAssistProvider *provider)
    : m_provider(provider)
{}

const IAssistProvider *CppQuickFixAssistProcessor::provider() const
{
    return m_provider;
}

// --------------------------
// CppQuickFixAssistInterface
// --------------------------
CppQuickFixAssistInterface::CppQuickFixAssistInterface(CPPEditorWidget *editor,
                                                       TextEditor::AssistReason reason)
    : DefaultAssistInterface(editor->document(), editor->position(), editor->file(), reason)
    , m_editor(editor)
    , m_semanticInfo(editor->semanticInfo())
    , m_snapshot(CPlusPlus::CppModelManagerInterface::instance()->snapshot())
    , m_context(m_semanticInfo.doc, m_snapshot)
{
    CPlusPlus::ASTPath astPath(m_semanticInfo.doc);
    m_path = astPath(editor->textCursor());
}

const QList<AST *> &CppQuickFixAssistInterface::path() const
{
    return m_path;
}

Snapshot CppQuickFixAssistInterface::snapshot() const
{
    return m_snapshot;
}

SemanticInfo CppQuickFixAssistInterface::semanticInfo() const
{
    return m_semanticInfo;
}

const LookupContext &CppQuickFixAssistInterface::context() const
{
    return m_context;
}

CPPEditorWidget *CppQuickFixAssistInterface::editor() const
{
    return m_editor;
}

const CppRefactoringFile CppQuickFixAssistInterface::currentFile() const
{
    return CppRefactoringFile(m_editor, m_semanticInfo.doc);
}

bool CppQuickFixAssistInterface::isCursorOn(unsigned tokenIndex) const
{
    return currentFile().isCursorOn(tokenIndex);
}

bool CppQuickFixAssistInterface::isCursorOn(const CPlusPlus::AST *ast) const
{
    return currentFile().isCursorOn(ast);
}
