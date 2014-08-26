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


#include "cppeditortestcase.h"

#include "cppeditor.h"
#include "cppeditordocument.h"

#include <coreplugin/editormanager/editormanager.h>
#include <cpptools/cppsemanticinfo.h>
#include <cplusplus/CppDocument.h>

#include <QDir>

using namespace CppEditor::Internal;

namespace CppEditor {
namespace Internal {
namespace Tests {

TestDocument::TestDocument(const QByteArray &fileName, const QByteArray &source, char cursorMarker)
    : CppTools::Tests::TestDocument(fileName, source, cursorMarker)
    , m_cursorPosition(m_source.indexOf(QLatin1Char(m_cursorMarker)))
    , m_editor(0)
    , m_editorWidget(0)
{
}

bool TestDocument::hasCursorMarker() const { return m_cursorPosition != -1; }

TestCase::TestCase(bool runGarbageCollector)
    : CppTools::Tests::TestCase(runGarbageCollector)
{
}

TestCase::~TestCase()
{
}

bool TestCase::openCppEditor(const QString &fileName, CppEditor **editor, CppEditorWidget **editorWidget)
{
    if (CppEditor *e = dynamic_cast<CppEditor *>(Core::EditorManager::openEditor(fileName))) {
        if (editor)
            *editor = e;
        if (editorWidget) {
            if (CppEditorWidget *w = dynamic_cast<CppEditorWidget *>(e->editorWidget())) {
                *editorWidget = w;
                return true;
            } else {
                return false; // no or wrong widget
            }
        } else {
            return true; // ok since no widget requested
        }
    } else {
        return false; // no or wrong editor
    }
}

CPlusPlus::Document::Ptr TestCase::waitForRehighlightedSemanticDocument(
        CppEditorWidget *editorWidget)
{
    while (!editorWidget->isSemanticInfoValid())
        QCoreApplication::processEvents();
    return editorWidget->semanticInfo().doc;
}

} // namespace Tests
} // namespace Internal
} // namespace CppEditor
