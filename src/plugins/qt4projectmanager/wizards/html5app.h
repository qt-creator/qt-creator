/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#ifndef HTML5APP_H
#define HTML5APP_H

#include "abstractmobileapp.h"

#include <QHash>
#include <QStringList>

namespace Qt4ProjectManager {
namespace Internal {

struct Html5AppGeneratedFileInfo : public AbstractGeneratedFileInfo
{
    enum ExtendedFileType {
        MainHtmlFile = ExtendedFile,
        AppViewerPriFile,
        AppViewerCppFile,
        AppViewerHFile
    };

    Html5AppGeneratedFileInfo() : AbstractGeneratedFileInfo()
    {
    }
};

class Html5App : public AbstractMobileApp
{
public:
    enum ExtendedFileType {
        MainHtml = ExtendedFile,
        MainHtmlDeployed,
        MainHtmlOrigin,
        AppViewerPri,
        AppViewerPriOrigin,
        AppViewerCpp,
        AppViewerCppOrigin,
        AppViewerH,
        AppViewerHOrigin,
        HtmlDir,
        HtmlDirProFileRelative,
        ModulesDir
    };

    enum Mode {
        ModeGenerate,
        ModeImport,
        ModeUrl
    };

    Html5App();
    virtual ~Html5App();

    void setMainHtml(Mode mode, const QString &data = QString());
    Mode mainHtmlMode() const;

    void setTouchOptimizedNavigationEnabled(bool enabled);
    bool touchOptimizedNavigationEnabled() const;

#ifndef CREATORLESSTEST
    virtual Core::GeneratedFiles generateFiles(QString *errorMessage) const;
#else
    bool generateFiles(QString *errorMessage) const;
#endif // CREATORLESSTEST

    static const int StubVersion;

private:
    virtual QByteArray generateFileExtended(int fileType,
        bool *versionAndCheckSum, QString *comment, QString *errorMessage) const;
    virtual QString pathExtended(int fileType) const;
    virtual QString originsRoot() const;
    virtual QString mainWindowClassName() const;
    virtual int stubVersionMinor() const;
    virtual bool adaptCurrentMainCppTemplateLine(QString &line) const;
    virtual void handleCurrentProFileTemplateLine(const QString &line,
        QTextStream &proFileTemplate, QTextStream &proFile,
        bool &commentOutNextLine) const;
    QList<AbstractGeneratedFileInfo> updateableFiles(const QString &mainProFile) const;
    QList<DeploymentFolder> deploymentFolders() const;
    QByteArray appViewerCppFileCode(QString *errorMessage) const;

    QFileInfo m_indexHtmlFile;
    Mode m_mainHtmlMode;
    QString m_mainHtmlData;
    bool m_touchOptimizedNavigationEnabled;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // HTML5APP_H
