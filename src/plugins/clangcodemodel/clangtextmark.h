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

#include <clangsupport_global.h>
#include <clangsupport/diagnosticcontainer.h>

#include <languageserverprotocol/lsptypes.h>

#include <texteditor/textmark.h>

#include <functional>

namespace LanguageClient { class Client; }

namespace ClangCodeModel {
namespace Internal {
class ClangDiagnosticManager;

class ClangTextMark : public TextEditor::TextMark
{
public:
    using RemovedFromEditorHandler = std::function<void(ClangTextMark *)>;

    ClangTextMark(const ::Utils::FilePath &fileName,
                  const ClangBackEnd::DiagnosticContainer &diagnostic,
                  const RemovedFromEditorHandler &removedHandler,
                  bool fullVisualization,
                  const ClangDiagnosticManager *diagMgr);

    ClangBackEnd::DiagnosticContainer diagnostic() const { return m_diagnostic; }
    void updateIcon(bool valid = true);

private:
    bool addToolTipContent(QLayout *target) const override;
    void removedFromEditor() override;

private:
    ClangBackEnd::DiagnosticContainer m_diagnostic;
    RemovedFromEditorHandler m_removedFromEditorHandler;
    const ClangDiagnosticManager * const m_diagMgr;
};

class ClangdTextMark : public TextEditor::TextMark
{
    Q_DECLARE_TR_FUNCTIONS(ClangdTextMark)
public:
    ClangdTextMark(const ::Utils::FilePath &filePath,
                   const LanguageServerProtocol::Diagnostic &diagnostic,
                   const LanguageClient::Client *client);

private:
    bool addToolTipContent(QLayout *target) const override;

    const LanguageServerProtocol::Diagnostic m_lspDiagnostic;
    const ClangBackEnd::DiagnosticContainer m_diagnostic;
    const LanguageClient::Client * const m_client;
};

} // namespace Internal
} // namespace ClangCodeModel
