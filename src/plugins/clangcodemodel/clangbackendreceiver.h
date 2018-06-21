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

#include <cpptools/cppcursorinfo.h>
#include <cpptools/cppsymbolinfo.h>
#include <cpptools/baseeditordocumentprocessor.h>

#include <clangsupport/clangcodemodelclientinterface.h>

#include <QFuture>
#include <QPointer>
#include <QTextDocument>

namespace TextEditor { class TextEditorWidget; }

namespace ClangCodeModel {
namespace Internal {

class ClangCompletionAssistProcessor;

class BackendReceiver : public ClangBackEnd::ClangCodeModelClientInterface
{
public:
    BackendReceiver();
    ~BackendReceiver();

    using AliveHandler = std::function<void ()>;
    void setAliveHandler(const AliveHandler &handler);

    void addExpectedCompletionsMessage(quint64 ticket, ClangCompletionAssistProcessor *processor);
    void deleteProcessorsOfEditorWidget(TextEditor::TextEditorWidget *textEditorWidget);

    QFuture<CppTools::CursorInfo>
    addExpectedReferencesMessage(quint64 ticket,
                                 const CppTools::SemanticInfo::LocalUseMap &localUses
                                     = CppTools::SemanticInfo::LocalUseMap());
    QFuture<CppTools::SymbolInfo> addExpectedRequestFollowSymbolMessage(quint64 ticket);
    QFuture<CppTools::ToolTipInfo> addExpectedToolTipMessage(quint64 ticket);
    bool isExpectingCompletionsMessage() const;

    void reset();

private:
    void alive() override;
    void echo(const ClangBackEnd::EchoMessage &message) override;
    void completions(const ClangBackEnd::CompletionsMessage &message) override;

    void annotations(const ClangBackEnd::AnnotationsMessage &message) override;
    void references(const ClangBackEnd::ReferencesMessage &message) override;
    void tooltip(const ClangBackEnd::ToolTipMessage &message) override;
    void followSymbol(const ClangBackEnd::FollowSymbolMessage &message) override;

private:
    AliveHandler m_aliveHandler;
    QHash<quint64, ClangCompletionAssistProcessor *> m_assistProcessorsTable;

    struct ReferencesEntry {
        ReferencesEntry() = default;
        ReferencesEntry(QFutureInterface<CppTools::CursorInfo> futureInterface,
                        const CppTools::SemanticInfo::LocalUseMap &localUses)
            : futureInterface(futureInterface)
            , localUses(localUses) {}
        QFutureInterface<CppTools::CursorInfo> futureInterface;
        CppTools::SemanticInfo::LocalUseMap localUses;
    };
    QHash<quint64, ReferencesEntry> m_referencesTable;
    QHash<quint64, QFutureInterface<CppTools::ToolTipInfo>> m_toolTipsTable;
    QHash<quint64, QFutureInterface<CppTools::SymbolInfo>> m_followTable;
};

} // namespace Internal
} // namespace ClangCodeModel
