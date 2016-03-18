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

#include <cplusplus/CppDocument.h>
#include <texteditor/texteditor.h>

QT_BEGIN_NAMESPACE
class QTextCursor;
QT_END_NAMESPACE

namespace CppTools { class SymbolFinder; }

namespace CppEditor {
namespace Internal {

class CppEditorWidget;
class VirtualFunctionAssistProvider;

class FollowSymbolUnderCursor
{
public:
    typedef TextEditor::TextEditorWidget::Link Link;

    FollowSymbolUnderCursor(CppEditorWidget *widget);
    ~FollowSymbolUnderCursor();

    Link findLink(const QTextCursor &cursor, bool resolveTarget,
                  const CPlusPlus::Snapshot &snapshot,
                  const CPlusPlus::Document::Ptr &documentFromSemanticInfo,
                  CppTools::SymbolFinder *symbolFinder, bool inNextSplit);

    VirtualFunctionAssistProvider *virtualFunctionAssistProvider();
    void setVirtualFunctionAssistProvider(VirtualFunctionAssistProvider *provider);

private:
    CppEditorWidget *m_widget;
    VirtualFunctionAssistProvider *m_virtualFunctionAssistProvider;
};

} // namespace Internal
} // namespace CppEditor
