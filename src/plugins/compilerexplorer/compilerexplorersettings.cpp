// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "compilerexplorersettings.h"
#include "compilerexplorertr.h"

#include "api/compiler.h"
#include "api/language.h"
#include "api/library.h"

#include <QComboBox>
#include <QFutureWatcher>
#include <QNetworkAccessManager>

namespace CompilerExplorer {

Settings &settings()
{
    static Settings instance;
    return instance;
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

Settings::Settings()
{
    static QNetworkAccessManager networkManager;
    m_networkAccessManager = &networkManager;

    setSettingsGroup("CompilerExplorer");

    source.setDefaultValue(R"(
int main()
{
    return 0;
}

)");

    compilerExplorerUrl.setLabelText(Tr::tr("Compiler Explorer URL:"));
    compilerExplorerUrl.setToolTip(Tr::tr("URL of the Compiler Explorer instance to use"));
    compilerExplorerUrl.setDefaultValue("https://godbolt.org/");
    compilerExplorerUrl.setDisplayStyle(Utils::StringAspect::DisplayStyle::LineEditDisplay);
    compilerExplorerUrl.setHistoryCompleter("CompilerExplorer.Url.History");

    languageId.setDefaultValue("c++");
    languageId.setLabelText(Tr::tr("Language:"));
    languageId.setFillCallback([this](auto cb) { fillLanguageIdModel(cb); });

    connect(&compilerExplorerUrl, &Utils::StringAspect::changed, this, [this] {
        languageId.setValue(languageId.defaultValue());
        cachedLanguages().clear();
        languageId.refill();
    });

    readSettings();
}

QString Settings::languageExtension() const
{
    auto it = std::find_if(std::begin(cachedLanguages()),
                           std::end(cachedLanguages()),
                           [this](const Api::Language &lang) { return lang.id == languageId(); });

    if (it != cachedLanguages().end())
        return it->extensions.first();

    return ".cpp";
}

CompilerSettings::CompilerSettings(Settings *settings)
    : m_parent(settings)
{
    setAutoApply(true);
    compilerOptions.setDefaultValue("-O3");
    compilerOptions.setLabelText(Tr::tr("Compiler options:"));
    compilerOptions.setToolTip(Tr::tr("Arguments passed to the compiler"));
    compilerOptions.setDisplayStyle(Utils::StringAspect::DisplayStyle::LineEditDisplay);

    compiler.setDefaultValue("clang_trunk");
    compiler.setLabelText(Tr::tr("Compiler:"));
    compiler.setFillCallback([this](auto cb) { fillCompilerModel(cb); });

    libraries.setLabelText(Tr::tr("Libraries:"));
    libraries.setFillCallback([this](auto cb) { fillLibraries(cb); });

    executeCode.setLabelText(Tr::tr("Execute the code"));
    compileToBinaryObject.setLabelText(Tr::tr("Compile to binary object"));
    intelAsmSyntax.setLabelText(Tr::tr("Intel asm syntax"));
    intelAsmSyntax.setDefaultValue(true);
    demangleIdentifiers.setLabelText(Tr::tr("Demangle identifiers"));
    demangleIdentifiers.setDefaultValue(true);

    connect(&settings->compilerExplorerUrl, &Utils::StringAspect::changed, this, [this] {
        cachedCompilers().clear();
        cachedLibraries().clear();

        compiler.refill();
        libraries.refill();
    });

    connect(&settings->languageId, &StringSelectionAspect::changed, this, [this] {
        compiler.refill();
        libraries.refill();
        if (m_parent->languageId() == "c++")
            compilerOptions.setValue("-O3");
        else
            compilerOptions.setValue("");
    });
}

void CompilerSettings::fillLibraries(LibrarySelectionAspect::ResultCallback cb)
{
    const QString lang = m_parent->languageId();
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

    auto future = Api::libraries(m_parent->apiConfig(), lang);

    auto watcher = new QFutureWatcher<Api::Libraries>(this);
    watcher->setFuture(future);
    QObject::connect(watcher,
                     &QFutureWatcher<Api::Libraries>::finished,
                     this,
                     [watcher, fillFromCache, lang]() {
                         try {
                             cachedLibraries(lang) = watcher->result();
                             fillFromCache();
                         } catch (const std::exception &e) {
                             qCritical() << e.what();
                             return;
                         }
                     });
}

void Settings::fillLanguageIdModel(StringSelectionAspect::ResultCallback cb)
{
    auto fillFromCache = [cb, this] {
        QList<QStandardItem *> items;
        for (const Api::Language &language : cachedLanguages()) {
            auto *newItem = new QStandardItem(language.name);
            newItem->setData(language.id);

            if (QFile::exists(":/compilerexplorer/logos/" + language.logoUrl)) {
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

    auto future = Api::languages(apiConfig());

    auto watcher = new QFutureWatcher<Api::Languages>(this);
    watcher->setFuture(future);
    QObject::connect(watcher,
                     &QFutureWatcher<Api::Languages>::finished,
                     this,
                     [watcher, fillFromCache]() {
                         try {
                             cachedLanguages() = watcher->result();
                             fillFromCache();
                         } catch (const std::exception &e) {
                             qCritical() << e.what();
                             return;
                         }
                     });
}

void CompilerSettings::fillCompilerModel(StringSelectionAspect::ResultCallback cb)
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

    auto it = cachedCompilers().find(m_parent->languageId());
    if (it != cachedCompilers().end()) {
        fillFromCache(it);
        return;
    }

    auto future = Api::compilers(m_parent->apiConfig(), m_parent->languageId());

    auto watcher = new QFutureWatcher<Api::Compilers>(this);
    watcher->setFuture(future);
    QObject::connect(watcher,
                     &QFutureWatcher<Api::Compilers>::finished,
                     this,
                     [watcher, this, fillFromCache]() {
                         try {
                             auto result = watcher->result();
                             auto itCache = cachedCompilers().insert(m_parent->languageId(), {});

                             for (const Api::Compiler &compiler : result)
                                 itCache->insert(compiler.name, compiler.id);

                             fillFromCache(itCache);
                         } catch (const std::exception &e) {
                             qCritical() << e.what();
                             return;
                         }
                     });
}

} // namespace CompilerExplorer
