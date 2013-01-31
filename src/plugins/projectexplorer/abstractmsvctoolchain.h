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

#ifndef ABSTRACTMSVCTOOLCHAIN_H
#define ABSTRACTMSVCTOOLCHAIN_H

#include "toolchain.h"
#include "abi.h"

#include <utils/environment.h>

namespace ProjectExplorer {
namespace Internal {

class PROJECTEXPLORER_EXPORT AbstractMsvcToolChain : public ToolChain
{
public:
    AbstractMsvcToolChain(const QString &id, bool autodetect, const Abi &abi, const QString& vcvarsBat);
    AbstractMsvcToolChain(const QString &id, bool autodetect);

    Abi targetAbi() const;

    bool isValid() const;

    QByteArray predefinedMacros(const QStringList &cxxflags) const;
    CompilerFlags compilerFlags(const QStringList &cxxflags) const;
    QList<HeaderPath> systemHeaderPaths(const QStringList &cxxflags, const Utils::FileName &sysRoot) const;
    void addToEnvironment(Utils::Environment &env) const;

    QString makeCommand(const Utils::Environment &environment) const;
    Utils::FileName compilerCommand() const;
    IOutputParser *outputParser() const;

    bool canClone() const;

    QString varsBat() const { return m_vcvarsBat; }

    bool operator ==(const ToolChain &) const;

protected:
    virtual Utils::Environment readEnvironmentSetting(Utils::Environment& env) const = 0;
    virtual QByteArray msvcPredefinedMacros(const QStringList cxxflags,
                                            const Utils::Environment& env) const;

    bool generateEnvironmentSettings(Utils::Environment &env,
                                     const QString& batchFile,
                                     const QString& batchArgs,
                                     QMap<QString, QString>& envPairs) const;

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
