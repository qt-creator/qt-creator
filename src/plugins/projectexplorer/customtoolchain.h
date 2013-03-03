/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CUSTOMTOOLCHAIN_H
#define CUSTOMTOOLCHAIN_H

#include "projectexplorer_export.h"

#include "abi.h"
#include "headerpath.h"
#include "toolchain.h"
#include "toolchainconfigwidget.h"

#include <utils/fileutils.h>

QT_BEGIN_NAMESPACE
class QPlainTextEdit;
class QTextEdit;
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
public:
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

protected:
    CustomToolChain(const QString &id, bool autodetect);
    CustomToolChain(const CustomToolChain &);

private:
    CustomToolChain(bool autodetect);

    Utils::FileName m_compilerCommand;
    Utils::FileName m_makeCommand;

    Abi m_targetAbi;
    QStringList m_predefinedMacros;
    QList<HeaderPath> m_systemHeaderPaths;
    QStringList m_cxx11Flags;
    QList<Utils::FileName> m_mkspecs;

    friend class Internal::CustomToolChainFactory;
    friend class ToolChainFactory;
};

namespace Internal {

class CustomToolChainFactory : public ToolChainFactory
{
    Q_OBJECT

public:
    // Name used to display the name of the tool chain that will be created.
    QString displayName() const;
    QString id() const;

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
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // CUSTOMTOOLCHAIN_H
