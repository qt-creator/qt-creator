/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#ifndef QTVERSIONMANAGER_H
#define QTVERSIONMANAGER_H

#include "qtsupport_global.h"
#include "baseqtversion.h"

namespace QtSupport {

class QTSUPPORT_EXPORT QtVersionManager : public QObject
{
    Q_OBJECT
    // for getUniqueId();
    friend class BaseQtVersion;
    friend class Internal::QtOptionsPageWidget;
public:
    static QObject *instance();
    QtVersionManager();
    ~QtVersionManager();
    static void extensionsInitialized();
    static bool delayedInitialize();

    static bool isLoaded();

    // This will *always* return at least one (Qt in Path), even if that is
    // unconfigured.
    static QList<BaseQtVersion *> versions();
    static QList<BaseQtVersion *> validVersions();

    // Note: DO NOT STORE THIS POINTER!
    //       The QtVersionManager will delete it at random times and you will
    //       need to get a new pointer by calling this function again!
    static BaseQtVersion *version(int id);

    static BaseQtVersion *qtVersionForQMakeBinary(const Utils::FileName &qmakePath);

    static void addVersion(BaseQtVersion *version);
    static void removeVersion(BaseQtVersion *version);

    enum MakefileCompatible { CouldNotParse, DifferentProject, SameProject };
    static MakefileCompatible makefileIsFor(const QString &makefile, const QString &proFile);
    static QPair<BaseQtVersion::QmakeBuildConfigs, QString> scanMakeFile(const QString &makefile,
                                                                     BaseQtVersion::QmakeBuildConfigs defaultBuildConfig);
    static Utils::FileName findQMakeBinaryFromMakefile(const QString &directory);
    static bool isValidId(int id);

signals:
    // content of BaseQtVersion objects with qmake path might have changed
    void dumpUpdatedFor(const Utils::FileName &qmakeCommand);
    void qtVersionsChanged(const QList<int> &addedIds, const QList<int> &removedIds, const QList<int> &changedIds);
    void qtVersionsLoaded();

public slots:
    void updateDumpFor(const Utils::FileName &qmakeCommand);

private slots:
    void updateFromInstaller(bool emitSignal = true);

private:
    // Used by QtOptionsPage
    static void setNewQtVersions(QList<BaseQtVersion *> newVersions);
    // Used by QtVersion
    static int getUniqueId();
};

} // namespace QtSupport

#endif // QTVERSIONMANAGER_H
