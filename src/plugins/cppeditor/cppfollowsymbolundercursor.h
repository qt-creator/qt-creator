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


#ifndef CPPFOLLOWSYMBOLUNDERCURSOR_H
#define CPPFOLLOWSYMBOLUNDERCURSOR_H

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

#endif // CPPFOLLOWSYMBOLUNDERCURSOR_H
