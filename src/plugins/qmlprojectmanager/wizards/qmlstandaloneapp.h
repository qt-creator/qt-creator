/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef QMLSTANDALONEAPP_H
#define QMLSTANDALONEAPP_H

#include <QtCore/QString>
#ifndef CREATORLESSTEST
#include <coreplugin/basefilewizard.h>
#endif // CREATORLESSTEST

namespace QmlProjectManager {
namespace Internal {

class QmlStandaloneApp
{
public:
    enum Orientation {
        LockLandscape,
        LockPortrait,
        Auto
    };

    enum Path {
        MainQml,
        MainCpp,
        AppProfile,
        AppViewerCpp,
        AppViewerH,
        SymbianSvgIcon,
        QmlDir
    };

    enum Location {
        Source,
        Target,
        AppProfileRelative,
        MainCppRelative
    };

    QmlStandaloneApp();

    void setOrientation(Orientation orientation);
    Orientation orientation() const;
    void setProjectName(const QString &name);
    QString projectName() const;
    void setProjectPath(const QString &path);
    void setSymbianSvgIcon(const QString &icon);
    QString symbianSvgIcon() const;
    void setSymbianTargetUid(const QString &uid);
    QString symbianTargetUid() const;
    void setLoadDummyData(bool loadIt);
    bool loadDummyData() const;
    void setNetworkEnabled(bool enabled);
    bool networkEnabled() const;

    static QString symbianUidForPath(const QString &path);
#ifndef CREATORLESSTEST
    Core::GeneratedFiles generateFiles(QString *errorMessage) const;
#else
    bool generateFiles(QString *errorMessage) const;
#endif // CREATORLESSTEST

private:
    QString path(Path path, Location location) const;
    QByteArray generateMainCpp(const QString *errorMessage) const;
    QByteArray generateProFile(const QString *errorMessage) const;

    QString m_projectName;
    QString m_projectPath;
    QString m_symbianSvgIcon;
    QString m_symbianTargetUid;
    bool m_loadDummyData;
    Orientation m_orientation;
    bool m_networkEnabled;
};

} // end of namespace Internal
} // end of namespace QmlProjectManager

#endif // QMLSTANDALONEAPP_H
