/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "abstractmobileapp.h"

#include <QDir>
#include <QFile>
#include <QTextStream>

#ifndef CREATORLESSTEST
#include <coreplugin/icore.h>
#endif // CREATORLESSTEST

#include <utils/fileutils.h>
#include <utils/qtcassert.h>

namespace QmakeProjectManager {

AbstractGeneratedFileInfo::AbstractGeneratedFileInfo()
    : fileType(ExtendedFile)
    , currentVersion(-1)
    , version(-1)
    , dataChecksum(0)
    , statedChecksum(0)
{
}

const QString AbstractMobileApp::CFileComment(QLatin1String("//"));
const QString AbstractMobileApp::ProFileComment(QLatin1Char('#'));

AbstractMobileApp::AbstractMobileApp()
    : QObject()
{
}

AbstractMobileApp::~AbstractMobileApp() { }

void AbstractMobileApp::setProjectName(const QString &name)
{
    m_projectName = name;
}

QString AbstractMobileApp::projectName() const
{
    return m_projectName;
}

void AbstractMobileApp::setProjectPath(const QString &path)
{
    m_projectPath.setFile(path);
}

QString AbstractMobileApp::path(int fileType) const
{
    const QString originsRootApp = originsRoot();
    const QString mainCppFileName = QLatin1String("main.cpp");
    switch (fileType) {
        case MainCpp:               return outputPathBase() + mainCppFileName;
        case MainCppOrigin:         return originsRootApp + mainCppFileName;
        case AppPro:                return outputPathBase() + m_projectName + QLatin1String(".pro");
        case AppProOrigin:          return originsRootApp + QLatin1String("app.pro");
        case AppProPath:            return outputPathBase();
        default:                    return pathExtended(fileType);
    }
    return QString();
}

bool AbstractMobileApp::readTemplate(int fileType, QByteArray *data, QString *errorMessage) const
{
    Utils::FileReader reader;
    if (!reader.fetch(path(fileType), errorMessage))
        return false;
    *data = reader.data();
    return true;
}

QByteArray AbstractMobileApp::generateMainCpp(QString *errorMessage) const
{
    QByteArray mainCppInput;
    if (!readTemplate(MainCppOrigin, &mainCppInput, errorMessage))
        return QByteArray();
    QTextStream in(&mainCppInput);

    QByteArray mainCppContent;
    QTextStream out(&mainCppContent, QIODevice::WriteOnly | QIODevice::Text);

    QString line;
    while (!(line = in.readLine()).isNull()) {
        bool adaptLine = true;
        if (line.contains(QLatin1String("// DELETE_LINE")))
            continue; // omit this line in the output
        else
            adaptLine = adaptCurrentMainCppTemplateLine(line);
        if (adaptLine) {
            const int commentIndex = line.indexOf(QLatin1String(" //"));
            if (commentIndex != -1)
                line.truncate(commentIndex);
            out << line << endl;
        }
    }

    return mainCppContent;
}

QByteArray AbstractMobileApp::generateProFile(QString *errorMessage) const
{
    const QChar comment = QLatin1Char('#');
    QByteArray proFileInput;
    if (!readTemplate(AppProOrigin, &proFileInput, errorMessage))
        return QByteArray();
    QTextStream in(&proFileInput);

    QByteArray proFileContent;
    QTextStream out(&proFileContent, QIODevice::WriteOnly | QIODevice::Text);

    QString valueOnNextLine;
    bool commentOutNextLine = false;
    QString line;
    while (!(line = in.readLine()).isNull()) {
        if (line.contains(QLatin1String("# REMOVE_NEXT_LINE"))) {
            in.readLine(); // eats the following line
        } else {
            handleCurrentProFileTemplateLine(line, in, out, commentOutNextLine);
        }

        // Remove all marker comments
        if (line.trimmed().startsWith(comment)
            && line.trimmed().endsWith(comment))
            continue;

        if (!valueOnNextLine.isEmpty()) {
            out << line.left(line.indexOf(QLatin1Char('=')) + 2)
                << QDir::fromNativeSeparators(valueOnNextLine) << endl;
            valueOnNextLine.clear();
            continue;
        }

        if (commentOutNextLine) {
            out << comment << line << endl;
            commentOutNextLine = false;
            continue;
        }
        out << line << endl;
    };

    return proFileContent;
}


#ifndef CREATORLESSTEST
// The definition of QtQuickApp::templatesRoot() for
// CREATORLESSTEST is in tests/manual/appwizards/helpers.cpp
QString AbstractMobileApp::templatesRoot()
{
    return Core::ICore::resourcePath()
        + QLatin1String("/templates/");
}

Core::GeneratedFile AbstractMobileApp::file(const QByteArray &data,
    const QString &targetFile)
{
    Core::GeneratedFile generatedFile(targetFile);
    generatedFile.setBinary(true);
    generatedFile.setBinaryContents(data);
    return generatedFile;
}

Core::GeneratedFiles AbstractMobileApp::generateFiles(QString *errorMessage) const
{
    Core::GeneratedFiles files;
    files << file(generateFile(AbstractGeneratedFileInfo::AppProFile, errorMessage), path(AppPro));
    files.last().setAttributes(Core::GeneratedFile::OpenProjectAttribute);
    files << file(generateFile(AbstractGeneratedFileInfo::MainCppFile, errorMessage), path(MainCpp));
    return files;
}
#endif // CREATORLESSTEST

QString AbstractMobileApp::error() const
{
    return m_error;
}

QByteArray AbstractMobileApp::readBlob(const QString &filePath,
    QString *errorMsg) const
{
    Utils::FileReader reader;
    if (!reader.fetch(filePath, errorMsg))
        return QByteArray();
    return reader.data();
}

QByteArray AbstractMobileApp::generateFile(int fileType,
    QString *errorMessage) const
{
    QByteArray data;
    QString comment = CFileComment;
    switch (fileType) {
        case AbstractGeneratedFileInfo::MainCppFile:
            data = generateMainCpp(errorMessage);
            break;
        case AbstractGeneratedFileInfo::AppProFile:
            data = generateProFile(errorMessage);
            comment = ProFileComment;
            break;
        default:
            data = generateFileExtended(fileType, errorMessage);
    }
    return data;
}

QString AbstractMobileApp::outputPathBase() const
{
    QString path = m_projectPath.absoluteFilePath();
    if (!path.endsWith(QLatin1Char('/')))
            path.append(QLatin1Char('/'));
    return path + projectName() + QLatin1Char('/');
}

void AbstractMobileApp::insertParameter(QString &line, const QString &parameter)
{
    line.replace(QRegExp(QLatin1String("\\([^()]+\\)")),
        QLatin1Char('(') + parameter + QLatin1Char(')'));
}

} // namespace QmakeProjectManager
