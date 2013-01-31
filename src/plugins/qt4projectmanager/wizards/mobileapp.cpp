/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "mobileapp.h"

#include <coreplugin/icore.h>

#include <QDir>
#include <QFile>
#include <QRegExp>
#include <QTextStream>

namespace Qt4ProjectManager {
namespace Internal {

const QString mainWindowBaseName(QLatin1String("mainwindow"));

const QString mainWindowCppFileName(mainWindowBaseName + QLatin1String(".cpp"));
const QString mainWindowHFileName(mainWindowBaseName + QLatin1String(".h"));
const QString mainWindowUiFileName(mainWindowBaseName + QLatin1String(".ui"));


bool MobileAppGeneratedFileInfo::isOutdated() const
{
    return version < AbstractMobileApp::makeStubVersion(MobileApp::StubVersion);
}

MobileApp::MobileApp() : AbstractMobileApp()
{
}

MobileApp::~MobileApp()
{
}

QString MobileApp::pathExtended(int fileType) const
{
    const QString pathBase = outputPathBase();
    switch (fileType) {
        case MainWindowCpp:       return pathBase + mainWindowCppFileName;
        case MainWindowCppOrigin: return originsRoot() + mainWindowCppFileName;
        case MainWindowH:         return pathBase + mainWindowHFileName;
        case MainWindowHOrigin:   return originsRoot() + mainWindowHFileName;
        case MainWindowUi:        return pathBase + mainWindowUiFileName;
        case MainWindowUiOrigin:  return originsRoot() + mainWindowUiFileName;
        default:                  qFatal("MobileApp::path() needs more work");
    }
    return QString();
}

bool MobileApp::adaptCurrentMainCppTemplateLine(QString &line) const
{
    Q_UNUSED(line);
    return true;
}

void MobileApp::handleCurrentProFileTemplateLine(const QString &line,
    QTextStream &proFileTemplate, QTextStream &proFile,
    bool &commentOutNextLine) const
{
    Q_UNUSED(line);
    Q_UNUSED(proFileTemplate);
    Q_UNUSED(proFile);
    Q_UNUSED(commentOutNextLine);
}

Core::GeneratedFiles MobileApp::generateFiles(QString *errorMessage) const
{
    Core::GeneratedFiles files = AbstractMobileApp::generateFiles(errorMessage);

    files.append(file(generateFile(AbstractGeneratedFileInfo::DeploymentPriFile, errorMessage), path(DeploymentPri)));
    files.append(file(generateFile(MobileAppGeneratedFileInfo::MainWindowCppFile, errorMessage), path(MainWindowCpp)));
    files.append(file(generateFile(MobileAppGeneratedFileInfo::MainWindowHFile, errorMessage), path(MainWindowH)));
    files.append(file(generateFile(MobileAppGeneratedFileInfo::MainWindowUiFile, errorMessage), path(MainWindowUi)));
    files.last().setAttributes(Core::GeneratedFile::OpenEditorAttribute);

    return files;
}

QByteArray MobileApp::generateFileExtended(int fileType,
    bool *versionAndCheckSum, QString *comment, QString *errorMessage) const
{
    Q_UNUSED(comment);
    QByteArray data;
    *versionAndCheckSum = false;
    switch (fileType) {
        case MobileAppGeneratedFileInfo::MainWindowCppFile:
            data = readBlob(path(MainWindowCppOrigin), errorMessage);
            break;
        case MobileAppGeneratedFileInfo::MainWindowHFile:
            data = readBlob(path(MainWindowHOrigin), errorMessage);
            break;
        case MobileAppGeneratedFileInfo::MainWindowUiFile:
            data = readBlob(path(MainWindowUiOrigin), errorMessage);
            break;
        default:
            Q_ASSERT_X(false, Q_FUNC_INFO, "Whoops, case missing!");
    }
    return data;
}

QString MobileApp::originsRoot() const
{
    return templatesRoot() + QLatin1String("mobileapp/");
}

QString MobileApp::mainWindowClassName() const
{
    return QLatin1String("MainWindow");
}

int MobileApp::stubVersionMinor() const { return StubVersion; }

QList<AbstractGeneratedFileInfo> MobileApp::updateableFiles(const QString &mainProFile) const
{
    Q_UNUSED(mainProFile)
    return QList<AbstractGeneratedFileInfo>(); // Nothing to update, here
}

QList<DeploymentFolder> MobileApp::deploymentFolders() const
{
    QList<DeploymentFolder> result;
    return result;
}

const int MobileApp::StubVersion = 2;

} // namespace Internal
} // namespace Qt4ProjectManager
