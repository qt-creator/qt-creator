// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
                  const Utils::LinkHandler &processLinkCallback,
                  bool resolveTarget,
                  const CPlusPlus::Snapshot &snapshot,
                  const CPlusPlus::Document::Ptr &documentFromSemanticInfo,
                  SymbolFinder *symbolFinder,
                  bool inNextSplit);

    void switchDeclDef(const CursorInEditor &data,
                       const Utils::LinkHandler &processLinkCallback,
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
