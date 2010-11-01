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

#include "cppquickfixcollector.h"
#include "cppeditor.h"

#include <extensionsystem/pluginmanager.h>

#include <cpptools/cpprefactoringchanges.h>
#include <cpptools/cpptoolsconstants.h>
#include <cpptools/cppmodelmanagerinterface.h>

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

bool CppQuickFixCollector::supportsEditor(TextEditor::ITextEditable *editor)
{
    return CppTools::CppModelManagerInterface::instance()->isCppEditor(editor);
}

TextEditor::QuickFixState *CppQuickFixCollector::initializeCompletion(TextEditor::BaseTextEditor *editor)
{
    if (CPPEditor *cppEditor = qobject_cast<CPPEditor *>(editor)) {
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
                state->_snapshot = CppTools::CppModelManagerInterface::instance()->snapshot();
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
