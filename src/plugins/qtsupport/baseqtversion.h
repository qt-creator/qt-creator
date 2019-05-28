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

#include "qtsupport_global.h"

#include <utils/fileutils.h>
#include <utils/macroexpander.h>

#include <projectexplorer/abi.h>
#include <projectexplorer/task.h>

#include <QSet>
#include <QStringList>
#include <QVariantMap>

namespace Utils {
class Environment;
class FileInProjectFinder;
}
namespace Core { class Id; }

namespace ProjectExplorer {
class IOutputParser;
class Kit;
class ToolChain;
class HeaderPath;
class Target;
} // namespace ProjectExplorer

QT_BEGIN_NAMESPACE
class ProKey;
class ProString;
class ProFileEvaluator;
class QMakeGlobals;
class QSettings;
QT_END_NAMESPACE

namespace QtSupport
{
class QtConfigWidget;

class BaseQtVersion;
class QtVersionFactory;

// Wrapper to make the std::unique_ptr<Utils::MacroExpander> "copyable":
class MacroExpanderWrapper
{
public:
    MacroExpanderWrapper() = default;
    MacroExpanderWrapper(const MacroExpanderWrapper &other) { Q_UNUSED(other); }
    MacroExpanderWrapper(MacroExpanderWrapper &&other) = default;

    Utils::MacroExpander *macroExpander(const BaseQtVersion *qtversion) const;
private:
    mutable std::unique_ptr<Utils::MacroExpander> m_expander;
};

class QTSUPPORT_EXPORT QtVersionNumber
{
public:
    QtVersionNumber(int ma = -1, int mi = -1, int p = -1);
    QtVersionNumber(const QString &versionString);

    QSet<Core::Id> features() const;

    int majorVersion;
    int minorVersion;
    int patchVersion;

    bool matches(int major = -1, int minor = -1, int patch = -1) const;

    bool operator <(const QtVersionNumber &b) const;
    bool operator <=(const QtVersionNumber &b) const;
    bool operator >(const QtVersionNumber &b) const;
    bool operator >=(const QtVersionNumber &b) const;
    bool operator !=(const QtVersionNumber &b) const;
    bool operator ==(const QtVersionNumber &b) const;
};

namespace Internal { class QtOptionsPageWidget; }

class QTSUPPORT_EXPORT BaseQtVersion
{
    friend class QtVersionFactory;
    friend class QtVersionManager;
    friend class QtSupport::Internal::QtOptionsPageWidget;
public:
    using Predicate = std::function<bool(const BaseQtVersion *)>;

    virtual ~BaseQtVersion();

    virtual void fromMap(const QVariantMap &map);
    virtual bool equals(BaseQtVersion *other);

    bool isAutodetected() const;
    QString autodetectionSource() const;

    QString displayName() const;
    QString unexpandedDisplayName() const;
    void setUnexpandedDisplayName(const QString &name);

    // All valid Ids are >= 0
    int uniqueId() const;

    QString type() const;

    virtual QVariantMap toMap() const;
    virtual bool isValid() const;
    static Predicate isValidPredicate(const Predicate &predicate = Predicate());
    virtual QString invalidReason() const;
    virtual QStringList warningReason() const;

    virtual QString description() const = 0;
    virtual QString toHtml(bool verbose) const;

    ProjectExplorer::Abis qtAbis() const;
    virtual ProjectExplorer::Abis detectQtAbis() const;

    enum PropertyVariant { PropertyVariantDev, PropertyVariantGet, PropertyVariantSrc };
    QString qmakeProperty(const QByteArray &name,
                          PropertyVariant variant = PropertyVariantGet) const;
    void applyProperties(QMakeGlobals *qmakeGlobals) const;
    virtual void addToEnvironment(const ProjectExplorer::Kit *k, Utils::Environment &env) const;
    virtual Utils::Environment qmakeRunEnvironment() const;

    // source path defined by qmake property QT_INSTALL_PREFIX/src or by qmake.stash QT_SOURCE_TREE
    Utils::FilePath sourcePath() const;
    // returns source path for installed qt packages and empty string for self build qt
    Utils::FilePath qtPackageSourcePath() const;
    bool isInSourceDirectory(const Utils::FilePath &filePath);
    bool isSubProject(const Utils::FilePath &filePath) const;

    // used by UiCodeModelSupport
    QString uicCommand() const;
    QString designerCommand() const;
    QString linguistCommand() const;
    QString qscxmlcCommand() const;

    QString qtVersionString() const;
    QtVersionNumber qtVersion() const;

    bool hasExamples() const;
    QString examplesPath() const;

    bool hasDocumentation() const;
    QString documentationPath() const;

    bool hasDemos() const;
    QString demosPath() const;

    QString frameworkInstallPath() const;

    // former local functions
    Utils::FilePath qmakeCommand() const;

    /// @returns the name of the mkspec
    QString mkspec() const;
    QString mkspecFor(ProjectExplorer::ToolChain *tc) const;
    /// @returns the full path to the default directory
    /// specifally not the directory the symlink/ORIGINAL_QMAKESPEC points to
    Utils::FilePath mkspecPath() const;

    bool hasMkspec(const QString &spec) const;

    enum QmakeBuildConfig
    {
        NoBuild = 1,
        DebugBuild = 2,
        BuildAll = 8
    };

    Q_DECLARE_FLAGS(QmakeBuildConfigs, QmakeBuildConfig)

    virtual QmakeBuildConfigs defaultBuildConfig() const;
    virtual void recheckDumper();

    /// Check a .pro-file/Qt version combination on possible issues
    /// @return a list of tasks, ordered on severity (errors first, then
    ///         warnings and finally info items.
    ProjectExplorer::Tasks reportIssues(const QString &proFile, const QString &buildDir) const;

    static bool isQmlDebuggingSupported(ProjectExplorer::Kit *k, QString *reason = nullptr);
    bool isQmlDebuggingSupported(QString *reason = nullptr) const;
    static bool isQtQuickCompilerSupported(ProjectExplorer::Kit *k, QString *reason = nullptr);
    bool isQtQuickCompilerSupported(QString *reason = nullptr) const;

    QString qmlDumpTool(bool debugVersion) const;

    bool hasQmlDump() const;
    bool hasQmlDumpWithRelocatableFlag() const;
    bool needsQmlDump() const;

    virtual QtConfigWidget *createConfigurationWidget() const;

    static QString defaultUnexpandedDisplayName(const Utils::FilePath &qmakePath,
                                      bool fromPath = false);

    virtual QSet<Core::Id> targetDeviceTypes() const = 0;

    virtual ProjectExplorer::Tasks validateKit(const ProjectExplorer::Kit *k);

    Utils::FilePath headerPath() const;
    Utils::FilePath docsPath() const;
    Utils::FilePath libraryPath() const;
    Utils::FilePath pluginPath() const;
    Utils::FilePath qmlPath() const;
    Utils::FilePath binPath() const;
    Utils::FilePath mkspecsPath() const;
    Utils::FilePath qmlBinPath() const;
    Utils::FilePath librarySearchPath() const;

    Utils::FilePathList directoriesToIgnoreInProjectTree() const;

    QString qtNamespace() const;
    QString qtLibInfix() const;
    bool isFrameworkBuild() const;
    // Note: A Qt version can have both a debug and a release built at the same time!
    bool hasDebugBuild() const;
    bool hasReleaseBuild() const;

    QStringList configValues() const;
    QStringList qtConfigValues() const;

    Utils::MacroExpander *macroExpander() const; // owned by the Qt version
    static std::unique_ptr<Utils::MacroExpander>
    createMacroExpander(const std::function<const BaseQtVersion *()> &qtVersion);

    static void populateQmlFileFinder(Utils::FileInProjectFinder *finder,
                                      const ProjectExplorer::Target *target);

    QSet<Core::Id> features() const;
    void setIsAutodetected(bool isAutodetected);

protected:
    virtual QSet<Core::Id> availableFeatures() const;

    BaseQtVersion();
    BaseQtVersion(const BaseQtVersion &other) = delete;

    virtual ProjectExplorer::Tasks reportIssuesImpl(const QString &proFile, const QString &buildDir) const;

    // helper function for desktop and simulator to figure out the supported abis based on the libraries
    Utils::FilePathList qtCorePaths() const;
    static ProjectExplorer::Abis qtAbisFromLibrary(const Utils::FilePathList &coreLibraries);

    void ensureMkSpecParsed() const;
    virtual void parseMkSpec(ProFileEvaluator *) const;

private:
    void setupQmakePathAndId(const Utils::FilePath &path);
    void setAutoDetectionSource(const QString &autodetectionSource);
    void updateVersionInfo() const;
    enum HostBinaries { Designer, Linguist, Uic, QScxmlc };
    QString findHostBinary(HostBinaries binary) const;
    void updateMkspec() const;
    QHash<ProKey, ProString> versionInfo() const;
    static bool queryQMakeVariables(const Utils::FilePath &binary,
                                    const Utils::Environment &env,
                                    QHash<ProKey, ProString> *versionInfo,
                                    QString *error = nullptr);
    static QString qmakeProperty(const QHash<ProKey, ProString> &versionInfo,
                                 const QByteArray &name,
                                 PropertyVariant variant = PropertyVariantGet);
    static Utils::FilePath mkspecDirectoryFromVersionInfo(const QHash<ProKey,ProString> &versionInfo);
    static Utils::FilePath mkspecFromVersionInfo(const QHash<ProKey,ProString> &versionInfo);
    static Utils::FilePath sourcePath(const QHash<ProKey,ProString> &versionInfo);
    void setId(int id); // used by the qtversionmanager for legacy restore
                        // and by the qtoptionspage to replace Qt versions

    int m_id = -1;
    const QtVersionFactory *m_factory = nullptr; // The factory that created us.

    bool m_isAutodetected = false;
    mutable bool m_hasQmlDump = false;         // controlled by m_versionInfoUpToDate
    mutable bool m_mkspecUpToDate = false;
    mutable bool m_mkspecReadUpToDate = false;
    mutable bool m_defaultConfigIsDebug = true;
    mutable bool m_defaultConfigIsDebugAndRelease = true;
    mutable bool m_frameworkBuild = false;
    mutable bool m_versionInfoUpToDate = false;
    mutable bool m_installed = true;
    mutable bool m_hasExamples = false;
    mutable bool m_hasDemos = false;
    mutable bool m_hasDocumentation = false;
    mutable bool m_qmakeIsExecutable = true;
    mutable bool m_hasQtAbis = false;

    mutable QStringList m_configValues;
    mutable QStringList m_qtConfigValues;

    QString m_unexpandedDisplayName;
    QString m_autodetectionSource;
    QSet<Core::Id> m_overrideFeatures;
    mutable Utils::FilePath m_sourcePath;
    mutable Utils::FilePath m_qtSources;

    mutable Utils::FilePath m_mkspec;
    mutable Utils::FilePath m_mkspecFullPath;

    mutable QHash<QString, QString> m_mkspecValues;

    mutable QHash<ProKey, ProString> m_versionInfo;

    Utils::FilePath m_qmakeCommand;
    mutable QString m_qtVersionString;
    mutable QString m_uicCommand;
    mutable QString m_designerCommand;
    mutable QString m_linguistCommand;
    mutable QString m_qscxmlcCommand;

    mutable ProjectExplorer::Abis m_qtAbis;

    MacroExpanderWrapper m_expander;
};
}

Q_DECLARE_OPERATORS_FOR_FLAGS(QtSupport::BaseQtVersion::QmakeBuildConfigs)
