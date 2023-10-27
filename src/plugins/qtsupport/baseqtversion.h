// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qtsupport_global.h"

#include <utils/filepath.h>
#include <utils/macroexpander.h>
#include <utils/store.h>

#include <projectexplorer/abi.h>
#include <projectexplorer/task.h>

#include <QSet>
#include <QStringList>
#include <QVersionNumber>

QT_BEGIN_NAMESPACE
class ProFileEvaluator;
class QMakeGlobals;
QT_END_NAMESPACE

namespace Utils {
class Environment;
class FileInProjectFinder;
} // Utils

namespace ProjectExplorer {
class Kit;
class ToolChain;
class Target;
} // ProjectExplorer

namespace QtSupport {

class QtConfigWidget;
class QtVersion;

namespace Internal {
class QtOptionsPageWidget;
class QtVersionPrivate;
}

class QTSUPPORT_EXPORT QtVersion
{
public:
    using Predicate = std::function<bool(const QtVersion *)>;

    virtual ~QtVersion();

    virtual void fromMap(const Utils::Store &map,
                         const Utils::FilePath &filePath = {},
                         bool forceRefreshCache = false);
    virtual bool equals(QtVersion *other);

    bool isAutodetected() const;
    QString detectionSource() const;

    QString displayName() const;
    QString unexpandedDisplayName() const;
    void setUnexpandedDisplayName(const QString &name);

    // All valid Ids are >= 0
    int uniqueId() const;

    QString type() const;

    virtual Utils::Store toMap() const;
    virtual bool isValid() const;
    static Predicate isValidPredicate(const Predicate &predicate = {});
    virtual QString invalidReason() const;
    virtual QStringList warningReason() const;

    virtual QString description() const = 0;
    virtual QString toHtml(bool verbose) const;

    bool hasQtAbisSet() const;
    ProjectExplorer::Abis qtAbis() const;
    void setQtAbis(const ProjectExplorer::Abis &abis);
    bool hasAbi(ProjectExplorer::Abi::OS, ProjectExplorer::Abi::OSFlavor flavor = ProjectExplorer::Abi::UnknownFlavor) const;

    void applyProperties(QMakeGlobals *qmakeGlobals) const;
    virtual void addToEnvironment(const ProjectExplorer::Kit *k, Utils::Environment &env) const;
    Utils::Environment qmakeRunEnvironment() const;

    // source path defined by qmake property QT_INSTALL_PREFIX/src or by qmake.stash QT_SOURCE_TREE
    Utils::FilePath sourcePath() const;
    // returns source path for installed qt packages and empty string for self build qt
    Utils::FilePath qtPackageSourcePath() const;
    bool isInQtSourceDirectory(const Utils::FilePath &filePath) const;
    bool isQtSubProject(const Utils::FilePath &filePath) const;

    Utils::FilePath rccFilePath() const;
    // used by UiCodeModelSupport
    Utils::FilePath uicFilePath() const;
    Utils::FilePath designerFilePath() const;
    Utils::FilePath linguistFilePath() const;
    Utils::FilePath qscxmlcFilePath() const;
    Utils::FilePath qmlRuntimeFilePath() const;
    Utils::FilePath qmlplugindumpFilePath() const;

    QString qtVersionString() const;
    QVersionNumber qtVersion() const;

    Utils::FilePaths qtSoPaths() const;

    bool hasExamples() const;
    bool hasDocs() const;
    bool hasDemos() const;

    // former local functions
    Utils::FilePath qmakeFilePath() const;

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

    /// Check a .pro-file/Qt version combination on possible issues
    /// @return a list of tasks, ordered on severity (errors first, then
    ///         warnings and finally info items.
    ProjectExplorer::Tasks reportIssues(const Utils::FilePath &proFile, const Utils::FilePath &buildDir) const;

    static bool isQmlDebuggingSupported(const ProjectExplorer::Kit *k, QString *reason = nullptr);
    bool isQmlDebuggingSupported(QString *reason = nullptr) const;
    static bool isQtQuickCompilerSupported(const ProjectExplorer::Kit *k, QString *reason = nullptr);
    bool isQtQuickCompilerSupported(QString *reason = nullptr) const;

    bool hasQmlDumpWithRelocatableFlag() const;

    virtual QtConfigWidget *createConfigurationWidget() const;

    QString defaultUnexpandedDisplayName() const;

    virtual QSet<Utils::Id> targetDeviceTypes() const = 0;

    virtual ProjectExplorer::Tasks validateKit(const ProjectExplorer::Kit *k);

    Utils::FilePath prefix() const;

    Utils::FilePath binPath() const;
    Utils::FilePath libExecPath() const;
    Utils::FilePath configurationPath() const;
    Utils::FilePath dataPath() const;
    Utils::FilePath demosPath() const;
    Utils::FilePath docsPath() const;
    Utils::FilePath examplesPath() const;
    Utils::FilePath frameworkPath() const;
    Utils::FilePath headerPath() const;
    Utils::FilePath importsPath() const;
    Utils::FilePath libraryPath() const;
    Utils::FilePath pluginPath() const;
    Utils::FilePath qmlPath() const;
    Utils::FilePath translationsPath() const;

    Utils::FilePath hostBinPath() const;
    Utils::FilePath hostLibexecPath() const;
    Utils::FilePath hostDataPath() const;
    Utils::FilePath hostPrefixPath() const;

    Utils::FilePath mkspecsPath() const;
    Utils::FilePath librarySearchPath() const;

    Utils::FilePaths directoriesToIgnoreInProjectTree() const;

    QString qtNamespace() const;
    QString qtLibInfix() const;
    bool isFrameworkBuild() const;
    // Note: A Qt version can have both a debug and a release built at the same time!
    bool hasDebugBuild() const;
    bool hasReleaseBuild() const;

    Utils::MacroExpander *macroExpander() const; // owned by the Qt version
    static std::unique_ptr<Utils::MacroExpander>
    createMacroExpander(const std::function<const QtVersion *()> &qtVersion);

    static void populateQmlFileFinder(Utils::FileInProjectFinder *finder,
                                      const ProjectExplorer::Target *target);

    QSet<Utils::Id> features() const;

    virtual bool supportsMultipleQtAbis() const;

protected:
    QtVersion();
    QtVersion(const QtVersion &other) = delete;

    virtual QSet<Utils::Id> availableFeatures() const;
    virtual ProjectExplorer::Tasks reportIssuesImpl(const Utils::FilePath &proFile,
                                                    const Utils::FilePath &buildDir) const;

    virtual ProjectExplorer::Abis detectQtAbis() const;

    // helper function for desktop and simulator to figure out the supported abis based on the libraries
    static ProjectExplorer::Abis qtAbisFromLibrary(const Utils::FilePaths &coreLibraries);

    void resetCache() const;

    void ensureMkSpecParsed() const;
    virtual void parseMkSpec(ProFileEvaluator *) const;
    virtual void setupQmakeRunEnvironment(Utils::Environment &env) const;

private:
    void updateDefaultDisplayName();

    friend class QtVersionFactory;
    friend class QtVersionManager;
    friend class Internal::QtVersionPrivate;
    friend class Internal::QtOptionsPageWidget;

    void setId(int id);
    QtVersion *clone(bool forceRefreshCache = false) const;

    Internal::QtVersionPrivate *d = nullptr;
};

using QtVersions = QList<QtVersion *>;

} // QtSupport

Q_DECLARE_OPERATORS_FOR_FLAGS(QtSupport::QtVersion::QmakeBuildConfigs)
