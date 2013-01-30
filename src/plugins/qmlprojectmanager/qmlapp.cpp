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

#include "qmlapp.h"


#include <coreplugin/basefilewizard.h>
#include <coreplugin/icore.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QDir>
#include <QTextStream>

namespace QmlProjectManager {
namespace Internal {

static QStringList binaryFiles()
{
    static QStringList result;
    if (result.isEmpty())
        result << QLatin1String("png") << QLatin1String("jpg") << QLatin1String("jpeg");
    return result;
}

QString QmlApp::templateRootDirectory()
{
    return Core::ICore::instance()->resourcePath() + QLatin1String("/templates/qml/");
}

TemplateInfo::TemplateInfo()
    : priority(5)
{
}

QmlApp::QmlApp(QObject *parent)
    : QObject(parent)
{
}

QmlApp::~QmlApp()
{
}

QString QmlApp::mainQmlFileName() const
{
    return projectName() + QLatin1String(".qml");
}

void QmlApp::setProjectNameAndBaseDirectory(const QString &projectName, const QString &projectBaseDirectory)
{
    m_projectBaseDirectory = projectBaseDirectory;
    m_projectName = projectName.trimmed();
}

QString QmlApp::projectDirectory() const
{
    return QDir::cleanPath(m_projectBaseDirectory + QLatin1Char('/') + m_projectName);
}

QString QmlApp::projectName() const
{
    return m_projectName;
}

void QmlApp::setTemplateInfo(const TemplateInfo &templateInfo)
{
    m_templateInfo = templateInfo;
}

QString QmlApp::creatorFileName() const
{
    return m_creatorFileName;
}

const TemplateInfo &QmlApp::templateInfo() const
{
    return m_templateInfo;
}

QString QmlApp::templateDirectory() const
{
    const QDir dir(templateRootDirectory() + m_templateInfo.templateName);
    return QDir::cleanPath(dir.absolutePath());
}

QStringList QmlApp::templateNames()
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
        *target = QCoreApplication::translate("QmlProjectManager::Internal::QmlApplicationWizard",
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
    static const QLatin1String attribute_id("id");
    static const QLatin1String attribute_featuresRequired("featuresRequired");
    static const QLatin1String attribute_openEditor("openeditor");
    static const QLatin1String attribute_priority("priority");

    while (!reader.atEnd() && !reader.hasError()) {
        reader.readNext();
        if (reader.tokenType() != QXmlStreamReader::StartElement)
            continue;

        if (reader.name() == tag_template) {
            info->openFile = reader.attributes().value(attribute_openEditor).toString();
            if (reader.attributes().hasAttribute(attribute_priority))
                info->priority = reader.attributes().value(attribute_priority).toString().toInt();

            if (reader.attributes().hasAttribute(attribute_id))
                info->wizardId = reader.attributes().value(attribute_id).toString();

            if (reader.attributes().hasAttribute(attribute_featuresRequired))
                info->featuresRequired = reader.attributes().value(attribute_featuresRequired).toString();

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

QList<TemplateInfo> QmlApp::templateInfos()
{
    QList<TemplateInfo> result;
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
        if (parseTemplateXml(reader, &info))
            result.append(info);
    }
    return result;
}

static QFileInfoList allFilesRecursive(const QString &path)
{
    const QDir currentDirectory(path);

    QFileInfoList allFiles = currentDirectory.entryInfoList(QDir::Files);

    foreach (const QFileInfo &subDirectory, currentDirectory.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot))
        allFiles.append(allFilesRecursive(subDirectory.absoluteFilePath()));

    return allFiles;
}

QByteArray QmlApp::readFile(const QString &filePath, bool &ok) const
{
    Utils::FileReader reader;

    if (!reader.fetch(filePath)) {
        ok = false;
        return QByteArray();
    }

    ok = true;
    return reader.data();
}

QString QmlApp::readAndAdaptTemplateFile(const QString &filePath, bool &ok) const
{
    const QByteArray originalTemplate = readFile(filePath, ok);
    if (!ok)
        return QString();

    QTextStream tsIn(originalTemplate);
    QByteArray adaptedTemplate;
    QTextStream tsOut(&adaptedTemplate, QIODevice::WriteOnly | QIODevice::Text);
    int lineNr = 1;
    QString line;
    do {
        static const QString markerQtcReplace = QLatin1String("QTC_REPLACE");
        static const QString markerWith = QLatin1String("WITH");

        line = tsIn.readLine();
        const int markerQtcReplaceIndex = line.indexOf(markerQtcReplace);
        if (markerQtcReplaceIndex >= 0) {
            QString replaceXWithYString = line.mid(markerQtcReplaceIndex + markerQtcReplace.length()).trimmed();
            if (filePath.endsWith(QLatin1String(".json")))
                replaceXWithYString.replace(QRegExp(QLatin1String("\",$")), QString());
            else if (filePath.endsWith(QLatin1String(".html")))
                replaceXWithYString.replace(QRegExp(QLatin1String(" -->$")), QString());
            const QStringList replaceXWithY = replaceXWithYString.split(markerWith);
            if (replaceXWithY.count() != 2) {
                qWarning().nospace() << QString::fromLatin1("Error in %1:%2. Invalid %3 options.")
                              .arg(QDir::toNativeSeparators(filePath)).arg(lineNr).arg(markerQtcReplace);
                ok = false;
                return QString();
            }
            const QString replaceWhat = replaceXWithY.at(0).trimmed();
            const QString replaceWith = replaceXWithY.at(1).trimmed();
            if (!m_replacementVariables.contains(replaceWith)) {
                qWarning().nospace() << QString::fromLatin1("Error in %1:%2. Unknown %3 option '%4'.")
                              .arg(QDir::toNativeSeparators(filePath)).arg(lineNr).arg(markerQtcReplace).arg(replaceWith);
                ok = false;
                return QString();
            }
            line = tsIn.readLine(); // Following line which is to be patched.
            lineNr++;
            if (line.indexOf(replaceWhat) < 0) {
                qWarning().nospace() << QString::fromLatin1("Error in %1:%2. Replacement '%3' not found.")
                              .arg(QDir::toNativeSeparators(filePath)).arg(lineNr).arg(replaceWhat);
                ok = false;
                return QString();
            }
            line.replace(replaceWhat, m_replacementVariables.value(replaceWith));
        }
        if (!line.isNull())
            tsOut << line << endl;
        lineNr++;
    } while (!line.isNull());

    ok = true;
    return QString::fromUtf8(adaptedTemplate);
}

bool QmlApp::addTemplate(const QString &sourceDirectory,
                             const QString &templateFileName,
                             const QString &tragetDirectory,
                             const QString &targetFileName,
                             Core::GeneratedFiles *files,
                             QString *errorMessage) const
{
    bool fileIsReadable;
    Core::GeneratedFile file(tragetDirectory + QLatin1Char('/') + targetFileName);

    const QString &data = readAndAdaptTemplateFile(sourceDirectory + QLatin1Char('/') + templateFileName, fileIsReadable);

    if (!fileIsReadable) {
        if (errorMessage)
            *errorMessage = QCoreApplication::translate("QmlApplicationWizard", "Failed to read %1 template.").arg(templateFileName);
        return false;
    }

    file.setContents(data);
    files->append(file);

    return true;
}

bool QmlApp::addBinaryFile(const QString &sourceDirectory,
                               const QString &templateFileName,
                               const QString &tragetDirectory,
                               const QString &targetFileName,
                               Core::GeneratedFiles *files,
                               QString *errorMessage) const
{
    bool fileIsReadable;

    Core::GeneratedFile file(tragetDirectory + targetFileName);
    file.setBinary(true);

    const QByteArray &data = readFile(sourceDirectory + QLatin1Char('/') + templateFileName, fileIsReadable);

    if (!fileIsReadable) {
        if (errorMessage)
            *errorMessage = QCoreApplication::translate("QmlApplicationWizard", "Failed to read file %1.").arg(templateFileName);
        return false;
    }

    file.setBinaryContents(data);
    files->append(file);

    return true;
}

QString QmlApp::renameQmlFile(const QString &fileName) {
    if (fileName == QLatin1String("main.qml"))
        return mainQmlFileName();
    return fileName;
}

Core::GeneratedFiles QmlApp::generateFiles(QString *errorMessage)
{

    Core::GeneratedFiles files;

    QTC_ASSERT(errorMessage, return files);

    errorMessage->clear();
    setReplacementVariables();
    const QFileInfoList templateFiles = allFilesRecursive(templateDirectory());

    foreach (const QFileInfo &templateFile, templateFiles) {
        const QString targetSubDirectory = templateFile.path().mid(templateDirectory().length());
        const QString targetDirectory = projectDirectory() + targetSubDirectory + QLatin1Char('/');

        QString targetFileName = templateFile.fileName();

        if (templateFile.fileName() == QLatin1String("main.pro")) {
            targetFileName = projectName() + QLatin1String(".pro");
            m_creatorFileName = Core::BaseFileWizard::buildFileName(projectDirectory(),
                                                                    projectName(),
                                                                    QLatin1String("pro"));
        } else  if (templateFile.fileName() == QLatin1String("main.qmlproject")) {
            targetFileName = projectName() + QLatin1String(".qmlproject");
            m_creatorFileName = Core::BaseFileWizard::buildFileName(projectDirectory(),
                                                                    projectName(),
                                                                    QLatin1String("qmlproject"));
        } else if (templateFile.fileName() == QLatin1String("main.qbp")) {
            targetFileName = projectName() + QLatin1String(".qbp");
        } else if (targetFileName == QLatin1String("template.xml")
                   || targetFileName == QLatin1String("template.png")) {
            continue;
        } else {
            targetFileName = renameQmlFile(templateFile.fileName());
        }

        if (binaryFiles().contains(templateFile.suffix())) {
            bool canAddBinaryFile = addBinaryFile(templateFile.absolutePath(),
                                                  templateFile.fileName(),
                                                  targetDirectory,
                                                  targetFileName,
                                                  &files,
                                                  errorMessage);
            if (!canAddBinaryFile)
                return Core::GeneratedFiles();
        } else {
            bool canAddTemplate = addTemplate(templateFile.absolutePath(),
                                              templateFile.fileName(),
                                              targetDirectory,
                                              targetFileName,
                                              &files,
                                              errorMessage);
            if (!canAddTemplate)
                return Core::GeneratedFiles();

            if (templateFile.fileName() == QLatin1String("main.pro")) {
                files.last().setAttributes(Core::GeneratedFile::OpenProjectAttribute);
            } else if (templateFile.fileName() == QLatin1String("main.qmlproject")) {
                files.last().setAttributes(Core::GeneratedFile::OpenProjectAttribute);
            } else if (templateFile.fileName() == m_templateInfo.openFile) {
                files.last().setAttributes(Core::GeneratedFile::OpenEditorAttribute);
            }
        }
    }

    return files;
}

void QmlApp::setReplacementVariables()
{
    m_replacementVariables.clear();

    m_replacementVariables.insert(QLatin1String("main"), mainQmlFileName());
    m_replacementVariables.insert(QLatin1String("projectName"), projectName());
}

} // namespace Internal
} // namespace QmlProjectManager
