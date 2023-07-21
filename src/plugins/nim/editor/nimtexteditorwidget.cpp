// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "nimtexteditorwidget.h"
#include "nimconstants.h"
#include "suggest/nimsuggestcache.h"
#include "suggest/nimsuggest.h"

#include <texteditor/textdocument.h>
#include <texteditor/codeassist/assistinterface.h>
#include <utils/qtcassert.h>
#include <utils/textutils.h>

#include <QTextStream>
#include <QTemporaryFile>
#include <QTextDocument>

using namespace Nim::Suggest;

namespace Nim {

static std::unique_ptr<QTemporaryFile> writeDirtyFile(const TextEditor::TextDocument *doc)
{
    auto result = std::make_unique<QTemporaryFile>("qtcnim.XXXXXX.nim");
    QTC_ASSERT(result->open(), return nullptr);
    QTextStream stream(result.get());
    stream << doc->plainText();
    result->close();
    return result;
}

NimTextEditorWidget::NimTextEditorWidget(QWidget *parent)
    : TextEditorWidget(parent)
{
    setLanguageSettingsId(Nim::Constants::C_NIMLANGUAGE_ID);
}

void NimTextEditorWidget::findLinkAt(const QTextCursor &c, const Utils::LinkHandler &processLinkCallback, bool /*resolveTarget*/, bool /*inNextSplit*/)
{
    const Utils::FilePath &path = textDocument()->filePath();

    NimSuggest *suggest = Nim::Suggest::getFromCache(path);
    if (!suggest)
        return processLinkCallback(Utils::Link());

    std::unique_ptr<QTemporaryFile> dirtyFile = writeDirtyFile(textDocument());

    int line = 0, column = 0;
    Utils::Text::convertPosition(document(), c.position(), &line, &column);

    std::shared_ptr<NimSuggestClientRequest> request = suggest->def(path.toString(),
                                                                             line,
                                                                             column,
                                                                             dirtyFile->fileName());

    if (!request)
        return processLinkCallback(Utils::Link());

    if (m_request) {
        QObject::disconnect(m_request.get());
        m_request = nullptr;
    }

    if (m_callback)
        m_callback(Utils::Link());

    m_dirtyFile = std::move(dirtyFile);
    m_callback = processLinkCallback;
    m_request = std::move(request);

    Suggest::NimSuggestClientRequest *req = m_request.get();
    connect(req, &NimSuggestClientRequest::finished,
            this, [this, req] { onFindLinkFinished(req); });
}

void NimTextEditorWidget::onFindLinkFinished(Suggest::NimSuggestClientRequest *request)
{
    QTC_ASSERT(m_request.get() == request, return);
    if (m_request->lines().empty()) {
        m_callback(Utils::Link());
        return;
    }

    const Line &line = m_request->lines().front();
    m_callback(Utils::Link{Utils::FilePath::fromString(line.abs_path), line.row, line.column});
}

} // Nim
