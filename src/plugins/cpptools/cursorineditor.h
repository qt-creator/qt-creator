/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "cppeditorwidgetinterface.h"

#include <utils/fileutils.h>

#include <QTextCursor>

namespace CppTools {

class CursorInEditor
{
public:
    CursorInEditor(const QTextCursor &cursor, const Utils::FileName &filePath,
                 CppEditorWidgetInterface *editorWidget = nullptr)
        : m_cursor(cursor)
        , m_filePath(filePath)
        , m_editorWidget(editorWidget)
    {}
    CppEditorWidgetInterface *editorWidget() const { return m_editorWidget; }
    const QTextCursor &cursor() const { return m_cursor; }
    const Utils::FileName &filePath() const { return m_filePath; }
private:
    QTextCursor m_cursor;
    Utils::FileName m_filePath;
    CppEditorWidgetInterface *m_editorWidget = nullptr;
};

} // namespace CppTools
