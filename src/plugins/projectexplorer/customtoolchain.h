/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "projectexplorer_export.h"

#include "abi.h"
#include "customparser.h"
#include "headerpath.h"
#include "toolchain.h"
#include "toolchainconfigwidget.h"

#include <utils/fileutils.h>

QT_BEGIN_NAMESPACE
class QPlainTextEdit;
class QTextEdit;
class QComboBox;
class QPushButton;
QT_END_NAMESPACE

namespace Utils { class PathChooser; }

namespace ProjectExplorer {

class AbiWidget;

namespace Internal { class CustomToolChainFactory; }

// --------------------------------------------------------------------------
// CustomToolChain
// --------------------------------------------------------------------------

class PROJECTEXPLORER_EXPORT CustomToolChain : public ToolChain
{
    Q_DECLARE_TR_FUNCTIONS(CustomToolChain)

public:
    class Parser {
    public:
        Core::Id parserId;   ///< A unique id identifying a parser
        QString displayName; ///< A translateable name to show in the user interface
    };

    Abi targetAbi() const override;
    void setTargetAbi(const Abi &);

    bool isValid() const override;

    MacroInspectionRunner createMacroInspectionRunner() const override;
    Macros predefinedMacros(const QStringList &cxxflags) const override;
    Utils::LanguageExtensions languageExtensions(const QStringList &cxxflags) const override;
    WarningFlags warningFlags(const QStringList &cxxflags) const override;
    const Macros &rawPredefinedMacros() const;
    void setPredefinedMacros(const Macros &macros);

    BuiltInHeaderPathsRunner createBuiltInHeaderPathsRunner(
            const Utils::Environment &) const override;
    HeaderPaths builtInHeaderPaths(const QStringList &cxxFlags,
                                   const Utils::FilePath &,
                                   const Utils::Environment &env) const override;
    void addToEnvironment(Utils::Environment &env) const override;
    QStringList suggestedMkspecList() const override;
    IOutputParser *outputParser() const override;
    QStringList headerPathsList() const;
    void setHeaderPaths(const QStringList &list);

    QVariantMap toMap() const override;
    bool fromMap(const QVariantMap &data) override;

    std::unique_ptr<ToolChainConfigWidget> createConfigurationWidget() override;

    bool operator ==(const ToolChain &) const override;

    void setCompilerCommand(const Utils::FilePath &);
    Utils::FilePath compilerCommand() const override;
    void setMakeCommand(const Utils::FilePath &);
    Utils::FilePath makeCommand(const Utils::Environment &environment) const override;

    void setCxx11Flags(const QStringList &);
    const QStringList &cxx11Flags() const;

    void setMkspecs(const QString &);
    QString mkspecs() const;

    Core::Id outputParserId() const;
    void setOutputParserId(Core::Id parserId);
    CustomParserSettings customParserSettings() const;
    void setCustomParserSettings(const CustomParserSettings &settings);
    static QList<CustomToolChain::Parser> parsers();

private:
    CustomToolChain();

    Utils::FilePath m_compilerCommand;
    Utils::FilePath m_makeCommand;

    Abi m_targetAbi;
    Macros m_predefinedMacros;
    HeaderPaths m_builtInHeaderPaths;
    QStringList m_cxx11Flags;
    QStringList m_mkspecs;

    Core::Id m_outputParserId;
    CustomParserSettings m_customParserSettings;

    friend class Internal::CustomToolChainFactory;
    friend class ToolChainFactory;
};

namespace Internal {

class CustomToolChainFactory : public ToolChainFactory
{
    Q_OBJECT

public:
    CustomToolChainFactory();
};

// --------------------------------------------------------------------------
// CustomToolChainConfigWidget
// --------------------------------------------------------------------------

class TextEditDetailsWidget;

class CustomToolChainConfigWidget : public ToolChainConfigWidget
{
    Q_OBJECT

public:
    CustomToolChainConfigWidget(CustomToolChain *);

private:
    void updateSummaries();
    void errorParserChanged(int index = -1);
    void openCustomParserSettingsDialog();

protected:
    void applyImpl() override;
    void discardImpl() override { setFromToolchain(); }
    bool isDirtyImpl() const override;
    void makeReadOnlyImpl() override;

    void setFromToolchain();

    Utils::PathChooser *m_compilerCommand;
    Utils::PathChooser *m_makeCommand;
    AbiWidget *m_abiWidget;
    QPlainTextEdit *m_predefinedMacros;
    QPlainTextEdit *m_headerPaths;
    TextEditDetailsWidget *m_predefinedDetails;
    TextEditDetailsWidget *m_headerDetails;
    QLineEdit *m_cxx11Flags;
    QLineEdit *m_mkspecs;
    QComboBox *m_errorParserComboBox;
    QPushButton *m_customParserSettingsButton;

    CustomParserSettings m_customParserSettings;
};

} // namespace Internal
} // namespace ProjectExplorer
