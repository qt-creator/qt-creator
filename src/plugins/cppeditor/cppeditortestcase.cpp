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

#include "cppeditortestcase.h"

#include "cppeditor.h"
#include "cppeditorwidget.h"
#include "cppeditordocument.h"

#include <coreplugin/editormanager/editormanager.h>
#include <cpptools/cppsemanticinfo.h>
#include <cplusplus/CppDocument.h>

#include <QDir>

#include <utils/qtcassert.h>

using namespace CppEditor::Internal;

namespace CppEditor {
namespace Internal {
namespace Tests {

TestDocument::TestDocument(const QByteArray &fileName, const QByteArray &source, char cursorMarker)
    : CppTools::Tests::TestDocument(fileName, source, cursorMarker)
    , m_cursorPosition(-1)
    , m_anchorPosition(-1)
    , m_selectionStartMarker(QLatin1Char(m_cursorMarker) + QLatin1String("{start}"))
    , m_selectionEndMarker(QLatin1Char(m_cursorMarker) + QLatin1String("{end}"))
    , m_editor(0)
    , m_editorWidget(0)
{
    // Try to find selection markers
    const int selectionStartIndex = m_source.indexOf(m_selectionStartMarker);
    const int selectionEndIndex = m_source.indexOf(m_selectionEndMarker);
    const bool bothSelectionMarkersFound = selectionStartIndex != -1 && selectionEndIndex != -1;
    const bool noneSelectionMarkersFounds = selectionStartIndex == -1 && selectionEndIndex == -1;
    QTC_ASSERT(bothSelectionMarkersFound || noneSelectionMarkersFounds, return);

    if (selectionStartIndex != -1) {
        m_cursorPosition = selectionEndIndex;
        m_anchorPosition = selectionStartIndex;

    // No selection markers found, so check for simple cursorMarker
    } else {
        m_cursorPosition = m_source.indexOf(QLatin1Char(cursorMarker));
    }
}

bool TestDocument::hasCursorMarker() const { return m_cursorPosition != -1; }

bool TestDocument::hasAnchorMarker() const { return m_anchorPosition != -1; }

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
