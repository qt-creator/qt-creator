/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "cppquickfixcollector.h"
#include "cppeditor.h"

#include <extensionsystem/pluginmanager.h>

#include <cplusplus/ModelManagerInterface.h>
#include <cpptools/cpprefactoringchanges.h>
#include <cpptools/cpptoolsconstants.h>

#include <AST.h>
#include <cplusplus/ASTPath.h>

namespace CppEditor {
namespace Internal {

CppQuickFixCollector::CppQuickFixCollector()
{
}

CppQuickFixCollector::~CppQuickFixCollector()
{
}

bool CppQuickFixCollector::supportsEditor(TextEditor::ITextEditor *editor) const
{
    return CPlusPlus::CppModelManagerInterface::instance()->isCppEditor(editor);
}

TextEditor::QuickFixState *CppQuickFixCollector::initializeCompletion(TextEditor::BaseTextEditorWidget *editor)
{
    if (CPPEditorWidget *cppEditor = qobject_cast<CPPEditorWidget *>(editor)) {
        const SemanticInfo info = cppEditor->semanticInfo();

        if (info.revision != cppEditor->editorRevision()) {
            // outdated
            qWarning() << "TODO: outdated semantic info, force a reparse.";
            return 0;
        }

        if (info.doc) {
            CPlusPlus::ASTPath astPath(info.doc);

            const QList<CPlusPlus::AST *> path = astPath(cppEditor->textCursor());
            if (! path.isEmpty()) {
                CppQuickFixState *state = new CppQuickFixState(editor);
                state->_path = path;
                state->_semanticInfo = info;
                state->_snapshot = CPlusPlus::CppModelManagerInterface::instance()->snapshot();
                state->_context = CPlusPlus::LookupContext(info.doc, state->snapshot());
                return state;
            }
        }
    }

    return 0;
}

QList<TextEditor::QuickFixFactory *> CppQuickFixCollector::quickFixFactories() const
{
    QList<TextEditor::QuickFixFactory *> results;
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    foreach (CppQuickFixFactory *f, pm->getObjects<CppEditor::CppQuickFixFactory>())
        results.append(f);
    return results;
}

} // namespace Internal
} // namespace CppEditor
