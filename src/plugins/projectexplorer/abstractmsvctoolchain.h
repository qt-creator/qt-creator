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

#include "toolchain.h"
#include "abi.h"
#include "headerpath.h"

#include <QMutex>

#include <utils/environment.h>
#include <utils/fileutils.h>

namespace ProjectExplorer {
namespace Internal {

class PROJECTEXPLORER_EXPORT AbstractMsvcToolChain : public ToolChain
{
public:
    explicit AbstractMsvcToolChain(Core::Id typeId, Core::Id l, Detection d,
                                   const Abi &abi, const QString& vcvarsBat);
    explicit AbstractMsvcToolChain(Core::Id typeId, Detection d);
    AbstractMsvcToolChain(const AbstractMsvcToolChain &other);
    ~AbstractMsvcToolChain();

    Abi targetAbi() const override;

    bool isValid() const override;

    QString originalTargetTriple() const override;

    PredefinedMacrosRunner createPredefinedMacrosRunner() const override;
    Macros predefinedMacros(const QStringList &cxxflags) const override;
    CompilerFlags compilerFlags(const QStringList &cxxflags) const override;
    WarningFlags warningFlags(const QStringList &cflags) const override;
    SystemHeaderPathsRunner createSystemHeaderPathsRunner() const override;
    QList<HeaderPath> systemHeaderPaths(const QStringList &cxxflags,
                                        const Utils::FileName &sysRoot) const override;
    void addToEnvironment(Utils::Environment &env) const override;

    QString makeCommand(const Utils::Environment &environment) const override;
    Utils::FileName compilerCommand() const override;
    IOutputParser *outputParser() const override;

    bool canClone() const override;

    QString varsBat() const { return m_vcvarsBat; }

    bool operator ==(const ToolChain &) const override;

    static bool generateEnvironmentSettings(const Utils::Environment &env,
                                            const QString &batchFile,
                                            const QString &batchArgs,
                                            QMap<QString, QString> &envPairs);

protected:
    class WarningFlagAdder
    {
        int m_warningCode = 0;
        WarningFlags &m_flags;
        bool m_doesEnable = false;
        bool m_triggered = false;
    public:
        WarningFlagAdder(const QString &flag, WarningFlags &flags);
        void operator ()(int warningCode, WarningFlags flagsSet);

        bool triggered() const;
    };

    static void inferWarningsForLevel(int warningLevel, WarningFlags &flags);
    virtual Utils::Environment readEnvironmentSetting(const Utils::Environment& env) const = 0;
    // Function must be thread-safe!
    virtual Macros msvcPredefinedMacros(const QStringList cxxflags,
                                        const Utils::Environment& env) const = 0;


    Utils::FileName m_debuggerCommand;
    mutable QMutex *m_predefinedMacrosMutex = nullptr;
    mutable Macros m_predefinedMacros;
    mutable Utils::Environment m_lastEnvironment;   // Last checked 'incoming' environment.
    mutable Utils::Environment m_resultEnvironment; // Resulting environment for VC
    mutable QMutex *m_headerPathsMutex = nullptr;
    mutable QList<HeaderPath> m_headerPaths;
    Abi m_abi;

   QString m_vcvarsBat;
};

} // namespace Internal
} // namespace ProjectExplorer
