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

#include <utils/link.h>

#include <QSharedPointer>
#include <QString>

#include <memory>

namespace TextEditor {
class TextDocument;
class BaseHoverHandler;
} // namespace TextEditor

namespace CppEditor {

class AbstractOverviewModel;
class BaseEditorDocumentProcessor;
class CppCompletionAssistProvider;
class CursorInEditor;
class RefactoringEngineInterface;

class CPPEDITOR_EXPORT ModelManagerSupport
{
public:
    using Ptr = QSharedPointer<ModelManagerSupport>;

public:
    virtual ~ModelManagerSupport() = 0;

    virtual CppCompletionAssistProvider *completionAssistProvider() = 0;
    virtual CppCompletionAssistProvider *functionHintAssistProvider() = 0;
    virtual TextEditor::BaseHoverHandler *createHoverHandler() = 0;
    virtual BaseEditorDocumentProcessor *createEditorDocumentProcessor(
                TextEditor::TextDocument *baseTextDocument) = 0;
    virtual RefactoringEngineInterface &refactoringEngineInterface() = 0;
    virtual std::unique_ptr<AbstractOverviewModel> createOverviewModel() = 0;
    virtual bool supportsOutline(const TextEditor::TextDocument *) const { return true; }
    virtual bool supportsLocalUses(const TextEditor::TextDocument *) const { return true; }

    virtual void followSymbol(const CursorInEditor &data,
                              Utils::ProcessLinkCallback &&processLinkCallback,
                              bool resolveTarget, bool inNextSplit) = 0;
    virtual void switchDeclDef(const CursorInEditor &data,
                               Utils::ProcessLinkCallback &&processLinkCallback) = 0;
};

class CPPEDITOR_EXPORT ModelManagerSupportProvider
{
public:
    virtual ~ModelManagerSupportProvider() = default;

    virtual QString id() const = 0;
    virtual QString displayName() const = 0;

    virtual ModelManagerSupport::Ptr createModelManagerSupport() = 0;
};

} // CppEditor namespace
