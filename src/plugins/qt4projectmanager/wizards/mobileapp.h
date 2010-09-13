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

#ifndef MOBILEAPP_H
#define MOBILEAPP_H

#include <coreplugin/basefilewizard.h>

#include <QtCore/QStringList>
#include <QtCore/QFileInfo>
#include <QtCore/QHash>

namespace Qt4ProjectManager {
namespace Internal {

struct MobileAppGeneratedFileInfo
{
    enum File {
        MainCppFile,
        AppProFile,
        AppPriFile,
        MainWindowCppFile,
        MainWindowHFile,
        MainWindowUiFile,
        SymbianSvgIconFile,
        MaemoPngIconFile,
        DesktopFile
    };

    MobileAppGeneratedFileInfo();

    bool isUpToDate() const;
    bool isOutdated() const;
    bool wasModified() const;

    File file;
    QFileInfo fileInfo;
    int version;
    quint16 dataChecksum;
    quint16 statedChecksum;
};

class MobileApp: public QObject
{
public:
    enum Orientation {
        LockLandscape,
        LockPortrait,
        Auto
    };

    enum Path {
        MainCpp,
        MainCppOrigin,
        AppPro,
        AppProOrigin,
        AppProPath,
        Desktop,
        DesktopOrigin,
        AppPri,
        AppPriOrigin,
        MainWindowCpp,
        MainWindowCppOrigin,
        MainWindowH,
        MainWindowHOrigin,
        MainWindowUi,
        MainWindowUiOrigin,
        SymbianSvgIcon,
        SymbianSvgIconOrigin,
        MaemoPngIcon,
        MaemoPngIconOrigin,
    };

    MobileApp();
    ~MobileApp();

    void setOrientation(Orientation orientation);
    Orientation orientation() const;
    void setProjectName(const QString &name);
    QString projectName() const;
    void setProjectPath(const QString &path);
    void setSymbianSvgIcon(const QString &icon);
    QString symbianSvgIcon() const;
    void setMaemoPngIcon(const QString &icon);
    QString maemoPngIcon() const;
    void setSymbianTargetUid(const QString &uid);
    QString symbianTargetUid() const;
    void setNetworkEnabled(bool enabled);
    bool networkEnabled() const;

    static QString symbianUidForPath(const QString &path);
    Core::GeneratedFiles generateFiles(QString *errorMessage) const;
    QString path(Path path) const;
    QString error() const;
    QByteArray generateFile(MobileAppGeneratedFileInfo::File file,
        const QString *errorMessage) const;
    static int stubVersion();
private:
    QByteArray generateMainCpp(const QString *errorMessage) const;
    QByteArray generateProFile(const QString *errorMessage) const;
    QByteArray generateDesktopFile(const QString *errorMessage) const;
    static QString templatesRoot();

    QString m_projectName;
    QFileInfo m_projectPath;
    QString m_symbianSvgIcon;
    QString m_maemoPngIcon;
    QString m_symbianTargetUid;
    Orientation m_orientation;
    bool m_networkEnabled;
    QString m_error;
};

} // end of namespace Internal
} // end of namespace Qt4ProjectManager

#endif // MOBILEAPP_H
