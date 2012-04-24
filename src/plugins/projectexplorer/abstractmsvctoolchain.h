/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

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
    QList<HeaderPath> systemHeaderPaths() const;
    void addToEnvironment(Utils::Environment &env) const;

    QString makeCommand() const;
    Utils::FileName compilerCommand() const;
    IOutputParser *outputParser() const;

    bool canClone() const;

    QString varsBat() const { return m_vcvarsBat; }
    static QString findInstalledJom();

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
