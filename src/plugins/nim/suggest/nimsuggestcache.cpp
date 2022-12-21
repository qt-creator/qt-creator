// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
