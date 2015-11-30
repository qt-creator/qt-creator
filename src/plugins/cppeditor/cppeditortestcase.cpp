/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
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
