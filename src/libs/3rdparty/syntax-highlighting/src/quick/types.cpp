/*
    SPDX-FileCopyrightText: 2018 Eike Hein <hein@kde.org>
    SPDX-FileCopyrightText: 2021 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: MIT
*/

#include "types.h"

#include <QJSEngine>

KSyntaxHighlighting::Repository *defaultRepository() // also used by KQuickSyntaxHighlighter
{
    static std::unique_ptr<KSyntaxHighlighting::Repository> s_instance;
    if (!s_instance) {
        s_instance = std::make_unique<KSyntaxHighlighting::Repository>();
    }
    return s_instance.get();
}

KSyntaxHighlighting::Repository *RepositoryForeign::create([[maybe_unused]] QQmlEngine *engine, QJSEngine *scriptEngine)
{
    auto repo = defaultRepository();
    scriptEngine->setObjectOwnership(repo, QJSEngine::CppOwnership);
    return repo;
}

#include "moc_types.cpp"
