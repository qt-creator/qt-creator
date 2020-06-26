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
class BaseQtVersion;

class QTSUPPORT_EXPORT QtVersionNumber
{
public:
    QtVersionNumber(int ma = -1, int mi = -1, int p = -1);
    QtVersionNumber(const QString &versionString);

    QSet<Utils::Id> features() const;

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

namespace Internal {
class QtOptionsPageWidget;
class BaseQtVersionPrivate;
}

class QTSUPPORT_EXPORT BaseQtVersion
{
    Q_DECLARE_TR_FUNCTIONS(QtSupport::BaseQtVersion)

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
    QString qmlsceneCommand() const;

    QString qtVersionString() const;
    QtVersionNumber qtVersion() const;

    QStringList qtSoPaths() const;

    bool hasExamples() const;
    bool hasDocs() const;
    bool hasDemos() const;

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

    /// Check a .pro-file/Qt version combination on possible issues
    /// @return a list of tasks, ordered on severity (errors first, then
    ///         warnings and finally info items.
    ProjectExplorer::Tasks reportIssues(const QString &proFile, const QString &buildDir) const;

    static bool isQmlDebuggingSupported(const ProjectExplorer::Kit *k, QString *reason = nullptr);
    bool isQmlDebuggingSupported(QString *reason = nullptr) const;
    static bool isQtQuickCompilerSupported(const ProjectExplorer::Kit *k, QString *reason = nullptr);
    bool isQtQuickCompilerSupported(QString *reason = nullptr) const;

    QString qmlDumpTool(bool debugVersion) const;

    bool hasQmlDump() const;
    bool hasQmlDumpWithRelocatableFlag() const;
    bool needsQmlDump() const;

    virtual QtConfigWidget *createConfigurationWidget() const;

    QString defaultUnexpandedDisplayName() const;

    virtual QSet<Utils::Id> targetDeviceTypes() const = 0;

    virtual ProjectExplorer::Tasks validateKit(const ProjectExplorer::Kit *k);

    Utils::FilePath prefix() const;

    Utils::FilePath binPath() const;
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
    Utils::FilePath hostDataPath() const;

    Utils::FilePath mkspecsPath() const;
    Utils::FilePath qmlBinPath() const;
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
    createMacroExpander(const std::function<const BaseQtVersion *()> &qtVersion);

    static void populateQmlFileFinder(Utils::FileInProjectFinder *finder,
                                      const ProjectExplorer::Target *target);

    QSet<Utils::Id> features() const;

protected:
    BaseQtVersion();
    BaseQtVersion(const BaseQtVersion &other) = delete;

    virtual QSet<Utils::Id> availableFeatures() const;
    virtual ProjectExplorer::Tasks reportIssuesImpl(const QString &proFile, const QString &buildDir) const;

    virtual ProjectExplorer::Abis detectQtAbis() const;

    // helper function for desktop and simulator to figure out the supported abis based on the libraries
    static ProjectExplorer::Abis qtAbisFromLibrary(const Utils::FilePaths &coreLibraries);

    void resetCache() const;

    void ensureMkSpecParsed() const;
    virtual void parseMkSpec(ProFileEvaluator *) const;

private:
    void updateDefaultDisplayName();

    friend class QtVersionFactory;
    friend class QtVersionManager;
    friend class Internal::BaseQtVersionPrivate;
    friend class Internal::QtOptionsPageWidget;

    void setId(int id);
    BaseQtVersion *clone() const;

    Internal::BaseQtVersionPrivate *d = nullptr;
};

} // QtSupport

Q_DECLARE_OPERATORS_FOR_FLAGS(QtSupport::BaseQtVersion::QmakeBuildConfigs)
