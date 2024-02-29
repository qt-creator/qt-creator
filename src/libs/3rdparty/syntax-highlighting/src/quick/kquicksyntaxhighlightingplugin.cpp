/*
    SPDX-FileCopyrightText: 2018 Eike Hein <hein@kde.org>
    SPDX-FileCopyrightText: 2021 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: MIT
*/

#include "kquicksyntaxhighlightingplugin.h"
#include "kquicksyntaxhighlighter.h"

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
    qRegisterMetaType<QList<Definition>>();
    qRegisterMetaType<Theme>();
    qRegisterMetaType<QList<Theme>>();
    qmlRegisterType<KQuickSyntaxHighlighter>(uri, 1, 0, "SyntaxHighlighter");
    qmlRegisterUncreatableMetaObject(Definition::staticMetaObject, uri, 1, 0, "Definition", {});
    qmlRegisterUncreatableMetaObject(Theme::staticMetaObject, uri, 1, 0, "Theme", {});
    qmlRegisterSingletonType<Repository>(uri, 1, 0, "Repository", [](auto engine, auto scriptEngine) {
        (void)engine;
        auto repo = defaultRepository();
        scriptEngine->setObjectOwnership(repo, QJSEngine::CppOwnership);
        return defaultRepository();
    });
}

#include "moc_kquicksyntaxhighlightingplugin.cpp"
