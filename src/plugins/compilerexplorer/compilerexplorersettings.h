// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "api/config.h"
#include "compilerexploreraspects.h"

#include <utils/aspects.h>

#include <texteditor/textdocument.h>

#include <QNetworkAccessManager>

namespace CompilerExplorer {
class SourceSettings;
class CompilerSettings;

using ApiConfigFunction = std::function<Api::Config()>;

class PluginSettings : public Utils::AspectContainer
{
public:
    PluginSettings();
    Utils::StringAspect defaultDocument{this};
};

PluginSettings &settings();

class CompilerExplorerSettings : public Utils::AspectContainer
{
public:
    CompilerExplorerSettings();
    ~CompilerExplorerSettings();

    Utils::StringAspect compilerExplorerUrl{this};
    Utils::TypedAspect<Utils::Store> windowState{this};

    Utils::AspectList m_sources{this};

    Api::Config apiConfig() const
    {
        return Api::Config(m_networkAccessManager, compilerExplorerUrl());
    }

    QNetworkAccessManager *networkAccessManager() const { return m_networkAccessManager; }

private:
    QNetworkAccessManager *m_networkAccessManager{nullptr};
};

class SourceSettings : public Utils::AspectContainer,
                       public std::enable_shared_from_this<SourceSettings>
{
    Q_OBJECT
public:
    SourceSettings(const ApiConfigFunction &apiConfigFunction);

    void refresh();

    ApiConfigFunction apiConfigFunction() const { return m_apiConfigFunction; }

    TextEditor::TextDocumentPtr sourceTextDocument() const { return m_sourceTextDocument; }
    void setSourceTextDocument(TextEditor::TextDocumentPtr sourceTextDocument)
    {
        m_sourceTextDocument = sourceTextDocument;
    }

public:
    Utils::StringSelectionAspect languageId{this};
    Utils::StringAspect source{this};
    Utils::AspectList compilers{this};

public:
    QString languageExtension() const;

signals:
    void languagesChanged();

private:
    void fillLanguageIdModel(const Utils::StringSelectionAspect::ResultCallback &cb);

private:
    CompilerExplorerSettings *m_parent;
    ApiConfigFunction m_apiConfigFunction;
    TextEditor::TextDocumentPtr m_sourceTextDocument{nullptr};
};

class CompilerSettings : public Utils::AspectContainer,
                         public std::enable_shared_from_this<CompilerSettings>
{
public:
    CompilerSettings(const ApiConfigFunction &apiConfigFunction);

    Utils::StringSelectionAspect compiler{this};

    Utils::StringAspect compilerOptions{this};
    LibrarySelectionAspect libraries{this};

    // "Filters"
    Utils::BoolAspect executeCode{this};
    Utils::BoolAspect compileToBinaryObject{this};
    Utils::BoolAspect intelAsmSyntax{this};
    Utils::BoolAspect demangleIdentifiers{this};

    void refresh();
    void setLanguageId(const QString &languageId);

private:
    void fillCompilerModel(const Utils::StringSelectionAspect::ResultCallback &cb);
    void fillLibraries(const LibrarySelectionAspect::ResultCallback &cb);

    QString m_languageId;
    ApiConfigFunction m_apiConfigFunction;
};

} // namespace CompilerExplorer
