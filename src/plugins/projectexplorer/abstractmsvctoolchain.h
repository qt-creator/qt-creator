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

#ifndef ABSTRACTMSVCTOOLCHAIN_H
#define ABSTRACTMSVCTOOLCHAIN_H

#include "toolchain.h"
#include "abi.h"
#include "headerpath.h"

#include <utils/environment.h>
#include <utils/fileutils.h>

namespace ProjectExplorer {
namespace Internal {

class PROJECTEXPLORER_EXPORT AbstractMsvcToolChain : public ToolChain
{
public:
    explicit AbstractMsvcToolChain(Core::Id typeId, Detection d, const Abi &abi, const QString& vcvarsBat);
    explicit AbstractMsvcToolChain(Core::Id typeId, Detection d);

    Abi targetAbi() const override;

    bool isValid() const override;

    QByteArray predefinedMacros(const QStringList &cxxflags) const override;
    CompilerFlags compilerFlags(const QStringList &cxxflags) const override;
    WarningFlags warningFlags(const QStringList &cflags) const override;
    QList<HeaderPath> systemHeaderPaths(const QStringList &cxxflags,
                                        const Utils::FileName &sysRoot) const override;
    void addToEnvironment(Utils::Environment &env) const override;

    QString makeCommand(const Utils::Environment &environment) const override;
    Utils::FileName compilerCommand() const override;
    IOutputParser *outputParser() const override;

    bool canClone() const override;

    QString varsBat() const { return m_vcvarsBat; }

    bool operator ==(const ToolChain &) const override;

    static bool generateEnvironmentSettings(Utils::Environment &env,
                                            const QString &batchFile,
                                            const QString &batchArgs,
                                            QMap<QString, QString> &envPairs);

protected:
    class WarningFlagAdder
    {
        int m_warningCode;
        WarningFlags &m_flags;
        bool m_doesEnable;
        bool m_triggered;
    public:
        WarningFlagAdder(const QString &flag, WarningFlags &flags);
        void operator ()(int warningCode, WarningFlags flagsSet);
        void operator ()(int warningCode, WarningFlag flag);

        bool triggered() const;
    };

    static void inferWarningsForLevel(int warningLevel, WarningFlags &flags);
    virtual Utils::Environment readEnvironmentSetting(Utils::Environment& env) const = 0;
    virtual QByteArray msvcPredefinedMacros(const QStringList cxxflags,
                                            const Utils::Environment& env) const;


    Utils::FileName m_debuggerCommand;
    mutable QByteArray m_predefinedMacros;
    mutable Utils::Environment m_lastEnvironment;   // Last checked 'incoming' environment.
    mutable Utils::Environment m_resultEnvironment; // Resulting environment for VC
    mutable QList<HeaderPath> m_headerPaths;
    Abi m_abi;

   QString m_vcvarsBat;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // ABSTRACTMSVCTOOLCHAIN_H
