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

#ifndef QTVERSIONMANAGER_H
#define QTVERSIONMANAGER_H

#include "qtsupport_global.h"
#include "baseqtversion.h"

#include <QtCore/QSet>
#include <QtCore/QStringList>

namespace Utils {
class Environment;
}

namespace ProjectExplorer {
class HeaderPath;
class IOutputParser;
class Task;
}

namespace QtSupport {
namespace Internal {
class QtOptionsPageWidget;
class QtOptionsPage;
}

struct QMakeAssignment
{
    QString variable;
    QString op;
    QString value;
};

class QTSUPPORT_EXPORT QtVersionManager : public QObject
{
    Q_OBJECT
    // for getUniqueId();
    friend class BaseQtVersion;
    friend class Internal::QtOptionsPage;
public:
    static QtVersionManager *instance();
    QtVersionManager();
    ~QtVersionManager();
    void extensionsInitialized();

    // This will *always* return at least one (Qt in Path), even if that is
    // unconfigured.
    QList<BaseQtVersion *> versions() const;
    QList<BaseQtVersion *> validVersions() const;

    // Note: DO NOT STORE THIS POINTER!
    //       The QtVersionManager will delete it at random times and you will
    //       need to get a new pointer by calling this method again!
    BaseQtVersion *version(int id) const;

    BaseQtVersion *qtVersionForQMakeBinary(const QString &qmakePath);

    // Used by the projectloadwizard
    void addVersion(BaseQtVersion *version);
    void removeVersion(BaseQtVersion *version);

    // Target Support:
    bool supportsTargetId(const QString &id) const;
    // This returns a list of versions that support the target with the given id.
    // @return A list of QtVersions that supports a target. This list may be empty!

    QList<BaseQtVersion *> versionsForTargetId(const QString &id, const QtVersionNumber &minimumQtVersion = QtVersionNumber()) const;
    QSet<QString> supportedTargetIds() const;

    // Static Methods
    enum MakefileCompatible { CouldNotParse, DifferentProject, SameProject };
    static MakefileCompatible makefileIsFor(const QString &makefile, const QString &proFile);
    static QPair<BaseQtVersion::QmakeBuildConfigs, QString> scanMakeFile(const QString &makefile,
                                                                     BaseQtVersion::QmakeBuildConfigs defaultBuildConfig);
    static QString findQMakeBinaryFromMakefile(const QString &directory);
    bool isValidId(int id) const;

    // Compatibility with pre-2.2:
    QString popPendingMwcUpdate();
    QString popPendingGcceUpdate();
signals:
    // content of BaseQtVersion objects with qmake path might have changed
    void dumpUpdatedFor(const QString &qmakeCommand);
    void qtVersionsChanged(const QList<int> &uniqueIds);
    void updateExamples(QString, QString, QString);

public slots:
    void updateDumpFor(const QString &qmakeCommand);

private slots:
    void updateSettings();

private:
    // This function is really simplistic...
    static bool equals(BaseQtVersion *a, BaseQtVersion *b);
    static QString findQMakeLine(const QString &directory, const QString &key);
    static QString trimLine(const QString line);
    static void parseArgs(const QString &args,
                          QList<QMakeAssignment> *assignments,
                          QList<QMakeAssignment> *afterAssignments,
                          QString *additionalArguments);
    static BaseQtVersion::QmakeBuildConfigs qmakeBuildConfigFromCmdArgs(QList<QMakeAssignment> *assignments,
                                                                    BaseQtVersion::QmakeBuildConfigs defaultBuildConfig);
    bool restoreQtVersions();
    bool legacyRestore();
    void findSystemQt();
    void updateFromInstaller();
    void saveQtVersions();
    // Used by QtOptionsPage
    void setNewQtVersions(QList<BaseQtVersion *> newVersions);
    // Used by QtVersion
    int getUniqueId();
    void addNewVersionsFromInstaller();
    void updateDocumentation();

    static int indexOfVersionInList(const BaseQtVersion * const version, const QList<BaseQtVersion *> &list);
    void updateUniqueIdToIndexMap();

    QMap<int, BaseQtVersion *> m_versions;
    int m_idcount;
    // managed by QtProjectManagerPlugin
    static QtVersionManager *m_self;

    // Compatibility with pre-2.2:
    QStringList m_pendingMwcUpdates;
    QStringList m_pendingGcceUpdates;
};

} // namespace Qt4ProjectManager

#endif // QTVERSIONMANAGER_H
