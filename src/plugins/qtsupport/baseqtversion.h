/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef BASEQTVERSION_H
#define BASEQTVERSION_H

#include "qtsupport_global.h"

#include <projectexplorer/abi.h>
#include <projectexplorer/headerpath.h>
#include <projectexplorer/task.h>
#include <projectexplorer/ioutputparser.h>
#include <utils/environment.h>

#include <QtCore/QVariantMap>

QT_BEGIN_NAMESPACE
class ProFileEvaluator;
class QSettings;
QT_END_NAMESPACE

namespace QtSupport
{
class QTSUPPORT_EXPORT QtVersionNumber
{
public:
    QtVersionNumber(int ma, int mi, int p);
    QtVersionNumber(const QString &versionString);
    QtVersionNumber();

    int majorVersion;
    int minorVersion;
    int patchVersion;
    bool operator <(const QtVersionNumber &b) const;
    bool operator <=(const QtVersionNumber &b) const;
    bool operator >(const QtVersionNumber &b) const;
    bool operator >=(const QtVersionNumber &b) const;
    bool operator !=(const QtVersionNumber &b) const;
    bool operator ==(const QtVersionNumber &b) const;
private:
    bool checkVersionString(const QString &version) const;
};

namespace Internal { class QtOptionsPageWidget; }

class QTSUPPORT_EXPORT QtConfigWidget : public QWidget
{
    Q_OBJECT
public:
    QtConfigWidget();
signals:
    void changed();
};

class QTSUPPORT_EXPORT BaseQtVersion
{
    friend class QtVersionFactory;
    friend class QtVersionManager;
    friend class QtSupport::Internal::QtOptionsPageWidget;
public:
    virtual ~BaseQtVersion();

    virtual void fromMap(const QVariantMap &map);
    // pre 2.3 settings, only used by SymbianQt
    virtual void restoreLegacySettings(QSettings *s);
    virtual BaseQtVersion *clone() const = 0;
    virtual bool equals(BaseQtVersion *other);

    bool isAutodetected() const;
    QString autodetectionSource() const;

    QString displayName() const;
    void setDisplayName(const QString &name);

    // All valid Ids are >= 0
    int uniqueId() const;

    virtual QString type() const = 0;

    virtual QVariantMap toMap() const;
    virtual bool isValid() const;
    virtual QString invalidReason() const;
    virtual QString warningReason() const;

    virtual bool toolChainAvailable(const QString &id) const;

    virtual QString description() const = 0;
    virtual QString toHtml(bool verbose) const;

    virtual bool supportsTargetId(const QString &id) const = 0;
    virtual QSet<QString> supportedTargetIds() const = 0;
    virtual QList<ProjectExplorer::Abi> qtAbis() const = 0;

    // Returns the PREFIX, BINPREFIX, DOCPREFIX and similar information
    virtual QHash<QString,QString> versionInfo() const;
    virtual void addToEnvironment(Utils::Environment &env) const;

    virtual QString sourcePath() const;
    // used by QtUiCodeModelSupport
    virtual QString uicCommand() const;
    virtual QString designerCommand() const;
    virtual QString linguistCommand() const;
    QString qmlviewerCommand() const;

    virtual QString qtVersionString() const;
    virtual QtVersionNumber qtVersion() const;

    bool hasExamples() const;
    QString examplesPath() const;

    bool hasDocumentation() const;
    QString documentationPath() const;

    bool hasDemos() const;
    QString demosPath() const;

    virtual QList<ProjectExplorer::HeaderPath> systemHeaderPathes() const;
    virtual QString frameworkInstallPath() const;

    // former local functions
    QString qmakeCommand() const;
    virtual QString systemRoot() const;

    /// @returns the name of the mkspec
    QString mkspec() const;
    /// @returns the full path to the default directory
    /// specifally not the directory the symlink/ORIGINAL_QMAKESPEC points to
    QString mkspecPath() const;

    bool hasMkspec(const QString &) const;

    enum QmakeBuildConfig
    {
        NoBuild = 1,
        DebugBuild = 2,
        BuildAll = 8
    };

    Q_DECLARE_FLAGS(QmakeBuildConfigs, QmakeBuildConfig)

    virtual QmakeBuildConfigs defaultBuildConfig() const;
    virtual void recheckDumper();
    virtual bool supportsShadowBuilds() const;

    /// Check a .pro-file/Qt version combination on possible issues
    /// @return a list of tasks, ordered on severity (errors first, then
    ///         warnings and finally info items.
    QList<ProjectExplorer::Task> reportIssues(const QString &proFile, const QString &buildDir);

    virtual ProjectExplorer::IOutputParser *createOutputParser() const;

    static bool queryQMakeVariables(const QString &binary, QHash<QString, QString> *versionInfo);
    static bool queryQMakeVariables(const QString &binary, QHash<QString, QString> *versionInfo, bool *qmakeIsExecutable);
    static QString mkspecFromVersionInfo(const QHash<QString, QString> &versionInfo);


    virtual bool supportsBinaryDebuggingHelper() const;
    virtual QString gdbDebuggingHelperLibrary() const;
    virtual QString qmlDebuggingHelperLibrary(bool debugVersion) const;
    virtual QString qmlDumpTool(bool debugVersion) const;
    virtual QString qmlObserverTool() const;
    virtual QStringList debuggingHelperLibraryLocations() const;

    virtual bool hasGdbDebuggingHelper() const;
    virtual bool hasQmlDump() const;
    virtual bool hasQmlDebuggingLibrary() const;
    virtual bool needsQmlDebuggingLibrary() const;
    virtual bool hasQmlObserver() const;
    Utils::Environment qmlToolsEnvironment() const;

    virtual QtConfigWidget *createConfigurationWidget() const;

    static QString defaultDisplayName(const QString &versionString,
                                      const QString &qmakePath,
                                      bool fromPath = false);

protected:
    BaseQtVersion();
    BaseQtVersion(const QString &path, bool isAutodetected = false, const QString &autodetectionSource = QString());

    virtual QList<ProjectExplorer::Task> reportIssuesImpl(const QString &proFile, const QString &buildDir);

    // helper function for desktop and simulator to figure out the supported abis based on the libraries
    static QString qtCorePath(const QHash<QString,QString> &versionInfo, const QString &versionString);
    static QList<ProjectExplorer::Abi> qtAbisFromLibrary(const QString &coreLibrary);

    void ensureMkSpecParsed() const;
    virtual void parseMkSpec(ProFileEvaluator *) const;
private:
    void setAutoDetectionSource(const QString &autodetectionSource);
    static int getUniqueId();
    void ctor(const QString &qmakePath);
    void updateSourcePath() const;
    void updateVersionInfo() const;
    QString findQtBinary(const QStringList &possibleName) const;
    void updateMkspec() const;
    void setId(int id); // used by the qtversionmanager for legacy restore
                        // and by the qtoptionspage to replace qt versions
    QString m_displayName;
    int m_id;
    bool m_isAutodetected;
    QString m_autodetectionSource;

    mutable QString m_sourcePath;
    mutable bool m_hasDebuggingHelper; // controlled by m_versionInfoUpToDate
    mutable bool m_hasQmlDump;         // controlled by m_versionInfoUpToDate
    mutable bool m_hasQmlDebuggingLibrary; // controlled by m_versionInfoUpdate
    mutable bool m_hasQmlObserver;     // controlled by m_versionInfoUpToDate

    mutable bool m_mkspecUpToDate;
    mutable QString m_mkspec;
    mutable QString m_mkspecFullPath;

    mutable bool m_mkspecReadUpToDate;
    mutable bool m_defaultConfigIsDebug;
    mutable bool m_defaultConfigIsDebugAndRelease;

    mutable bool m_versionInfoUpToDate;
    mutable QHash<QString,QString> m_versionInfo;
    mutable bool m_notInstalled;
    mutable bool m_hasExamples;
    mutable bool m_hasDemos;
    mutable bool m_hasDocumentation;

    mutable QString m_qmakeCommand;
    mutable QString m_qtVersionString;
    mutable QString m_uicCommand;
    mutable QString m_designerCommand;
    mutable QString m_linguistCommand;
    mutable QString m_qmlviewerCommand;

    mutable bool m_qmakeIsExecutable;
};
}

Q_DECLARE_OPERATORS_FOR_FLAGS(QtSupport::BaseQtVersion::QmakeBuildConfigs)
#endif // BASEQTVERSION_H
