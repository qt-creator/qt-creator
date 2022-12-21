// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/texteditor.h>

namespace Nim {
namespace Suggest { class NimSuggestClientRequest; }

class NimTextEditorWidget : public TextEditor::TextEditorWidget
{
public:
    NimTextEditorWidget(QWidget* parent = nullptr);

protected:
    void findLinkAt(const QTextCursor &, const Utils::LinkHandler &processLinkCallback,
                    bool resolveTarget, bool inNextSplit);

private:
    void onFindLinkFinished(Suggest::NimSuggestClientRequest *request);

    std::shared_ptr<Nim::Suggest::NimSuggestClientRequest> m_request;
    Utils::LinkHandler m_callback;
    std::unique_ptr<QTemporaryFile> m_dirtyFile;
};

}
