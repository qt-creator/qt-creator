/*
    SPDX-FileCopyrightText: 2018 Eike Hein <hein@kde.org>
    SPDX-FileCopyrightText: 2021 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: MIT
*/

#include "kquicksyntaxhighlightingplugin.h"
#include "kquicksyntaxhighlighter.h"
#include "repositorywrapper.h"

#include <KSyntaxHighlighting/Definition>
#include <KSyntaxHighlighting/Repository>
#include <KSyntaxHighlighting/Theme>

#include <memory>

using namespace KSyntaxHighlighting;

Repository *defaultRepository()
{
    static std::unique_ptr<Repository> s_instance;
    if (!s_instance) {
        s_instance = std::make_unique<Repository>();
    }
    return s_instance.get();
}

void KQuickSyntaxHighlightingPlugin::registerTypes(const char *uri)
{
    Q_ASSERT(QLatin1String(uri) == QLatin1String("org.kde.syntaxhighlighting"));
    qRegisterMetaType<Definition>();
    qRegisterMetaType<QVector<Definition>>();
    qRegisterMetaType<Theme>();
    qRegisterMetaType<QVector<Theme>>();
    qmlRegisterType<KQuickSyntaxHighlighter>(uri, 1, 0, "SyntaxHighlighter");
    qmlRegisterUncreatableType<Definition>(uri, 1, 0, "Definition", {});
    qmlRegisterUncreatableType<Theme>(uri, 1, 0, "Theme", {});
    qmlRegisterSingletonType<RepositoryWrapper>(uri, 1, 0, "Repository", [](auto engine, auto scriptEngine) {
        (void)engine;
        (void)scriptEngine;
        auto repo = new RepositoryWrapper;
        repo->m_repository = defaultRepository();
        return repo;
    });
}
