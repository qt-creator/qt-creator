/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef ABSTRACTMOBILEAPP_H
#define ABSTRACTMOBILEAPP_H

#include <QtCore/QFileInfo>

#ifndef CREATORLESSTEST
#include <coreplugin/basefilewizard.h>
#endif // CREATORLESSTEST

QT_FORWARD_DECLARE_CLASS(QTextStream)

namespace Qt4ProjectManager {
namespace Internal {

struct AbstractGeneratedFileInfo
{
    enum FileType {
        MainQmlFile,
        MainCppFile,
        AppProFile,
        DeploymentPriFile,
        SymbianSvgIconFile,
        MaemoPngIconFile,
        DesktopFile,
        ExtendedFile
    };

    AbstractGeneratedFileInfo();

    bool isUpToDate() const;
    bool wasModified() const;
    virtual bool isOutdated() const=0;

    int fileType;
    QFileInfo fileInfo;
    int version;
    quint16 dataChecksum;
    quint16 statedChecksum;
};

class AbstractMobileApp : public QObject
{
    Q_OBJECT

public:
    enum ScreenOrientation {
        ScreenOrientationLockLandscape,
        ScreenOrientationLockPortrait,
        ScreenOrientationAuto
    };

    enum FileType {
        MainCpp,
        MainCppOrigin,
        AppPro,
        AppProOrigin,
        AppProPath,
        Desktop,
        DesktopOrigin,
        DeploymentPri,
        DeploymentPriOrigin,
        SymbianSvgIcon,
        SymbianSvgIconOrigin,
        MaemoPngIcon,
        MaemoPngIconOrigin,
        ExtendedFile
    };

    virtual ~AbstractMobileApp();

    void setOrientation(ScreenOrientation orientation);
    ScreenOrientation orientation() const;
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
    QString path(int fileType) const;
    QString error() const;

#ifndef CREATORLESSTEST
    virtual Core::GeneratedFiles generateFiles(QString *errorMessage) const;
#else
    bool generateFiles(QString *errorMessage) const;
#endif // CREATORLESSTEST

    static QString symbianUidForPath(const QString &path);
    static int makeStubVersion(int minor);

    static const QString DeploymentPriFileName;
protected:
    AbstractMobileApp();

    static QString templatesRoot();
    static void insertParameter(QString &line, const QString &parameter);

    QByteArray readBlob(const QString &filePath, QString *errorMsg) const;
    QByteArray generateFile(int fileType, QString *errorMessage) const;
    QString outputPathBase() const;

#ifndef CREATORLESSTEST
    static Core::GeneratedFile file(const QByteArray &data,
        const QString &targetFile);
#endif // CREATORLESSTEST

    static const QString CFileComment;
    static const QString ProFileComment;
    static const QString FileChecksum;
    static const QString FileStubVersion;
    static const int StubVersion;

    QString m_error;
private:
    QByteArray generateDesktopFile(QString *errorMessage) const;
    QByteArray generateMainCpp(QString *errorMessage) const;
    QByteArray generateProFile(QString *errorMessage) const;

    virtual QByteArray generateFileExtended(int fileType,
        bool *versionAndCheckSum, QString *comment, QString *errorMessage) const=0;
    virtual QString pathExtended(int fileType) const=0;
    virtual QString originsRoot() const=0;
    virtual QString mainWindowClassName() const=0;
    virtual int stubVersionMinor() const=0;
    virtual bool adaptCurrentMainCppTemplateLine(QString &line) const=0;
    virtual void handleCurrentProFileTemplateLine(const QString &line,
        QTextStream &proFileTemplate, QTextStream &proFile,
        bool &uncommentNextLine) const=0;

    QString m_projectName;
    QFileInfo m_projectPath;
    QString m_symbianSvgIcon;
    QString m_maemoPngIcon;
    QString m_symbianTargetUid;
    ScreenOrientation m_orientation;
    bool m_networkEnabled;
};

} // end of namespace Internal
} // end of namespace Qt4ProjectManager

#endif // ABSTRACTMOBILEAPP_H
