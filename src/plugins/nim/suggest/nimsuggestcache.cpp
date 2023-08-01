// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "nimsuggestcache.h"

#include "nimconstants.h"
#include "nimsuggest.h"
#include "settings/nimsettings.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>

#include <unordered_map>

using namespace Utils;

namespace Nim::Suggest {

class NimSuggestCache final : public QObject
{
public:
    NimSuggestCache()
    {
        setExecutablePath(settings().nimSuggestPath());
        QObject::connect(&settings().nimSuggestPath, &StringAspect::changed, this, [this] {
            setExecutablePath(settings().nimSuggestPath());
        });

        Core::EditorManager *editorManager = Core::EditorManager::instance();
        connect(editorManager, &Core::EditorManager::editorOpened,
                this, &NimSuggestCache::onEditorOpened);
        connect(editorManager, &Core::EditorManager::editorAboutToClose,
                this, &NimSuggestCache::onEditorClosed);
    }

    void setExecutablePath(const FilePath &path)
    {
        if (m_executablePath == path)
            return;

        m_executablePath = path;

        for (const auto &pair : m_nimSuggestInstances)
            pair.second->setExecutablePath(path);
    }

    void onEditorOpened(Core::IEditor *editor)
    {
        if (editor->document()->mimeType() == Constants::C_NIM_MIMETYPE)
            getFromCache(editor->document()->filePath());
    }

    void onEditorClosed(Core::IEditor *editor)
    {
        auto it = m_nimSuggestInstances.find(editor->document()->filePath());
        if (it != m_nimSuggestInstances.end())
            m_nimSuggestInstances.erase(it);
    }

    NimSuggest *get(const FilePath &filePath)
    {
        auto it = m_nimSuggestInstances.find(filePath);
        if (it == m_nimSuggestInstances.end()) {
            auto instance = std::make_unique<NimSuggest>(this);
            instance->setProjectFile(filePath);
            instance->setExecutablePath(m_executablePath);
            it = m_nimSuggestInstances.emplace(filePath, std::move(instance)).first;
        }
        return it->second.get();
    }

    std::unordered_map<FilePath, std::unique_ptr<Suggest::NimSuggest>> m_nimSuggestInstances;

    FilePath m_executablePath;
};

static NimSuggestCache &cache()
{
    static NimSuggestCache theCache;
    return theCache;
}

NimSuggest *getFromCache(const FilePath &filePath)
{
    return cache().get(filePath);
}

} // Nim::Suggest
