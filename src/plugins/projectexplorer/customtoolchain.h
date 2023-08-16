// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"

#include "abi.h"
#include "customparser.h"
#include "headerpath.h"
#include "toolchain.h"

namespace ProjectExplorer {

namespace Internal { class CustomToolChainFactory; }

class PROJECTEXPLORER_EXPORT CustomToolChain : public ToolChain
{
public:
    class Parser {
    public:
        Utils::Id parserId;   ///< A unique id identifying a parser
        QString displayName; ///< A translateable name to show in the user interface
    };

    bool isValid() const override;

    MacroInspectionRunner createMacroInspectionRunner() const override;
    Utils::LanguageExtensions languageExtensions(const QStringList &cxxflags) const override;
    Utils::WarningFlags warningFlags(const QStringList &cxxflags) const override;
    const Macros &rawPredefinedMacros() const;
    void setPredefinedMacros(const Macros &macros);

    BuiltInHeaderPathsRunner createBuiltInHeaderPathsRunner(
            const Utils::Environment &) const override;
    void addToEnvironment(Utils::Environment &env) const override;
    QStringList suggestedMkspecList() const override;
    QList<Utils::OutputLineParser *> createOutputParsers() const override;
    QStringList headerPathsList() const;
    void setHeaderPaths(const QStringList &list);

    void toMap(QVariantMap &data) const override;
    void fromMap(const QVariantMap &data) override;

    std::unique_ptr<ToolChainConfigWidget> createConfigurationWidget() override;

    bool operator ==(const ToolChain &) const override;

    void setMakeCommand(const Utils::FilePath &);
    Utils::FilePath makeCommand(const Utils::Environment &environment) const override;

    void setCxx11Flags(const QStringList &);
    const QStringList &cxx11Flags() const;

    void setMkspecs(const QString &);
    QString mkspecs() const;

    Utils::Id outputParserId() const;
    void setOutputParserId(Utils::Id parserId);
    static QList<CustomToolChain::Parser> parsers();

private:
    CustomToolChain();

    CustomParserSettings customParserSettings() const;

    Utils::FilePath m_makeCommand;

    Macros m_predefinedMacros;
    HeaderPaths m_builtInHeaderPaths;
    QStringList m_cxx11Flags;
    QStringList m_mkspecs;

    Utils::Id m_outputParserId;

    friend class Internal::CustomToolChainFactory;
    friend class ToolChainFactory;
};

namespace Internal {

class CustomToolChainFactory : public ToolChainFactory
{
public:
    CustomToolChainFactory();
};

} // namespace Internal
} // namespace ProjectExplorer
