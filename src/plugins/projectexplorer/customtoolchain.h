/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CUSTOMTOOLCHAIN_H
#define CUSTOMTOOLCHAIN_H

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
    enum OutputParser
    {
        Gcc = 0,
        Clang = 1,
        LinuxIcc = 2,
#if defined(Q_OS_WIN)
        Msvc = 3,
#endif
        Custom,
        OutputParserCount
    };

    QString type() const;
    QString typeDisplayName() const;
    Abi targetAbi() const;
    void setTargetAbi(const Abi &);

    bool isValid() const;

    QByteArray predefinedMacros(const QStringList &cxxflags) const;
    CompilerFlags compilerFlags(const QStringList &cxxflags) const;
    WarningFlags warningFlags(const QStringList &cxxflags) const;
    const QStringList &rawPredefinedMacros() const;
    void setPredefinedMacros(const QStringList &list);

    QList<HeaderPath> systemHeaderPaths(const QStringList &cxxFlags, const Utils::FileName &) const;
    void addToEnvironment(Utils::Environment &env) const;
    QList<Utils::FileName> suggestedMkspecList() const;
    IOutputParser *outputParser() const;
    QStringList headerPathsList() const;
    void setHeaderPaths(const QStringList &list);

    QVariantMap toMap() const;
    bool fromMap(const QVariantMap &data);

    ToolChainConfigWidget *configurationWidget();

    bool operator ==(const ToolChain &) const;

    void setCompilerCommand(const Utils::FileName &);
    Utils::FileName compilerCommand() const;
    void setMakeCommand(const Utils::FileName &);
    QString makeCommand(const Utils::Environment &environment) const;

    void setCxx11Flags(const QStringList &);
    const QStringList &cxx11Flags() const;

    void setMkspecs(const QString &);
    QString mkspecs() const;

    ToolChain *clone() const;

    OutputParser outputParserType() const;
    void setOutputParserType(OutputParser parser);
    CustomParserSettings customParserSettings() const;
    void setCustomParserSettings(const CustomParserSettings &settings);
    static QString parserName(OutputParser parser);

protected:
    explicit CustomToolChain(const QString &id, Detection d);
    CustomToolChain(const CustomToolChain &);

private:
    explicit CustomToolChain(Detection d);

    Utils::FileName m_compilerCommand;
    Utils::FileName m_makeCommand;

    Abi m_targetAbi;
    QStringList m_predefinedMacros;
    QList<HeaderPath> m_systemHeaderPaths;
    QStringList m_cxx11Flags;
    QList<Utils::FileName> m_mkspecs;

    OutputParser m_outputParser;
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

    bool canCreate();
    ToolChain *create();

    // Used by the ToolChainManager to restore user-generated tool chains
    bool canRestore(const QVariantMap &data);
    ToolChain *restore(const QVariantMap &data);

protected:
    virtual CustomToolChain *createToolChain(bool autoDetect);
    QList<ToolChain *> autoDetectToolchains(const QString &compiler,
                                            const Abi &);
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

private slots:
    void updateSummaries();
    void errorParserChanged(int index);
    void openCustomParserSettingsDialog();

protected:
    void applyImpl();
    void discardImpl() { setFromToolchain(); }
    bool isDirtyImpl() const;
    void makeReadOnlyImpl();

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

#endif // CUSTOMTOOLCHAIN_H
