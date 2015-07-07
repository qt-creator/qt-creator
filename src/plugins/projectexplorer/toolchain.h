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

#ifndef TOOLCHAIN_H
#define TOOLCHAIN_H

#include "projectexplorer_export.h"
#include <coreplugin/id.h>

#include <QObject>
#include <QString>
#include <QVariantMap>

namespace Utils {
class Environment;
class FileName;
}

namespace ProjectExplorer {

namespace Internal { class ToolChainPrivate; }

class Abi;
class HeaderPath;
class IOutputParser;
class ToolChainConfigWidget;
class ToolChainFactory;
class ToolChainManager;
class Task;
class Kit;

// --------------------------------------------------------------------------
// ToolChain (documentation inside)
// --------------------------------------------------------------------------

class PROJECTEXPLORER_EXPORT ToolChain
{
public:
    enum Detection {
        ManualDetection,
        AutoDetection,
        AutoDetectionFromSettings
    };

    virtual ~ToolChain();

    QString displayName() const;
    void setDisplayName(const QString &name);

    inline bool isAutoDetected() const { return detection() != ManualDetection; }
    Detection detection() const;

    QByteArray id() const;

    virtual QList<Utils::FileName> suggestedMkspecList() const;
    virtual Utils::FileName suggestedDebugger() const;

    Core::Id typeId() const;
    virtual QString typeDisplayName() const = 0;
    virtual Abi targetAbi() const = 0;

    virtual bool isValid() const = 0;

    virtual QByteArray predefinedMacros(const QStringList &cxxflags) const = 0;

    enum CompilerFlag {
        NoFlags = 0,
        StandardCxx11 = 0x1,
        StandardC99 = 0x2,
        StandardC11 = 0x4,
        GnuExtensions = 0x8,
        MicrosoftExtensions = 0x10,
        BorlandExtensions = 0x20,
        OpenMP = 0x40,
        ObjectiveC = 0x80,
        StandardCxx14 = 0x100,
        StandardCxx17 = 0x200,
        StandardCxx98 = 0x400,
    };
    Q_DECLARE_FLAGS(CompilerFlags, CompilerFlag)

    virtual CompilerFlags compilerFlags(const QStringList &cxxflags) const = 0;

    enum WarningFlag {
        // General settings
        WarningsAsErrors,
        WarningsDefault,
        WarningsAll,
        WarningsExtra,
        WarningsPedantic,

        // Any language
        WarnUnusedLocals,
        WarnUnusedParams,
        WarnUnusedFunctions,
        WarnUnusedResult,
        WarnUnusedValue,
        WarnDocumentation,
        WarnUninitializedVars,
        WarnHiddenLocals,
        WarnUnknownPragma,
        WarnDeprecated,
        WarnSignedComparison,
        WarnIgnoredQualfiers,

        // C++
        WarnOverloadedVirtual,
        WarnEffectiveCxx,
        WarnNonVirtualDestructor
    };
    Q_DECLARE_FLAGS(WarningFlags, WarningFlag)

    virtual WarningFlags warningFlags(const QStringList &cflags) const = 0;
    virtual QList<HeaderPath> systemHeaderPaths(const QStringList &cxxflags,
                                                const Utils::FileName &sysRoot) const = 0;
    virtual void addToEnvironment(Utils::Environment &env) const = 0;
    virtual QString makeCommand(const Utils::Environment &env) const = 0;

    virtual Utils::FileName compilerCommand() const = 0;
    virtual IOutputParser *outputParser() const = 0;

    virtual bool operator ==(const ToolChain &) const;

    virtual ToolChainConfigWidget *configurationWidget() = 0;
    virtual bool canClone() const;
    virtual ToolChain *clone() const = 0;

    // Used by the toolchainmanager to save user-generated tool chains.
    // Make sure to call this function when deriving!
    virtual QVariantMap toMap() const;
    virtual QList<Task> validateKit(const Kit *k) const;
protected:
    explicit ToolChain(Core::Id typeId, Detection d);
    explicit ToolChain(const ToolChain &);

    void toolChainUpdated();

    // Make sure to call this function when deriving!
    virtual bool fromMap(const QVariantMap &data);

private:
    void setDetection(Detection d);

    Internal::ToolChainPrivate *const d;

    friend class ToolChainManager;
    friend class ToolChainFactory;
};

class PROJECTEXPLORER_EXPORT ToolChainFactory : public QObject
{
    Q_OBJECT

public:
    Core::Id typeId() const { return m_typeId; }
    QString displayName() const { return m_displayName; }

    virtual QList<ToolChain *> autoDetect();

    virtual bool canCreate();
    virtual ToolChain *create();

    virtual bool canRestore(const QVariantMap &data);
    virtual ToolChain *restore(const QVariantMap &data);

    static QByteArray idFromMap(const QVariantMap &data);
    static Core::Id typeIdFromMap(const QVariantMap &data);
    static void autoDetectionToMap(QVariantMap &data, bool detected);

protected:
    void setTypeId(Core::Id id) { m_typeId = id; }
    void setDisplayName(const QString &name) { m_displayName = name; }

private:
    QString m_displayName;
    Core::Id m_typeId;
};

} // namespace ProjectExplorer

#endif // TOOLCHAIN_H
