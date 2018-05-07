/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "toolchain.h"
#include "abi.h"
#include "headerpath.h"

#include <utils/fileutils.h>
#include <utils/optional.h>

#include <QMutex>
#include <QStringList>

#include <functional>
#include <memory>

namespace ProjectExplorer {

namespace Internal {
class ClangToolChainFactory;
class GccToolChainConfigWidget;
class GccToolChainFactory;
class MingwToolChainFactory;
class LinuxIccToolChainFactory;
}

// --------------------------------------------------------------------------
// GccToolChain
// --------------------------------------------------------------------------

template<class T, int Size = 16>
class Cache
{
public:
    Cache() { m_cache.reserve(Size); }
    Cache(const Cache &other) = delete;
    Cache &operator =(const Cache &other) = delete;

    Cache(Cache &&other)
    {
        using std::swap;

        QMutexLocker otherLocker(&other.m_mutex);
        swap(m_cache, other.m_cache);
    }

    Cache &operator =(Cache &&other)
    {
        using std::swap;

        QMutexLocker locker(&m_mutex);
        QMutexLocker otherLocker(&other.m_mutex);
        auto temporay(std::move(other.m_cache)); // Make sure other.m_cache is empty!
        swap(m_cache, temporay);
        return *this;
    }

    void insert(const QStringList &compilerArguments, const T &values)
    {
        CacheItem runResults;
        runResults.first = compilerArguments;
        runResults.second = values;

        QMutexLocker locker(&m_mutex);
        if (!checkImpl(compilerArguments)) {
            if (m_cache.size() < Size) {
                m_cache.push_back(runResults);
            } else {
                std::rotate(m_cache.begin(), std::next(m_cache.begin()), m_cache.end());
                m_cache.back() = runResults;
            }
        }
    }

    Utils::optional<T> check(const QStringList &compilerArguments)
    {
        QMutexLocker locker(&m_mutex);
        return checkImpl(compilerArguments);
    }

    void invalidate()
    {
        QMutexLocker locker(&m_mutex);
        m_cache.clear();
    }

private:
    Utils::optional<T> checkImpl(const QStringList &compilerArguments)
    {
        auto it = std::stable_partition(m_cache.begin(), m_cache.end(), [&](const CacheItem &ci) {
            return ci.first != compilerArguments;
        });
        if (it != m_cache.end())
            return m_cache.back().second;
        return {};
    }

    using CacheItem = QPair<QStringList, T>;

    QMutex m_mutex;
    QVector<CacheItem> m_cache;
};

class PROJECTEXPLORER_EXPORT GccToolChain : public ToolChain
{
public:
    GccToolChain(Core::Id typeId, Detection d);
    QString typeDisplayName() const override;
    Abi targetAbi() const override;
    QString originalTargetTriple() const override;
    QString version() const;
    QList<Abi> supportedAbis() const override;
    void setTargetAbi(const Abi &);

    bool isValid() const override;

    CompilerFlags compilerFlags(const QStringList &cxxflags) const override;
    WarningFlags warningFlags(const QStringList &cflags) const override;

    PredefinedMacrosRunner createPredefinedMacrosRunner() const override;
    Macros predefinedMacros(const QStringList &cxxflags) const override;

    SystemHeaderPathsRunner createSystemHeaderPathsRunner() const override;
    QList<HeaderPath> systemHeaderPaths(const QStringList &flags,
                                        const Utils::FileName &sysRoot) const override;

    void addToEnvironment(Utils::Environment &env) const override;
    QString makeCommand(const Utils::Environment &environment) const override;
    Utils::FileNameList suggestedMkspecList() const override;
    IOutputParser *outputParser() const override;

    QVariantMap toMap() const override;
    bool fromMap(const QVariantMap &data) override;

    ToolChainConfigWidget *configurationWidget() override;

    bool operator ==(const ToolChain &) const override;

    void resetToolChain(const Utils::FileName &);
    Utils::FileName compilerCommand() const override;
    void setPlatformCodeGenFlags(const QStringList &);
    QStringList extraCodeModelFlags() const override;
    QStringList platformCodeGenFlags() const;
    void setPlatformLinkerFlags(const QStringList &);
    QStringList platformLinkerFlags() const;

    ToolChain *clone() const override;

    static void addCommandPathToEnvironment(const Utils::FileName &command, Utils::Environment &env);

    class DetectedAbisResult {
    public:
        DetectedAbisResult() = default;
        DetectedAbisResult(const QList<Abi> &supportedAbis,
                           const QString &originalTargetTriple = QString()) :
            supportedAbis(supportedAbis),
            originalTargetTriple(originalTargetTriple)
        { }

        QList<Abi> supportedAbis;
        QString originalTargetTriple;
    };

protected:
    using CacheItem = QPair<QStringList, Macros>;
    using GccCache = QVector<CacheItem>;

    GccToolChain(const GccToolChain &);

    void setCompilerCommand(const Utils::FileName &path);
    void setSupportedAbis(const QList<Abi> &m_abis);
    void setOriginalTargetTriple(const QString &targetTriple);
    void setMacroCache(const QStringList &allCxxflags, const Macros &macroCache) const;
    Macros macroCache(const QStringList &allCxxflags) const;

    virtual QString defaultDisplayName() const;
    virtual CompilerFlags defaultCompilerFlags() const;

    virtual DetectedAbisResult detectSupportedAbis() const;
    virtual QString detectVersion() const;

    // Reinterpret options for compiler drivers inheriting from GccToolChain (e.g qcc) to apply -Wp option
    // that passes the initial options directly down to the gcc compiler
    using OptionsReinterpreter = std::function<QStringList(const QStringList &options)>;
    void setOptionsReinterpreter(const OptionsReinterpreter &optionsReinterpreter);

    using ExtraHeaderPathsFunction = std::function<void(QList<HeaderPath> &)>;
    void initExtraHeaderPathsFunction(ExtraHeaderPathsFunction &&extraHeaderPathsFunction) const;

    static QList<HeaderPath> gccHeaderPaths(const Utils::FileName &gcc, const QStringList &args, const QStringList &env);

    class WarningFlagAdder
    {
    public:
        WarningFlagAdder(const QString &flag, WarningFlags &flags);
        void operator ()(const char name[], WarningFlags flagsSet);

        bool triggered() const;
    private:
        QByteArray m_flagUtf8;
        WarningFlags &m_flags;
        bool m_doesEnable = false;
        bool m_triggered = false;
    };
    void toolChainUpdated() override;

private:
    explicit GccToolChain(Detection d);

    void updateSupportedAbis() const;
    static QStringList gccPrepareArguments(const QStringList &flags,
                                           const QString &sysRoot,
                                           const QStringList &platformCodeGenFlags,
                                           Core::Id languageId,
                                           OptionsReinterpreter reinterpretOptions);

    Utils::FileName m_compilerCommand;
    QStringList m_platformCodeGenFlags;
    QStringList m_platformLinkerFlags;

    OptionsReinterpreter m_optionsReinterpreter = [](const QStringList &v) { return v; };

    Abi m_targetAbi;
    mutable QList<Abi> m_supportedAbis;
    mutable QString m_originalTargetTriple;
    mutable QList<HeaderPath> m_headerPaths;
    mutable QString m_version;

    mutable std::shared_ptr<Cache<QVector<Macro>, 64>> m_predefinedMacrosCache;
    mutable std::shared_ptr<Cache<QList<HeaderPath>>> m_headerPathsCache;
    mutable ExtraHeaderPathsFunction m_extraHeaderPathsFunction = [](QList<HeaderPath> &) {};

    friend class Internal::GccToolChainConfigWidget;
    friend class Internal::GccToolChainFactory;
    friend class ToolChainFactory;
};

// --------------------------------------------------------------------------
// ClangToolChain
// --------------------------------------------------------------------------

class PROJECTEXPLORER_EXPORT ClangToolChain : public GccToolChain
{
public:
    explicit ClangToolChain(Detection d);
    QString typeDisplayName() const override;
    QString makeCommand(const Utils::Environment &environment) const override;

    CompilerFlags compilerFlags(const QStringList &cxxflags) const override;
    WarningFlags warningFlags(const QStringList &cflags) const override;

    IOutputParser *outputParser() const override;

    ToolChain *clone() const override;

    Utils::FileNameList suggestedMkspecList() const override;
    void addToEnvironment(Utils::Environment &env) const override;

protected:
    CompilerFlags defaultCompilerFlags() const override;

private:
    friend class Internal::ClangToolChainFactory;
    friend class ToolChainFactory;
};

// --------------------------------------------------------------------------
// MingwToolChain
// --------------------------------------------------------------------------

class PROJECTEXPLORER_EXPORT MingwToolChain : public GccToolChain
{
public:
    QString typeDisplayName() const override;
    QString makeCommand(const Utils::Environment &environment) const override;

    ToolChain *clone() const override;

    Utils::FileNameList suggestedMkspecList() const override;

private:
    explicit MingwToolChain(Detection d);

    friend class Internal::MingwToolChainFactory;
    friend class ToolChainFactory;
};

// --------------------------------------------------------------------------
// LinuxIccToolChain
// --------------------------------------------------------------------------

class PROJECTEXPLORER_EXPORT LinuxIccToolChain : public GccToolChain
{
public:
    QString typeDisplayName() const override;

    CompilerFlags compilerFlags(const QStringList &cxxflags) const override;
    IOutputParser *outputParser() const override;

    ToolChain *clone() const override;

    Utils::FileNameList suggestedMkspecList() const override;

private:
    explicit LinuxIccToolChain(Detection d);

    friend class Internal::LinuxIccToolChainFactory;
    friend class ToolChainFactory;
};

} // namespace ProjectExplorer
