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

#include "qtquickapp.h"

#include <utils/qtcassert.h>
#include <utils/fileutils.h>
#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/pluginspec.h>

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QTextStream>

#ifndef CREATORLESSTEST
#include <coreplugin/icore.h>
#endif // CREATORLESSTEST

namespace QmakeProjectManager {
namespace Internal {

static QString sharedDirectory()
{
    return Core::ICore::resourcePath() + QLatin1String("/templates/shared/");
}

static QString templateRootDirectory()
{
    return Core::ICore::resourcePath() + QLatin1String("/templates/qtquick/");
}

static QStringList templateNames()
{
    QStringList templateNameList;
    const QDir templateRoot(templateRootDirectory());

    foreach (const QFileInfo &subDirectory,
             templateRoot.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot))
        templateNameList.append(subDirectory.fileName());

    return templateNameList;
}

// Return locale language attribute "de_UTF8" -> "de", empty string for "C"
static QString languageSetting()
{
#ifdef QT_CREATOR
    QString name = Core::ICore::userInterfaceLanguage();
    const int underScorePos = name.indexOf(QLatin1Char('_'));
    if (underScorePos != -1)
        name.truncate(underScorePos);
    if (name.compare(QLatin1String("C"), Qt::CaseInsensitive) == 0)
        name.clear();
    return name;
#else
    return QLocale::system().name();
#endif
}

static inline bool assignLanguageElementText(QXmlStreamReader &reader,
                                             const QString &desiredLanguage,
                                             QString *target)
{
    const QStringRef elementLanguage = reader.attributes().value(QLatin1String("xml:lang"));
    if (elementLanguage.isEmpty()) {
        // Try to find a translation for our Wizards
        *target = QCoreApplication::translate("QmakeProjectManager::QtQuickAppWizard",
                    reader.readElementText().toLatin1().constData());
        return true;
    }
    if (elementLanguage == desiredLanguage) {
        *target = reader.readElementText();
        return true;
    }
    return false;
}

static bool parseTemplateXml(QXmlStreamReader &reader, TemplateInfo *info)
{
    const QString locale = languageSetting();

    static const QLatin1String tag_template("template");
    static const QLatin1String tag_displayName("displayname");
    static const QLatin1String tag_description("description");
    static const QLatin1String attribute_featuresRequired("featuresRequired");
    static const QLatin1String attribute_openEditor("openeditor");
    static const QLatin1String attribute_priority("priority");
    static const QLatin1String attribute_qrcdeployment("qrcdeployment");
    static const QLatin1String attribute_requiredPlugins("requiredPlugins");

    while (!reader.atEnd() && !reader.hasError()) {
        reader.readNext();
        if (reader.tokenType() != QXmlStreamReader::StartElement)
            continue;

        if (reader.name() == tag_template) {
            info->openFile = reader.attributes().value(attribute_openEditor).toString();
            if (reader.attributes().hasAttribute(attribute_priority))
                info->priority = reader.attributes().value(attribute_priority).toString();

            if (reader.attributes().hasAttribute(attribute_featuresRequired))
                info->featuresRequired = reader.attributes().value(attribute_featuresRequired).toString();

            if (reader.attributes().hasAttribute(attribute_qrcdeployment))
                info->qrcDeployment = reader.attributes().value(attribute_qrcdeployment).toString();

            // This attribute is currently used in enterprise addons to filter out templates when the enterprise
            // addon is not installed. This applies to the Boot To Qt addon for example.
            if (reader.attributes().hasAttribute(attribute_requiredPlugins))
                info->requiredPlugins = reader.attributes().value(attribute_requiredPlugins).toString()
                    .split(QLatin1Char(','), QString::SkipEmptyParts);

        } else if (reader.name() == tag_displayName) {
            if (!assignLanguageElementText(reader, locale, &info->displayName))
                continue;
        } else if (reader.name() == tag_description) {
            if (!assignLanguageElementText(reader, locale, &info->description))
                continue;
        }
    }
    if (reader.hasError()) {
        qWarning() << reader.errorString();
        return false;
    }

    return true;
}

class TemplateInfoList
{
public:
    TemplateInfoList()
    {
        QSet<QString> availablePlugins;
        foreach (ExtensionSystem::PluginSpec *s, ExtensionSystem::PluginManager::plugins()) {
            if (s->state() == ExtensionSystem::PluginSpec::Running && !s->hasError())
                availablePlugins += s->name();
        }

        QMultiMap<QString, TemplateInfo> multiMap;
        foreach (const QString &templateName, templateNames()) {
            const QString templatePath = templateRootDirectory() + templateName;
            QFile xmlFile(templatePath + QLatin1String("/template.xml"));
            if (!xmlFile.open(QIODevice::ReadOnly)) {
                qWarning().nospace() << QString::fromLatin1("Cannot open %1").arg(QDir::toNativeSeparators(QFileInfo(xmlFile.fileName()).absoluteFilePath()));
                continue;
            }
            TemplateInfo info;
            info.templateName = templateName;
            info.templatePath = templatePath;
            QXmlStreamReader reader(&xmlFile);
            if (!parseTemplateXml(reader, &info))
                continue;

            bool ok = true;
            foreach (const QString &neededPlugin, info.requiredPlugins) {
                if (!availablePlugins.contains(neededPlugin)) {
                    ok = false;
                    break;
                }
            }
            if (ok)
                multiMap.insert(info.priority, info);
        }
        m_templateInfoList = multiMap.values();
    }
    QList<TemplateInfo> templateInfoList() const { return m_templateInfoList; }

private:
    QList<TemplateInfo> m_templateInfoList;
};

Q_GLOBAL_STATIC(TemplateInfoList, templateInfoList)

QList<TemplateInfo> QtQuickApp::templateInfos()
{
    return templateInfoList()->templateInfoList();
}

QtQuickApp::QtQuickApp()
    : AbstractMobileApp()
{
}

void QtQuickApp::setTemplateInfo(const TemplateInfo &templateInfo)
{
    m_templateInfo = templateInfo;
}

QString QtQuickApp::pathExtended(int fileType) const
{
    const QString mainQmlFile = QLatin1String("main.qml");
    const QString mainQrcFile = QLatin1String("qml.qrc");

    const QString qrcDeploymentFile = QLatin1String("deployment.pri");

    const QString pathBase = outputPathBase();

    switch (fileType) {
        case MainQml:                       return pathBase + mainQmlFile;
        case MainQmlOrigin:                 return originsRoot() + mainQmlFile;
        case MainQrc:                       return pathBase + mainQrcFile;
        case MainQrcOrigin:                 return originsRoot() + mainQrcFile;
        case QrcDeployment:                 return pathBase + qrcDeploymentFile;
        case QrcDeploymentOrigin:           return sharedDirectory() + qrcDeployment();
        default:                            qFatal("QtQuickApp::pathExtended() needs more work");
    }
    return QString();
}

QString QtQuickApp::originsRoot() const
{
    return m_templateInfo.templatePath + QLatin1Char('/');
}

bool QtQuickApp::adaptCurrentMainCppTemplateLine(QString &line) const
{
    Q_UNUSED(line)
    return true;
}

void QtQuickApp::handleCurrentProFileTemplateLine(const QString &line,
    QTextStream &proFileTemplate, QTextStream &proFile,
    bool &commentOutNextLine) const
{
    Q_UNUSED(commentOutNextLine)
    if (line.contains(QLatin1String("# QML_IMPORT_PATH"))) {
        QString nextLine = proFileTemplate.readLine(); // eats 'QML_IMPORT_PATH ='
        if (!nextLine.startsWith(QLatin1String("QML_IMPORT_PATH =")))
            return;
        proFile << nextLine << endl;
    }
}

#ifndef CREATORLESSTEST

static QFileInfoList allFilesRecursive(const QString &path)
{
    const QDir currentDirectory(path);

    QFileInfoList allFiles = currentDirectory.entryInfoList(QDir::Files);

    foreach (const QFileInfo &subDirectory, currentDirectory.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot))
        allFiles.append(allFilesRecursive(subDirectory.absoluteFilePath()));

    return allFiles;
}

Core::GeneratedFiles QtQuickApp::generateFiles(QString *errorMessage) const
{
    Core::GeneratedFiles files = AbstractMobileApp::generateFiles(errorMessage);

    const QFileInfoList templateFiles = allFilesRecursive(originsRoot());

    //Deploy additional .qml files
    foreach (const QFileInfo &templateFile, templateFiles) {
        QString targetFileName = templateFile.fileName();
        if (templateFile.suffix() == QLatin1String("qml")
                && targetFileName != QLatin1String("main.qml"))
            files.append(file(readBlob(templateFile.absoluteFilePath(), errorMessage), outputPathBase() + targetFileName));
    }

    if (!useExistingMainQml()) {
        files.append(file(generateFile(QtQuickAppGeneratedFileInfo::MainQmlFile, errorMessage), path(MainQml)));
        files.last().setAttributes(Core::GeneratedFile::OpenEditorAttribute);
    }
    if (QFileInfo::exists(path(MainQrcOrigin))) {
        files.append(file(generateFile(QtQuickAppGeneratedFileInfo::MainQrcFile, errorMessage), path(MainQrc)));
    }
    if (!qrcDeployment().isEmpty()) {
        files.append(file(generateFile(QtQuickAppGeneratedFileInfo::QrcDeploymentFile, errorMessage), path(QrcDeployment)));
    }

    return files;
}
#endif // CREATORLESSTEST

bool QtQuickApp::useExistingMainQml() const
{
    return !m_mainQmlFile.filePath().isEmpty();
}

QString QtQuickApp::qrcDeployment() const
{
    return m_templateInfo.qrcDeployment;
}

QByteArray QtQuickApp::generateProFile(QString *errorMessage) const
{
    QByteArray proFileContent = AbstractMobileApp::generateProFile(errorMessage);
    proFileContent.replace("../../shared/qrc", ""); // fix a path to qrcdeployment.pri
    return proFileContent;
}

QByteArray QtQuickApp::generateFileExtended(int fileType, QString *errorMessage) const
{
    QByteArray data;
    switch (fileType) {
        case QtQuickAppGeneratedFileInfo::MainQmlFile:
            data = readBlob(path(MainQmlOrigin), errorMessage);
            break;
        case QtQuickAppGeneratedFileInfo::MainQrcFile:
            data = readBlob(path(MainQrcOrigin), errorMessage);
            break;
        case QtQuickAppGeneratedFileInfo::QrcDeploymentFile:
            data = readBlob(path(QrcDeploymentOrigin), errorMessage);
            break;
        default:
            break;
    }
    return data;
}

} // namespace Internal
} // namespace QmakeProjectManager
