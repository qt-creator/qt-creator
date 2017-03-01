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
#include "projectexplorer_global.h"

#include <coreplugin/id.h>

#include <utils/fileutils.h>

#include <QObject>
#include <QSet>
#include <QString>
#include <QVariantMap>

#include <functional>

namespace Utils { class Environment; }

namespace ProjectExplorer {

namespace Internal { class ToolChainPrivate; }

namespace Deprecated {
// Deprecated in 4.3:
namespace Toolchain {
enum Language {
    None = 0,
    C,
    Cxx
};
QString languageId(Language l);
} // namespace Toolchain
} // namespace Deprecated

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

    using Predicate = std::function<bool(const ToolChain *)>;

    virtual ~ToolChain();

    QString displayName() const;
    void setDisplayName(const QString &name);

    inline bool isAutoDetected() const { return detection() != ManualDetection; }
    Detection detection() const;

    QByteArray id() const;

    virtual Utils::FileNameList suggestedMkspecList() const;
    virtual Utils::FileName suggestedDebugger() const;

    Core::Id typeId() const;
    virtual QString typeDisplayName() const = 0;
    virtual Abi targetAbi() const = 0;
    virtual QList<Abi> supportedAbis() const;
    virtual QString originalTargetTriple() const { return QString(); }

    virtual bool isValid() const = 0;

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
    virtual WarningFlags warningFlags(const QStringList &cflags) const = 0;

    // A PredefinedMacrosRunner is created in the ui thread and runs in another thread.
    using PredefinedMacrosRunner = std::function<QByteArray(const QStringList &cxxflags)>;
    virtual PredefinedMacrosRunner createPredefinedMacrosRunner() const = 0;
    virtual QByteArray predefinedMacros(const QStringList &cxxflags) const = 0;

    // A SystemHeaderPathsRunner is created in the ui thread and runs in another thread.
    using SystemHeaderPathsRunner = std::function<QList<HeaderPath>(const QStringList &cxxflags, const QString &sysRoot)>;
    virtual SystemHeaderPathsRunner createSystemHeaderPathsRunner() const = 0;
    virtual QList<HeaderPath> systemHeaderPaths(const QStringList &cxxflags,
                                                const Utils::FileName &sysRoot) const = 0;
    virtual void addToEnvironment(Utils::Environment &env) const = 0;
    virtual QString makeCommand(const Utils::Environment &env) const = 0;

    Core::Id language() const;

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

    void setLanguage(Core::Id language);

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
    QString displayName() const { return m_displayName; }

    virtual QList<ToolChain *> autoDetect(const QList<ToolChain *> &alreadyKnown);
    virtual QList<ToolChain *> autoDetect(const Utils::FileName &compilerPath, const Core::Id &language);

    virtual bool canCreate();
    virtual ToolChain *create(Core::Id l);

    virtual bool canRestore(const QVariantMap &data);
    virtual ToolChain *restore(const QVariantMap &data);

    static QByteArray idFromMap(const QVariantMap &data);
    static Core::Id typeIdFromMap(const QVariantMap &data);
    static void autoDetectionToMap(QVariantMap &data, bool detected);

    virtual QSet<Core::Id> supportedLanguages() const = 0;

protected:
    void setDisplayName(const QString &name) { m_displayName = name; }

private:
    QString m_displayName;
};

} // namespace ProjectExplorer
