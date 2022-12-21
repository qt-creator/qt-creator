// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/fileutils.h>

#include <QObject>

#include <unordered_map>

namespace Core { class IEditor; }

namespace Nim {
namespace Suggest {

class NimSuggest;

class NimSuggestCache : public QObject
{
    Q_OBJECT

public:
    static NimSuggestCache &instance();

    NimSuggest *get(const Utils::FilePath &filename);

    QString executablePath() const;
    void setExecutablePath(const QString &path);

private:
    NimSuggestCache();
    ~NimSuggestCache();

    void onEditorOpened(Core::IEditor *editor);
    void onEditorClosed(Core::IEditor *editor);

    std::unordered_map<Utils::FilePath, std::unique_ptr<Suggest::NimSuggest>> m_nimSuggestInstances;

    QString m_executablePath;
};

} // namespace Suggest
} // namespace Nim
