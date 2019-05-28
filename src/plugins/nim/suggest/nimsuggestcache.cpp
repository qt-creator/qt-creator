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

#include "nimsuggestcache.h"

#include "nimconstants.h"
#include "nimsuggest.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>

namespace Nim {
namespace Suggest {

NimSuggestCache &NimSuggestCache::instance()
{
    static NimSuggestCache instance;
    return instance;
}

NimSuggestCache::~NimSuggestCache() = default;

NimSuggest *NimSuggestCache::get(const Utils::FilePath &filename)
{
    auto it = m_nimSuggestInstances.find(filename);
    if (it == m_nimSuggestInstances.end()) {
        auto instance = std::make_unique<Suggest::NimSuggest>(this);
        instance->setProjectFile(filename.toString());
        instance->setExecutablePath(m_executablePath);
        it = m_nimSuggestInstances.emplace(filename, std::move(instance)).first;
    }
    return it->second.get();
}

NimSuggestCache::NimSuggestCache()
{
    Core::EditorManager *editorManager = Core::EditorManager::instance();
    connect(editorManager, &Core::EditorManager::editorOpened,
            this, &NimSuggestCache::onEditorOpened);
    connect(editorManager, &Core::EditorManager::editorAboutToClose,
            this, &NimSuggestCache::onEditorClosed);
}

QString NimSuggestCache::executablePath() const
{
    return m_executablePath;
}

void NimSuggestCache::setExecutablePath(const QString &path)
{
    if (m_executablePath == path)
        return;

    m_executablePath = path;

    for (const auto &pair : m_nimSuggestInstances) {
        pair.second->setExecutablePath(path);
    }
}

void Nim::Suggest::NimSuggestCache::onEditorOpened(Core::IEditor *editor)
{
    if (editor->document()->mimeType() == Constants::C_NIM_MIMETYPE) {
        get(editor->document()->filePath());
    }
}

void Nim::Suggest::NimSuggestCache::onEditorClosed(Core::IEditor *editor)
{
    auto it = m_nimSuggestInstances.find(editor->document()->filePath());
    if (it != m_nimSuggestInstances.end())
        m_nimSuggestInstances.erase(it);
}

}
}
