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

#pragma once

#include <cpptools/cpptoolstestcase.h>

#include <QVector>

namespace CppEditor {
namespace Internal {

class CppEditor;
class CppEditorWidget;

namespace Tests {

class TestDocument : public CppTools::Tests::TestDocument
{
public:
    TestDocument(const QByteArray &fileName, const QByteArray &source, char cursorMarker = '@');

    bool hasCursorMarker() const;
    bool hasAnchorMarker() const;

public:
    int m_cursorPosition;
    int m_anchorPosition;
    QString m_selectionStartMarker;
    QString m_selectionEndMarker;
    CppEditor *m_editor;
    CppEditorWidget *m_editorWidget;
};

using TestDocuments = QVector<TestDocument>;

class TestCase : public CppTools::Tests::TestCase
{
public:
    TestCase(bool runGarbageCollector = true);
    ~TestCase();

    static bool openCppEditor(const QString &fileName,
                              CppEditor **editor,
                              CppEditorWidget **editorWidget = 0);

    static CPlusPlus::Document::Ptr waitForRehighlightedSemanticDocument(
            CppEditorWidget *editorWidget);
};

} // namespace Tests
} // namespace Internal
} // namespace CppEditor
