// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>

#include <QObject>

#include <unordered_map>

namespace Core { class IEditor; }

namespace Nim::Suggest {

class NimSuggest;

class NimSuggestCache final : public QObject
{
public:
    static NimSuggestCache &instance();

    NimSuggest *get(const Utils::FilePath &filename);

    Utils::FilePath executablePath() const;
    void setExecutablePath(const Utils::FilePath &path);

private:
    NimSuggestCache();
    ~NimSuggestCache();

    void onEditorOpened(Core::IEditor *editor);
    void onEditorClosed(Core::IEditor *editor);

    std::unordered_map<Utils::FilePath, std::unique_ptr<Suggest::NimSuggest>> m_nimSuggestInstances;

    Utils::FilePath m_executablePath;
};

} // Nim::Suggest
