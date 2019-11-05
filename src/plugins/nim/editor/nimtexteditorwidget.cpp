/****************************************************************************
**
** Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
** Contact: http://www.qt.io/licensing
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

using namespace Nim;
using namespace Suggest;

namespace {

std::unique_ptr<QTemporaryFile> writeDirtyFile(const TextEditor::TextDocument *doc)
{
    auto result = std::make_unique<QTemporaryFile>("qtcnim.XXXXXX.nim");
    QTC_ASSERT(result->open(), return nullptr);
    QTextStream stream(result.get());
    stream << doc->plainText();
    result->close();
    return result;
}

}

NimTextEditorWidget::NimTextEditorWidget(QWidget *parent)
    : TextEditorWidget(parent)
{
    setLanguageSettingsId(Nim::Constants::C_NIMLANGUAGE_ID);
}

void NimTextEditorWidget::findLinkAt(const QTextCursor &c, Utils::ProcessLinkCallback &&processLinkCallback, bool /*resolveTarget*/, bool /*inNextSplit*/)
{
    const Utils::FilePath &path = textDocument()->filePath();

    NimSuggest *suggest = NimSuggestCache::instance().get(path);
    if (!suggest)
        return processLinkCallback(Utils::Link());

    std::unique_ptr<QTemporaryFile> dirtyFile = writeDirtyFile(textDocument());

    int line = 0, column = 0;
    Utils::Text::convertPosition(document(), c.position(), &line, &column);

    std::shared_ptr<NimSuggestClientRequest> request = suggest->def(path.toString(),
                                                                             line,
                                                                             column - 1,
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
    m_callback = std::move(processLinkCallback);
    m_request = std::move(request);

    QObject::connect(m_request.get(), &NimSuggestClientRequest::finished, this, &NimTextEditorWidget::onFindLinkFinished);
}

void NimTextEditorWidget::onFindLinkFinished()
{
    QTC_ASSERT(m_request.get() == this->sender(), return);
    if (m_request->lines().empty()) {
        m_callback(Utils::Link());
        return;
    }

    const Line &line = m_request->lines().front();
    m_callback(Utils::Link{line.abs_path, line.row, line.column});
}
