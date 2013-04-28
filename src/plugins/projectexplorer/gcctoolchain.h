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

#ifndef GCCTOOLCHAIN_H
#define GCCTOOLCHAIN_H

#include "projectexplorer_export.h"

#include "toolchain.h"
#include "abi.h"
#include "headerpath.h"

#include <utils/fileutils.h>
#include <QStringList>

namespace ProjectExplorer {

namespace Internal {
class ClangToolChainFactory;
class GccToolChainFactory;
class MingwToolChainFactory;
class LinuxIccToolChainFactory;
}

// --------------------------------------------------------------------------
// GccToolChain
// --------------------------------------------------------------------------

class PROJECTEXPLORER_EXPORT GccToolChain : public ToolChain
{
public:
    GccToolChain(const QString &id, bool autodetect);
    QString type() const;
    QString typeDisplayName() const;
    Abi targetAbi() const;
    QString version() const;
    QList<Abi> supportedAbis() const;
    void setTargetAbi(const Abi &);

    bool isValid() const;

    QByteArray predefinedMacros(const QStringList &cxxflags) const;
    CompilerFlags compilerFlags(const QStringList &cxxflags) const;
    WarningFlags warningFlags(const QStringList &cflags) const;

    QList<HeaderPath> systemHeaderPaths(const QStringList &cxxflags, const Utils::FileName &sysRoot) const;
    void addToEnvironment(Utils::Environment &env) const;
    QString makeCommand(const Utils::Environment &environment) const;
    QList<Utils::FileName> suggestedMkspecList() const;
    IOutputParser *outputParser() const;

    QVariantMap toMap() const;
    bool fromMap(const QVariantMap &data);

    ToolChainConfigWidget *configurationWidget();

    bool operator ==(const ToolChain &) const;

    void setCompilerCommand(const Utils::FileName &);
    Utils::FileName compilerCommand() const;
    void setPlatformCodeGenFlags(const QStringList &);
    QStringList platformCodeGenFlags() const;
    void setPlatformLinkerFlags(const QStringList &);
    QStringList platformLinkerFlags() const;

    ToolChain *clone() const;

protected:
    typedef QList<QPair<QStringList, QByteArray> > GccCache;

    GccToolChain(const GccToolChain &);

    virtual QString defaultDisplayName() const;
    virtual CompilerFlags defaultCompilerFlags() const;

    virtual QList<Abi> detectSupportedAbis() const;
    virtual QString detectVersion() const;

    static QList<HeaderPath> gccHeaderPaths(const Utils::FileName &gcc, const QStringList &args, const QStringList &env, const Utils::FileName &sysrootPath);

    static const int PREDEFINED_MACROS_CACHE_SIZE;
    mutable GccCache m_predefinedMacros;

    class WarningFlagAdder
    {
        QByteArray m_flagUtf8;
        WarningFlags &m_flags;
        bool m_doesEnable;
        bool m_triggered;
    public:
        WarningFlagAdder(const QString &flag, WarningFlags &flags);
        void operator ()(const char name[], WarningFlags flagsSet);
        void operator ()(const char name[], WarningFlag flag);

        bool triggered() const;
    };

private:
    GccToolChain(bool autodetect);

    void updateSupportedAbis() const;

    Utils::FileName m_compilerCommand;
    QStringList m_platformCodeGenFlags;
    QStringList m_platformLinkerFlags;

    Abi m_targetAbi;
    mutable QList<Abi> m_supportedAbis;
    mutable QList<HeaderPath> m_headerPaths;
    mutable QString m_version;

    friend class Internal::GccToolChainFactory;
    friend class ToolChainFactory;
};

// --------------------------------------------------------------------------
// ClangToolChain
// --------------------------------------------------------------------------

class PROJECTEXPLORER_EXPORT ClangToolChain : public GccToolChain
{
public:
    QString type() const;
    QString typeDisplayName() const;
    QString makeCommand(const Utils::Environment &environment) const;

    CompilerFlags compilerFlags(const QStringList &cxxflags) const;
    WarningFlags warningFlags(const QStringList &cflags) const;

    IOutputParser *outputParser() const;

    ToolChain *clone() const;

    QList<Utils::FileName> suggestedMkspecList() const;

protected:
    virtual CompilerFlags defaultCompilerFlags() const;

private:
    ClangToolChain(bool autodetect);

    friend class Internal::ClangToolChainFactory;
    friend class ToolChainFactory;
};

// --------------------------------------------------------------------------
// MingwToolChain
// --------------------------------------------------------------------------

class PROJECTEXPLORER_EXPORT MingwToolChain : public GccToolChain
{
public:
    QString type() const;
    QString typeDisplayName() const;
    QString makeCommand(const Utils::Environment &environment) const;

    ToolChain *clone() const;

    QList<Utils::FileName> suggestedMkspecList() const;

private:
    MingwToolChain(bool autodetect);

    friend class Internal::MingwToolChainFactory;
    friend class ToolChainFactory;
};

// --------------------------------------------------------------------------
// LinuxIccToolChain
// --------------------------------------------------------------------------

class PROJECTEXPLORER_EXPORT LinuxIccToolChain : public GccToolChain
{
public:
    QString type() const;
    QString typeDisplayName() const;

    CompilerFlags compilerFlags(const QStringList &cxxflags) const;
    IOutputParser *outputParser() const;

    ToolChain *clone() const;

    QList<Utils::FileName> suggestedMkspecList() const;

private:
    LinuxIccToolChain(bool autodetect);

    friend class Internal::LinuxIccToolChainFactory;
    friend class ToolChainFactory;
};

} // namespace ProjectExplorer

#endif // GCCTOOLCHAIN_H
