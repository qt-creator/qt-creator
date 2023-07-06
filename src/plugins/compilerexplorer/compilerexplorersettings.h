// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "api/config.h"
#include "compilerexploreraspects.h"

#include <utils/aspects.h>

#include <QNetworkAccessManager>

namespace CompilerExplorer {
class Settings;

class CompilerSettings : public Utils::AspectContainer
{
public:
    CompilerSettings(Settings *settings);

    StringSelectionAspect compiler{this};

    Utils::StringAspect compilerOptions{this};
    LibrarySelectionAspect libraries{this};

    // "Filters"
    Utils::BoolAspect executeCode{this};
    Utils::BoolAspect compileToBinaryObject{this};
    Utils::BoolAspect intelAsmSyntax{this};
    Utils::BoolAspect demangleIdentifiers{this};

private:
    void fillCompilerModel(StringSelectionAspect::ResultCallback cb);
    void fillLibraries(LibrarySelectionAspect::ResultCallback cb);

    Settings *m_parent;
};

class Settings : public Utils::AspectContainer
{
    Q_OBJECT
public:
    Settings();

    StringSelectionAspect languageId{this};
    Utils::StringAspect compilerExplorerUrl{this};

    Utils::StringAspect source{this};

    QNetworkAccessManager *networkAccessManager() const { return m_networkAccessManager; }

    Api::Config apiConfig() const
    {
        return Api::Config(m_networkAccessManager, compilerExplorerUrl());
    }

    QString languageExtension() const;

signals:
    void languagesChanged();

private:
    void fillLanguageIdModel(StringSelectionAspect::ResultCallback cb);

    QNetworkAccessManager *m_networkAccessManager{nullptr};
};

Settings &settings();

} // namespace CompilerExplorer
