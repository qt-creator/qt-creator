// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "compilerexplorersettings.h"
#include "compilerexplorertr.h"

#include "api/compiler.h"
#include "api/language.h"
#include "api/library.h"

#include <coreplugin/messagemanager.h>

#include <QComboBox>
#include <QFileInfo>

using namespace QtTaskTree;

namespace CompilerExplorer {

PluginSettings &settings()
{
    static PluginSettings instance;
    return instance;
}

PluginSettings::PluginSettings()
{
    defaultDocument.setSettingsKey("DefaultDocument");
    defaultDocument.setDefaultValue(R"(
{
    "Sources": [{
        "LanguageId": "c++",
        "Source": "int main() {\n  return 0;\n}",
        "Compilers": [{
            "Id": "clang_trunk",
            "Options": "-O3"
        }]
    }]
}
        )");
}

static Api::Languages &cachedLanguages()
{
    static Api::Languages instance;
    return instance;
}

static QMap<QString, Api::Libraries> &cachedLibraries()
{
    static QMap<QString, Api::Libraries> instance;
    return instance;
}

static Api::Libraries &cachedLibraries(const QString &languageId)
{
    return cachedLibraries()[languageId];
}

static QMap<QString, QMap<QString, QString>> &cachedCompilers()
{
    static QMap<QString, QMap<QString, QString>> instance;
    return instance;
}

SourceSettings::SourceSettings(const ApiConfigFunction &apiConfigFunction)
    : m_apiConfigFunction(apiConfigFunction)
{
    setAutoApply(false);

    source.setSettingsKey("Source");

    languageId.setSettingsKey("LanguageId");
    languageId.setDefaultValue("c++");
    languageId.setLabelText(Tr::tr("Language:"));
    languageId.setFillCallback([this](auto cb) { fillLanguageIdModel(cb); });

    compilers.setSettingsKey("Compilers");
    compilers.setCreateItemFunction([this, apiConfigFunction] {
        auto result = std::make_shared<CompilerSettings>(apiConfigFunction);
        connect(this, &SourceSettings::languagesChanged, result.get(), &CompilerSettings::refresh);
        languageId.addOnChanged( result.get(), [this, result = result.get()] {
            result->setLanguageId(languageId());
        });

        connect(result.get(), &Utils::AspectContainer::changed, this, &SourceSettings::changed);

        result->setLanguageId(languageId());
        return result;
    });

    for (const auto &aspect : this->aspects()) {
        connect(aspect,
                &Utils::BaseAspect::volatileValueChanged,
                this,
                &Utils::AspectContainer::changed);
    }
}

void SourceSettings::refresh()
{
    languageId.setValue(languageId.defaultValue());
    cachedLanguages().clear();
    languageId.refill();

    compilers.forEachItem(&CompilerSettings::refresh);
}

QString SourceSettings::languageExtension() const
{
    auto it = std::find_if(std::begin(cachedLanguages()),
                           std::end(cachedLanguages()),
                           [this](const Api::Language &lang) { return lang.id == languageId(); });

    if (it != cachedLanguages().end())
        return it->extensions.first();

    return ".cpp";
}

CompilerSettings::CompilerSettings(const ApiConfigFunction &apiConfigFunction)
    : m_apiConfigFunction(apiConfigFunction)
{
    setAutoApply(false);
    compiler.setSettingsKey("Id");
    compiler.setLabelText(Tr::tr("Compiler:"));
    compiler.setFillCallback([this](auto cb) { fillCompilerModel(cb); });

    compilerOptions.setSettingsKey("Options");
    compilerOptions.setLabelText(Tr::tr("Compiler options:"));
    compilerOptions.setToolTip(Tr::tr("Arguments passed to the compiler."));
    compilerOptions.setDisplayStyle(Utils::StringAspect::DisplayStyle::LineEditDisplay);

    libraries.setSettingsKey("Libraries");
    libraries.setLabelText(Tr::tr("Libraries:"));
    libraries.setFillCallback([this](auto cb) { fillLibraries(cb); });

    executeCode.setSettingsKey("ExecuteCode");
    executeCode.setLabelText(Tr::tr("Execute the code"));

    compileToBinaryObject.setSettingsKey("CompileToBinaryObject");
    compileToBinaryObject.setLabelText(Tr::tr("Compile to binary object"));

    intelAsmSyntax.setSettingsKey("IntelAsmSyntax");
    intelAsmSyntax.setLabelText(Tr::tr("Intel asm syntax"));
    intelAsmSyntax.setDefaultValue(true);

    demangleIdentifiers.setSettingsKey("DemangleIdentifiers");
    demangleIdentifiers.setLabelText(Tr::tr("Demangle identifiers"));
    demangleIdentifiers.setDefaultValue(true);

    for (const auto &aspect : this->aspects()) {
        connect(aspect,
                &Utils::BaseAspect::volatileValueChanged,
                this,
                &Utils::AspectContainer::changed);
    }
}

void CompilerSettings::refresh()
{
    cachedCompilers().clear();
    cachedLibraries().clear();

    compiler.refill();
    libraries.refill();
}

void CompilerSettings::setLanguageId(const QString &languageId)
{
    m_languageId = languageId;

    compiler.refill();
    libraries.refill();

    // TODO: Set real defaults ...
    if (m_languageId == "c++")
        compilerOptions.setValue("-O3");
    else
        compilerOptions.setValue("");
}

void CompilerSettings::fillLibraries(const LibrarySelectionAspect::ResultCallback &cb)
{
    const QString lang = m_languageId;
    auto fillFromCache = [cb, lang] {
        QList<QStandardItem *> items;
        for (const Api::Library &lib : cachedLibraries(lang)) {
            QStandardItem *newItem = new QStandardItem(lib.name);
            newItem->setData(QVariant::fromValue(lib), LibrarySelectionAspect::LibraryData);
            items.append(newItem);
        }
        cb(items);
    };

    if (!cachedLibraries(lang).isEmpty()) {
        fillFromCache();
        return;
    }

    const Api::ResultStorage<Api::Libraries> storage;
    const Group recipe {
        storage,
        Api::librariesTask(m_apiConfigFunction(), lang, storage),
        onGroupDone([storage, fillFromCache, lang] {
            const auto &result = *storage;
            if (result) {
                cachedLibraries(lang) = *result;
                fillFromCache();
            } else {
                Core::MessageManager::writeDisrupting(
                    Tr::tr("Failed to fetch libraries: \"%1\".").arg(result.error()));
            }
        }, CallDoneFlag::OnSuccess | CallDoneFlag::OnError),
    };
    m_librariesRunner.start(recipe);
}

void SourceSettings::fillLanguageIdModel(const Utils::StringSelectionAspect::ResultCallback &cb)
{
    auto fillFromCache = [cb, this] {
        QList<QStandardItem *> items;
        for (const Api::Language &language : cachedLanguages()) {
            auto *newItem = new QStandardItem(language.name);
            newItem->setData(language.id);

            if (QFileInfo::exists(":/compilerexplorer/logos/" + language.logoUrl)) {
                QIcon icon(":/compilerexplorer/logos/" + language.logoUrl);
                newItem->setIcon(icon);
            }

            items.append(newItem);
        }
        cb(items);
        emit languagesChanged();
    };

    if (!cachedLanguages().isEmpty()) {
        fillFromCache();
        return;
    }

    const Api::ResultStorage<Api::Languages> storage;
    const Group recipe {
        storage,
        Api::languagesTask(m_apiConfigFunction(), storage),
        onGroupDone([storage, fillFromCache] {
            const auto &result = *storage;
            if (result) {
                cachedLanguages() = *result;
                fillFromCache();
            } else {
                Core::MessageManager::writeDisrupting(
                    Tr::tr("Failed to fetch languages: \"%1\".").arg(result.error()));
            }
        }, CallDoneFlag::OnSuccess | CallDoneFlag::OnError),
    };
    m_languagesRunner.start(recipe);
}

void CompilerSettings::fillCompilerModel(const Utils::StringSelectionAspect::ResultCallback &cb)
{
    auto fillFromCache = [cb](auto it) {
        QList<QStandardItem *> items;
        for (const QString &key : it->keys()) {
            QStandardItem *newItem = new QStandardItem(key);
            newItem->setData(it->value(key));
            items.append(newItem);
        }
        cb(items);
    };

    auto it = cachedCompilers().find(m_languageId);
    if (it != cachedCompilers().end()) {
        fillFromCache(it);
        return;
    }

    const Api::ResultStorage<Api::Compilers> storage;
    const Group recipe {
        storage,
        Api::compilersTask(m_apiConfigFunction(), m_languageId, storage),
        onGroupDone([this, storage, fillFromCache] {
            const auto &result = *storage;
            if (result) {
                auto itCache = cachedCompilers().insert(m_languageId, {});
                for (const Api::Compiler &compiler : *result)
                    itCache->insert(compiler.name, compiler.id);
                fillFromCache(itCache);
            } else {
                Core::MessageManager::writeDisrupting(
                    Tr::tr("Failed to fetch compilers: \"%1\".").arg(result.error()));
            }
        }, CallDoneFlag::OnSuccess | CallDoneFlag::OnError),
    };
    m_compilersRunner.start(recipe);
}

CompilerExplorerSettings::CompilerExplorerSettings()
{
    setAutoApply(false);
    setSettingsKey("CompilerExplorer");

    compilerExplorerUrl.setSettingsKey("CompilerExplorerUrl");
    compilerExplorerUrl.setLabelText(Tr::tr("Compiler Explorer URL:"));
    compilerExplorerUrl.setToolTip(Tr::tr("URL of the Compiler Explorer instance to use."));
    compilerExplorerUrl.setDefaultValue("https://godbolt.org/");
    compilerExplorerUrl.setDisplayStyle(Utils::StringAspect::DisplayStyle::LineEditDisplay);
    compilerExplorerUrl.setHistoryCompleter("CompilerExplorer.Url.History");

    windowState.setSettingsKey("WindowState");

    m_sources.setSettingsKey("Sources");
    m_sources.setCreateItemFunction([this] {
        auto newSourceSettings = std::make_shared<SourceSettings>([this] { return apiConfig(); });
        connect(newSourceSettings.get(),
                &Utils::AspectContainer::changed,
                this,
                &CompilerExplorerSettings::changed);
        return newSourceSettings;
    });

    connect(&compilerExplorerUrl, &Utils::StringAspect::volatileValueChanged, this, [this] {
        m_sources.forEachItem(&SourceSettings::refresh);
    });

    for (const auto &aspect : this->aspects()) {
        connect(aspect,
                &Utils::BaseAspect::volatileValueChanged,
                this,
                &CompilerExplorerSettings::changed);
    }
}

CompilerExplorerSettings::~CompilerExplorerSettings() = default;

} // namespace CompilerExplorer
