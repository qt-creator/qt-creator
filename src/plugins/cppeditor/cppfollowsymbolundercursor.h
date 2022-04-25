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

#include "cppeditor_global.h"
#include "cursorineditor.h"

#include <cplusplus/CppDocument.h>

#include <texteditor/texteditor.h>

namespace CppEditor {
class SymbolFinder;
class VirtualFunctionAssistProvider;

class CPPEDITOR_EXPORT FollowSymbolUnderCursor
{
public:
    FollowSymbolUnderCursor();

    void findLink(const CursorInEditor &data,
                  Utils::ProcessLinkCallback &&processLinkCallback,
                  bool resolveTarget,
                  const CPlusPlus::Snapshot &snapshot,
                  const CPlusPlus::Document::Ptr &documentFromSemanticInfo,
                  SymbolFinder *symbolFinder,
                  bool inNextSplit);

    void switchDeclDef(const CursorInEditor &data,
                       Utils::ProcessLinkCallback &&processLinkCallback,
                       const CPlusPlus::Snapshot &snapshot,
                       const CPlusPlus::Document::Ptr &documentFromSemanticInfo,
                       SymbolFinder *symbolFinder);

    QSharedPointer<VirtualFunctionAssistProvider> virtualFunctionAssistProvider();
    void setVirtualFunctionAssistProvider(
            const QSharedPointer<VirtualFunctionAssistProvider> &provider);

private:
    QSharedPointer<VirtualFunctionAssistProvider> m_virtualFunctionAssistProvider;
};

} // namespace CppEditor
