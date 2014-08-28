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

#include "exampleslistmodel.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QUrl>
#include <QXmlStreamReader>

#include <coreplugin/helpmanager.h>
#include <coreplugin/icore.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtversionmanager.h>
#include <utils/environment.h>
#include <utils/qtcassert.h>
#include <utils/algorithm.h>

#include <algorithm>

namespace QtSupport {
namespace Internal {

static bool debugExamples()
{
    static bool isDebugging = !qgetenv("QTC_DEBUG_EXAMPLESMODEL").isEmpty();
    return isDebugging;
}

static const char kSelectedExampleSetKey[] = "WelcomePage/SelectedExampleSet";

void ExampleSetModel::writeCurrentIdToSettings(int currentIndex) const
{
    QSettings *settings = Core::ICore::settings();
    settings->setValue(QLatin1String(kSelectedExampleSetKey), getId(currentIndex));
}

int ExampleSetModel::readCurrentIndexFromSettings() const
{
    QVariant id = Core::ICore::settings()->value(QLatin1String(kSelectedExampleSetKey));
    for (int i=0; i < rowCount(); i++) {
        if (id == getId(i))
            return i;
    }
    return -1;
}

ExampleSetModel::ExampleSetModel(ExamplesListModel *examplesModel, QObject *parent) :
    QStandardItemModel(parent),
    examplesModel(examplesModel)
{
}

void ExampleSetModel::update()
{
    beginResetModel();
    clear();

    QSet<QString> extraManifestDirs;
    QList<ExamplesListModel::ExtraExampleSet> extraExampleSets = examplesModel->extraExampleSets();
    for (int i = 0; i < extraExampleSets.size(); ++i)  {
        ExamplesListModel::ExtraExampleSet set = extraExampleSets.at(i);
        QStandardItem *newItem = new QStandardItem();
        newItem->setData(set.displayName, Qt::UserRole + 1);
        newItem->setData(QVariant(), Qt::UserRole + 2);
        newItem->setData(i, Qt::UserRole + 3);
        appendRow(newItem);

        extraManifestDirs.insert(set.manifestPath);
    }

    QList<BaseQtVersion *> qtVersions = examplesModel->qtVersions();

    foreach (BaseQtVersion *version, qtVersions) {
        // sanitize away qt versions that have already been added through extra sets
        if (extraManifestDirs.contains(version->documentationPath())) {
            if (debugExamples()) {
                qWarning() << "Not showing Qt version because manifest path is already added through InstalledExamples settings:"
                           << version->displayName();
            }
            continue;
        }
        QStandardItem *newItem = new QStandardItem();
        newItem->setData(version->displayName(), Qt::UserRole + 1);
        newItem->setData(version->uniqueId(), Qt::UserRole + 2);
        newItem->setData(QVariant(), Qt::UserRole + 3);
        appendRow(newItem);
    }
    endResetModel();
}

int ExampleSetModel::indexForQtVersion(BaseQtVersion *qtVersion) const
{
    // return either the entry with the same QtId, or an extra example set with same path

    if (!qtVersion)
        return -1;

    // check for Qt version
    for (int i = 0; i < rowCount(); ++i) {
        if (getType(i) == QtExampleSet && getQtId(i) == qtVersion->uniqueId())
            return i;
    }
    // check for extra set
    const QList<ExamplesListModel::ExtraExampleSet> &extraExamples
            = examplesModel->extraExampleSets();
    const QString &documentationPath = qtVersion->documentationPath();
    for (int i = 0; i < rowCount(); ++i) {
        if (getType(i) == ExtraExampleSet
                && extraExamples.at(getExtraExampleSetIndex(i)).manifestPath == documentationPath)
            return i;
    }
    return -1;
}

QVariant ExampleSetModel::getDisplayName(int i) const
{
    if (i < 0 || i >= rowCount())
        return QVariant();
    return data(index(i, 0), Qt::UserRole + 1);
}

// id is either the Qt version uniqueId, or the display name of the extra example set
QVariant ExampleSetModel::getId(int i) const
{
    if (i < 0 || i >= rowCount())
        return QVariant();
    QModelIndex modelIndex = index(i, 0);
    QVariant variant = data(modelIndex, Qt::UserRole + 2);
    if (variant.isValid()) // set from qt version
        return variant;
    return getDisplayName(i);
}

ExampleSetModel::ExampleSetType ExampleSetModel::getType(int i) const
{
    if (i < 0 || i >= rowCount())
        return InvalidExampleSet;
    QModelIndex modelIndex = index(i, 0);
    QVariant variant = data(modelIndex, Qt::UserRole + 2); /*Qt version uniqueId*/
    if (variant.isValid())
        return QtExampleSet;
    return ExtraExampleSet;
}

int ExampleSetModel::getQtId(int i) const
{
    QTC_ASSERT(i >= 0, return -1);
    QModelIndex modelIndex = index(i, 0);
    QVariant variant = data(modelIndex, Qt::UserRole + 2);
    QTC_ASSERT(variant.isValid(), return -1);
    QTC_ASSERT(variant.canConvert<int>(), return -1);
    return variant.toInt();
}

int ExampleSetModel::getExtraExampleSetIndex(int i) const
{
    QTC_ASSERT(i >= 0, return -1);
    QModelIndex modelIndex = index(i, 0);
    QVariant variant = data(modelIndex, Qt::UserRole + 3);
    QTC_ASSERT(variant.isValid(), return -1);
    QTC_ASSERT(variant.canConvert<int>(), return -1);
    return variant.toInt();
}

QHash<int, QByteArray> ExampleSetModel::roleNames() const
{
    QHash<int, QByteArray> roleNames;
    roleNames[Qt::UserRole + 1] = "text";
    roleNames[Qt::UserRole + 2] = "QtId";
    roleNames[Qt::UserRole + 3] = "extraSetIndex";
    return roleNames;
}

ExamplesListModel::ExamplesListModel(QObject *parent) :
    QAbstractListModel(parent),
    m_exampleSetModel(new ExampleSetModel(this, this)),
    m_selectedExampleSetIndex(-1)
{
    // read extra example sets settings
    QSettings *settings = Core::ICore::settings();
    QStringList list = settings->value(QLatin1String("Help/InstalledExamples"),
                                       QStringList()).toStringList();
    if (debugExamples())
        qWarning() << "Reading Help/InstalledExamples from settings:" << list;
    foreach (const QString &item, list) {
        const QStringList &parts = item.split(QLatin1Char('|'));
        if (parts.size() < 3) {
            if (debugExamples())
                qWarning() << "Item" << item << "has less than 3 parts (separated by '|'):" << parts;
            continue;
        }
        ExtraExampleSet set;
        set.displayName = parts.at(0);
        set.manifestPath = parts.at(1);
        set.examplesPath = parts.at(2);
        QFileInfo fi(set.manifestPath);
        if (!fi.isDir() || !fi.isReadable()) {
            if (debugExamples())
                qWarning() << "Manifest path " << set.manifestPath << "is not a readable directory, ignoring";
            continue;
        }
        m_extraExampleSets.append(set);
        if (debugExamples()) {
            qWarning() << "Adding examples set displayName=" << set.displayName
                       << ", manifestPath=" << set.manifestPath
                       << ", examplesPath=" << set.examplesPath;
        }
    }
}

static QString fixStringForTags(const QString &string)
{
    QString returnString = string;
    returnString.remove(QLatin1String("<i>"));
    returnString.remove(QLatin1String("</i>"));
    returnString.remove(QLatin1String("<tt>"));
    returnString.remove(QLatin1String("</tt>"));
    return returnString;
}

static QStringList trimStringList(const QStringList &stringlist)
{
    return Utils::transform(stringlist, [](const QString &str) { return str.trimmed(); });
}

static QString relativeOrInstallPath(const QString &path, const QString &manifestPath,
                                     const QString &installPath)
{
    const QChar slash = QLatin1Char('/');
    const QString relativeResolvedPath = manifestPath + slash + path;
    const QString installResolvedPath = installPath + slash + path;
    if (QFile::exists(relativeResolvedPath))
        return relativeResolvedPath;
    if (QFile::exists(installResolvedPath))
        return installResolvedPath;
    // doesn't exist, just return relative
    return relativeResolvedPath;
}

static bool isValidExampleOrDemo(ExampleItem &item)
{
    static QString invalidPrefix = QLatin1String("qthelp:////"); /* means that the qthelp url
                                                                    doesn't have any namespace */
    QString reason;
    bool ok = true;
    if (!item.hasSourceCode || !QFileInfo(item.projectPath).exists()) {
        ok = false;
        reason = QString::fromLatin1("projectPath \"%1\" empty or does not exist").arg(item.projectPath);
    } else if (item.imageUrl.startsWith(invalidPrefix) || !QUrl(item.imageUrl).isValid()) {
        ok = false;
        reason = QString::fromLatin1("imageUrl \"%1\" not valid").arg(item.imageUrl);
    } else if (!item.docUrl.isEmpty()
             && (item.docUrl.startsWith(invalidPrefix) || !QUrl(item.docUrl).isValid())) {
        ok = false;
        reason = QString::fromLatin1("docUrl \"%1\" non-empty but not valid").arg(item.docUrl);
    }
    if (!ok) {
        item.tags.append(QLatin1String("broken"));
        if (debugExamples())
            qWarning() << QString::fromLatin1("ERROR: Item \"%1\" broken: %2").arg(item.name, reason);
    }
    if (debugExamples() && item.description.isEmpty())
        qWarning() << QString::fromLatin1("WARNING: Item \"%1\" has no description").arg(item.name);
    return ok || debugExamples();
}

void ExamplesListModel::parseExamples(QXmlStreamReader *reader,
    const QString &projectsOffset, const QString &examplesInstallPath)
{
    ExampleItem item;
    const QChar slash = QLatin1Char('/');
    while (!reader->atEnd()) {
        switch (reader->readNext()) {
        case QXmlStreamReader::StartElement:
            if (reader->name() == QLatin1String("example")) {
                item = ExampleItem();
                item.type = Example;
                QXmlStreamAttributes attributes = reader->attributes();
                item.name = attributes.value(QLatin1String("name")).toString();
                item.projectPath = attributes.value(QLatin1String("projectPath")).toString();
                item.hasSourceCode = !item.projectPath.isEmpty();
                item.projectPath = relativeOrInstallPath(item.projectPath, projectsOffset, examplesInstallPath);
                item.imageUrl = attributes.value(QLatin1String("imageUrl")).toString();
                item.docUrl = attributes.value(QLatin1String("docUrl")).toString();

                if (attributes.hasAttribute(QLatin1String("isHighlighted")))
                    item.isHighlighted = attributes.value(QLatin1String("isHighlighted")).toString() == QLatin1String("true");

            } else if (reader->name() == QLatin1String("fileToOpen")) {
                const QString mainFileAttribute = reader->attributes().value(
                            QLatin1String("mainFile")).toString();
                const QString filePath = relativeOrInstallPath(
                            reader->readElementText(QXmlStreamReader::ErrorOnUnexpectedElement),
                            projectsOffset, examplesInstallPath);
                item.filesToOpen.append(filePath);
                if (mainFileAttribute.compare(QLatin1String("true"), Qt::CaseInsensitive) == 0)
                    item.mainFile = filePath;
            } else if (reader->name() == QLatin1String("description")) {
                item.description = fixStringForTags(reader->readElementText(QXmlStreamReader::ErrorOnUnexpectedElement));
            } else if (reader->name() == QLatin1String("dependency")) {
                item.dependencies.append(projectsOffset + slash + reader->readElementText(QXmlStreamReader::ErrorOnUnexpectedElement));
            } else if (reader->name() == QLatin1String("tags")) {
                item.tags = trimStringList(reader->readElementText(QXmlStreamReader::ErrorOnUnexpectedElement).split(QLatin1Char(','), QString::SkipEmptyParts));
            } else if (reader->name() == QLatin1String("platforms")) {
                item.platforms = trimStringList(reader->readElementText(QXmlStreamReader::ErrorOnUnexpectedElement).split(QLatin1Char(','), QString::SkipEmptyParts));
        }
            break;
        case QXmlStreamReader::EndElement:
            if (reader->name() == QLatin1String("example")) {
                if (isValidExampleOrDemo(item))
                    m_exampleItems.append(item);
            } else if (reader->name() == QLatin1String("examples")) {
                return;
            }
            break;
        default: // nothing
            break;
        }
    }
}

void ExamplesListModel::parseDemos(QXmlStreamReader *reader,
    const QString &projectsOffset, const QString &demosInstallPath)
{
    ExampleItem item;
    const QChar slash = QLatin1Char('/');
    while (!reader->atEnd()) {
        switch (reader->readNext()) {
        case QXmlStreamReader::StartElement:
            if (reader->name() == QLatin1String("demo")) {
                item = ExampleItem();
                item.type = Demo;
                QXmlStreamAttributes attributes = reader->attributes();
                item.name = attributes.value(QLatin1String("name")).toString();
                item.projectPath = attributes.value(QLatin1String("projectPath")).toString();
                item.hasSourceCode = !item.projectPath.isEmpty();
                item.projectPath = relativeOrInstallPath(item.projectPath, projectsOffset, demosInstallPath);
                item.imageUrl = attributes.value(QLatin1String("imageUrl")).toString();
                item.docUrl = attributes.value(QLatin1String("docUrl")).toString();

                if (attributes.hasAttribute(QLatin1String("isHighlighted")))
                    item.isHighlighted = attributes.value(QLatin1String("isHighlighted")).toString() == QLatin1String("true");

            } else if (reader->name() == QLatin1String("fileToOpen")) {
                item.filesToOpen.append(relativeOrInstallPath(reader->readElementText(QXmlStreamReader::ErrorOnUnexpectedElement),
                                                              projectsOffset, demosInstallPath));
            } else if (reader->name() == QLatin1String("description")) {
                item.description =  fixStringForTags(reader->readElementText(QXmlStreamReader::ErrorOnUnexpectedElement));
            } else if (reader->name() == QLatin1String("dependency")) {
                item.dependencies.append(projectsOffset + slash + reader->readElementText(QXmlStreamReader::ErrorOnUnexpectedElement));
            } else if (reader->name() == QLatin1String("tags")) {
                item.tags = reader->readElementText(QXmlStreamReader::ErrorOnUnexpectedElement).split(QLatin1Char(','));
            }
            break;
        case QXmlStreamReader::EndElement:
            if (reader->name() == QLatin1String("demo")) {
                if (isValidExampleOrDemo(item))
                    m_exampleItems.append(item);
            } else if (reader->name() == QLatin1String("demos")) {
                return;
            }
            break;
        default: // nothing
            break;
        }
    }
}

void ExamplesListModel::parseTutorials(QXmlStreamReader *reader, const QString &projectsOffset)
{
    ExampleItem item;
    const QChar slash = QLatin1Char('/');
    while (!reader->atEnd()) {
        switch (reader->readNext()) {
        case QXmlStreamReader::StartElement:
            if (reader->name() == QLatin1String("tutorial")) {
                item = ExampleItem();
                item.type = Tutorial;
                QXmlStreamAttributes attributes = reader->attributes();
                item.name = attributes.value(QLatin1String("name")).toString();
                item.projectPath = attributes.value(QLatin1String("projectPath")).toString();
                item.hasSourceCode = !item.projectPath.isEmpty();
                item.projectPath.prepend(slash);
                item.projectPath.prepend(projectsOffset);
                if (attributes.hasAttribute(QLatin1String("imageUrl")))
                    item.imageUrl = attributes.value(QLatin1String("imageUrl")).toString();
                if (attributes.hasAttribute(QLatin1String("docUrl")))
                    item.docUrl = attributes.value(QLatin1String("docUrl")).toString();
                if (attributes.hasAttribute(QLatin1String("isVideo")))
                    item.isVideo = attributes.value(QLatin1String("isVideo")).toString() == QLatin1String("true");
                if (attributes.hasAttribute(QLatin1String("videoUrl")))
                    item.videoUrl = attributes.value(QLatin1String("videoUrl")).toString();
                if (attributes.hasAttribute(QLatin1String("videoLength")))
                    item.videoLength = attributes.value(QLatin1String("videoLength")).toString();
            } else if (reader->name() == QLatin1String("fileToOpen")) {
                item.filesToOpen.append(projectsOffset + slash + reader->readElementText(QXmlStreamReader::ErrorOnUnexpectedElement));
            } else if (reader->name() == QLatin1String("description")) {
                item.description =  fixStringForTags(reader->readElementText(QXmlStreamReader::ErrorOnUnexpectedElement));
            } else if (reader->name() == QLatin1String("dependency")) {
                item.dependencies.append(projectsOffset + slash + reader->readElementText(QXmlStreamReader::ErrorOnUnexpectedElement));
            } else if (reader->name() == QLatin1String("tags")) {
                item.tags = reader->readElementText(QXmlStreamReader::ErrorOnUnexpectedElement).split(QLatin1Char(','));
            }
            break;
        case QXmlStreamReader::EndElement:
            if (reader->name() == QLatin1String("tutorial"))
                m_exampleItems.append(item);
            else if (reader->name() == QLatin1String("tutorials"))
                return;
            break;
        default: // nothing
            break;
        }
    }
}

void ExamplesListModel::updateExamples()
{
    QString examplesInstallPath;
    QString demosInstallPath;

    QStringList sources = exampleSources(&examplesInstallPath, &demosInstallPath);

    beginResetModel();
    m_exampleItems.clear();

    foreach (const QString &exampleSource, sources) {
        QFile exampleFile(exampleSource);
        if (!exampleFile.open(QIODevice::ReadOnly)) {
            if (debugExamples())
                qWarning() << "ERROR: Could not open file" << exampleSource;
            continue;
        }

        QFileInfo fi(exampleSource);
        QString offsetPath = fi.path();
        QDir examplesDir(offsetPath);
        QDir demosDir(offsetPath);

        if (debugExamples())
            qWarning() << QString::fromLatin1("Reading file \"%1\"...").arg(fi.absoluteFilePath());
        QXmlStreamReader reader(&exampleFile);
        while (!reader.atEnd())
            switch (reader.readNext()) {
            case QXmlStreamReader::StartElement:
                if (reader.name() == QLatin1String("examples"))
                    parseExamples(&reader, examplesDir.path(), examplesInstallPath);
                else if (reader.name() == QLatin1String("demos"))
                    parseDemos(&reader, demosDir.path(), demosInstallPath);
                else if (reader.name() == QLatin1String("tutorials"))
                    parseTutorials(&reader, examplesDir.path());
                break;
            default: // nothing
                break;
            }

        if (reader.hasError() && debugExamples())
            qWarning() << QString::fromLatin1("ERROR: Could not parse file as XML document (%1)").arg(exampleSource);
    }
    endResetModel();
}

void ExamplesListModel::updateQtVersions()
{
    QList<BaseQtVersion*> versions = QtVersionManager::validVersions();

    QMutableListIterator<BaseQtVersion*> iter(versions);
    while (iter.hasNext()) {
        BaseQtVersion *version = iter.next();
        if (!version->hasExamples()
                && !version->hasDemos())
            iter.remove();
    }

    // prioritize default qt version
    ProjectExplorer::Kit *defaultKit = ProjectExplorer::KitManager::defaultKit();
    BaseQtVersion *defaultVersion = QtKitInformation::qtVersion(defaultKit);
    if (defaultVersion && versions.contains(defaultVersion))
        versions.move(versions.indexOf(defaultVersion), 0);

    if (m_qtVersions == versions && m_selectedExampleSetIndex >= 0)
        return;

    m_qtVersions = versions;

    m_exampleSetModel->update();

    int currentIndex = m_selectedExampleSetIndex;
    if (currentIndex < 0) // reset from settings
        currentIndex = m_exampleSetModel->readCurrentIndexFromSettings();

    ExampleSetModel::ExampleSetType currentType
            = m_exampleSetModel->getType(currentIndex);

    if (currentType == ExampleSetModel::InvalidExampleSet) {
        // select examples corresponding to 'highest' Qt version
        BaseQtVersion *highestQt = findHighestQtVersion();
        currentIndex = m_exampleSetModel->indexForQtVersion(highestQt);
    } else if (currentType == ExampleSetModel::QtExampleSet) {
        // try to select the previously selected Qt version, or
        // select examples corresponding to 'highest' Qt version
        int currentQtId = m_exampleSetModel->getQtId(currentIndex);
        BaseQtVersion *newQtVersion = Utils::findOrDefault(m_qtVersions,
                                                    Utils::equal(&BaseQtVersion::uniqueId, currentQtId));

        if (!newQtVersion)
            newQtVersion = findHighestQtVersion();
        currentIndex = m_exampleSetModel->indexForQtVersion(newQtVersion);
    } // nothing to do for extra example sets
    selectExampleSet(currentIndex);
}

BaseQtVersion *ExamplesListModel::findHighestQtVersion() const
{
    QList<BaseQtVersion *> versions = qtVersions();

    BaseQtVersion *newVersion = 0;

    foreach (BaseQtVersion *version, versions) {
        if (!newVersion) {
            newVersion = version;
        } else {
            if (version->qtVersion() > newVersion->qtVersion()) {
                newVersion = version;
            } else if (version->qtVersion() == newVersion->qtVersion()
                       && version->uniqueId() < newVersion->uniqueId()) {
                newVersion = version;
            }
        }
    }

    if (!newVersion && !versions.isEmpty())
        newVersion = versions.first();

    if (!newVersion)
        return 0;

    return newVersion;
}

QStringList ExamplesListModel::exampleSources(QString *examplesInstallPath, QString *demosInstallPath)
{
    QStringList sources;
    QString resourceDir = Core::ICore::resourcePath() + QLatin1String("/welcomescreen/");

    // Qt Creator shipped tutorials
    sources << (resourceDir + QLatin1String("/qtcreator_tutorials.xml"));

    QString examplesPath;
    QString demosPath;
    QString manifestScanPath;

    ExampleSetModel::ExampleSetType currentType
            = m_exampleSetModel->getType(m_selectedExampleSetIndex);
    if (currentType == ExampleSetModel::ExtraExampleSet) {
        int index = m_exampleSetModel->getExtraExampleSetIndex(m_selectedExampleSetIndex);
        ExtraExampleSet exampleSet = m_extraExampleSets.at(index);
        manifestScanPath = exampleSet.manifestPath;
        examplesPath = exampleSet.examplesPath;
        demosPath = exampleSet.examplesPath;
    } else if (currentType == ExampleSetModel::QtExampleSet) {
        int qtId = m_exampleSetModel->getQtId(m_selectedExampleSetIndex);
        foreach (BaseQtVersion *version, qtVersions()) {
            if (version->uniqueId() == qtId) {
                manifestScanPath = version->documentationPath();
                examplesPath = version->examplesPath();
                demosPath = version->demosPath();
                break;
            }
        }
    }
    if (!manifestScanPath.isEmpty()) {
        // search for examples-manifest.xml, demos-manifest.xml in <path>/*/
        QDir dir = QDir(manifestScanPath);
        const QStringList examplesPattern(QLatin1String("examples-manifest.xml"));
        const QStringList demosPattern(QLatin1String("demos-manifest.xml"));
        QFileInfoList fis;
        foreach (QFileInfo subDir, dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            fis << QDir(subDir.absoluteFilePath()).entryInfoList(examplesPattern);
            fis << QDir(subDir.absoluteFilePath()).entryInfoList(demosPattern);
        }
        foreach (const QFileInfo &fi, fis)
            sources.append(fi.filePath());
    }
    if (examplesInstallPath)
        *examplesInstallPath = examplesPath;
    if (demosInstallPath)
        *demosInstallPath = demosPath;

    return sources;
}

int ExamplesListModel::rowCount(const QModelIndex &) const
{
    return m_exampleItems.size();
}

QString prefixForItem(const ExampleItem &item)
{
    if (item.isHighlighted)
        return QLatin1String("0000 ");
    return QString();
}

QVariant ExamplesListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row()+1 > m_exampleItems.count()) {
        qDebug() << Q_FUNC_INFO << "invalid index requested";
        return QVariant();
    }

    ExampleItem item = m_exampleItems.at(index.row());
    switch (role)
    {
    case Qt::DisplayRole: // for search only
        return QString(prefixForItem(item) + item.name + QLatin1Char(' ') + item.tags.join(QLatin1Char(' ')));
    case Name:
        return item.name;
    case ProjectPath:
        return item.projectPath;
    case Description:
        return item.description;
    case ImageUrl:
        return item.imageUrl;
    case DocUrl:
        return item.docUrl;
    case FilesToOpen:
        return item.filesToOpen;
    case MainFile:
        return item.mainFile;
    case Tags:
        return item.tags;
    case Difficulty:
        return item.difficulty;
    case Dependencies:
        return item.dependencies;
    case HasSourceCode:
        return item.hasSourceCode;
    case Type:
        return item.type;
    case IsVideo:
        return item.isVideo;
    case VideoUrl:
        return item.videoUrl;
    case VideoLength:
        return item.videoLength;
    case Platforms:
        return item.platforms;
    case IsHighlighted:
        return item.isHighlighted;
    default:
        qDebug() << Q_FUNC_INFO << "role type not supported";
        return QVariant();
    }
}

QHash<int, QByteArray> ExamplesListModel::roleNames() const
{
    QHash<int, QByteArray> roleNames;
    roleNames[Name] = "name";
    roleNames[ProjectPath] = "projectPath";
    roleNames[ImageUrl] = "imageUrl";
    roleNames[Description] = "description";
    roleNames[DocUrl] = "docUrl";
    roleNames[FilesToOpen] = "filesToOpen";
    roleNames[MainFile] = "mainFile";
    roleNames[Tags] = "tags";
    roleNames[Difficulty] = "difficulty";
    roleNames[Type] = "type";
    roleNames[HasSourceCode] = "hasSourceCode";
    roleNames[Dependencies] = "dependencies";
    roleNames[IsVideo] = "isVideo";
    roleNames[VideoUrl] = "videoUrl";
    roleNames[VideoLength] = "videoLength";
    roleNames[Platforms] = "platforms";
    roleNames[IsHighlighted] = "isHighlighted";
    return roleNames;
}

void ExamplesListModel::update()
{
    updateQtVersions();
    updateExamples();
}

int ExamplesListModel::selectedExampleSet() const
{
    return m_selectedExampleSetIndex;
}

void ExamplesListModel::selectExampleSet(int index)
{
    if (index != m_selectedExampleSetIndex) {
        m_selectedExampleSetIndex = index;
        m_exampleSetModel->writeCurrentIdToSettings(m_selectedExampleSetIndex);
        updateExamples();
        emit selectedExampleSetChanged();
    }
}

ExamplesListModelFilter::ExamplesListModelFilter(ExamplesListModel *sourceModel, QObject *parent) :
    QSortFilterProxyModel(parent),
    m_showTutorialsOnly(true),
    m_sourceModel(sourceModel),
    m_timerId(0),
    m_blockIndexUpdate(false),
    m_qtVersionManagerInitialized(false),
    m_helpManagerInitialized(false),
    m_initalized(false),
    m_exampleDataRequested(false)
{
    // initialization hooks
    connect(QtVersionManager::instance(), SIGNAL(qtVersionsLoaded()),
            this, SLOT(qtVersionManagerLoaded()));
    connect(Core::HelpManager::instance(), SIGNAL(setupFinished()),
            this, SLOT(helpManagerInitialized()));

    connect(this, SIGNAL(showTutorialsOnlyChanged()), SLOT(updateFilter()));

    connect(m_sourceModel, SIGNAL(selectedExampleSetChanged()), this, SIGNAL(exampleSetIndexChanged()));

    setSourceModel(m_sourceModel);
}

void ExamplesListModelFilter::updateFilter()
{
    ExamplesListModel *exampleListModel = qobject_cast<ExamplesListModel*>(sourceModel());
    if (exampleListModel) {
        exampleListModel->beginReset();
        invalidateFilter();
        exampleListModel->endReset();
    }
}

bool containsSubString(const QStringList &list, const QString &substr, Qt::CaseSensitivity cs)
{
    return Utils::contains(list, [&substr, &cs](const QString &elem) {
        return elem.contains(substr, cs);
    });
}

bool ExamplesListModelFilter::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    if (m_showTutorialsOnly) {
        int type = sourceModel()->index(sourceRow, 0, sourceParent).data(Type).toInt();
        if (type != Tutorial)
            return false;
    }

    if (!m_showTutorialsOnly) {
        int type = sourceModel()->index(sourceRow, 0, sourceParent).data(Type).toInt();
        if (type != Example && type != Demo)
            return false;
    }

    const QStringList tags = sourceModel()->index(sourceRow, 0, sourceParent).data(Tags).toStringList();

    if (!m_filterTags.isEmpty()) {
        return Utils::allOf(m_filterTags, [tags](const QString &filterTag) {
            return tags.contains(filterTag);
        });
    }

    if (!m_searchString.isEmpty()) {
        const QString description = sourceModel()->index(sourceRow, 0, sourceParent).data(Description).toString();
        const QString name = sourceModel()->index(sourceRow, 0, sourceParent).data(Name).toString();

        foreach (const QString &subString, m_searchString) {
            bool wordMatch = false;
            wordMatch |= (bool)name.contains(subString, Qt::CaseInsensitive);
            if (wordMatch)
                continue;
            wordMatch |= containsSubString(tags, subString, Qt::CaseInsensitive);
            if (wordMatch)
                continue;
            wordMatch |= (bool)description.contains(subString, Qt::CaseInsensitive);
            if (!wordMatch)
                return false;
        }
    }

    return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
}

int ExamplesListModelFilter::rowCount(const QModelIndex &parent) const
{
    exampleDataRequested();
    return QSortFilterProxyModel::rowCount(parent);
}

QVariant ExamplesListModelFilter::data(const QModelIndex &index, int role) const
{
    exampleDataRequested();
    return QSortFilterProxyModel::data(index, role);
}

QAbstractItemModel* ExamplesListModelFilter::exampleSetModel()
{
    return m_sourceModel->exampleSetModel();
}

void ExamplesListModelFilter::filterForExampleSet(int index)
{
    if (m_blockIndexUpdate || !m_initalized)
        return;

    m_sourceModel->selectExampleSet(index);
}

void ExamplesListModelFilter::setShowTutorialsOnly(bool showTutorialsOnly)
{
    m_showTutorialsOnly = showTutorialsOnly;
    emit showTutorialsOnlyChanged();
}

void ExamplesListModelFilter::handleQtVersionsChanged()
{
    m_blockIndexUpdate = true;
    m_sourceModel->update();
    m_blockIndexUpdate = false;
}

void ExamplesListModelFilter::qtVersionManagerLoaded()
{
    m_qtVersionManagerInitialized = true;
    tryToInitialize();
}

void ExamplesListModelFilter::helpManagerInitialized()
{
    m_helpManagerInitialized = true;
    tryToInitialize();
}

void ExamplesListModelFilter::exampleDataRequested() const
{
    ExamplesListModelFilter *that = const_cast<ExamplesListModelFilter *>(this);
    that->m_exampleDataRequested = true;
    that->tryToInitialize();
}

void ExamplesListModelFilter::tryToInitialize()
{
    if (!m_initalized
            && m_qtVersionManagerInitialized && m_helpManagerInitialized && m_exampleDataRequested) {
        m_initalized = true;
        connect(QtVersionManager::instance(), SIGNAL(qtVersionsChanged(QList<int>,QList<int>,QList<int>)),
                this, SLOT(handleQtVersionsChanged()));
        connect(ProjectExplorer::KitManager::instance(), SIGNAL(defaultkitChanged()),
                this, SLOT(handleQtVersionsChanged()));
        handleQtVersionsChanged();
    }
}

void ExamplesListModelFilter::delayedUpdateFilter()
{
    if (m_timerId != 0)
        killTimer(m_timerId);

    m_timerId = startTimer(320);
}

int ExamplesListModelFilter::exampleSetIndex() const
{
    return m_sourceModel->selectedExampleSet();
}

void ExamplesListModelFilter::timerEvent(QTimerEvent *timerEvent)
{
    if (m_timerId == timerEvent->timerId()) {
        updateFilter();
        killTimer(m_timerId);
        m_timerId = 0;
    }
}

struct SearchStringLexer
{
    QString code;
    const QChar *codePtr;
    QChar yychar;
    QString yytext;

    enum TokenKind {
        END_OF_STRING = 0,
        TAG,
        STRING_LITERAL,
        UNKNOWN
    };

    inline void yyinp() { yychar = *codePtr++; }

    SearchStringLexer(const QString &code)
        : code(code)
        , codePtr(code.unicode())
        , yychar(QLatin1Char(' ')) { }

    int operator()() { return yylex(); }

    int yylex() {
        while (yychar.isSpace())
            yyinp(); // skip all the spaces

        yytext.clear();

        if (yychar.isNull())
            return END_OF_STRING;

        QChar ch = yychar;
        yyinp();

        switch (ch.unicode()) {
        case '"':
        case '\'':
        {
            const QChar quote = ch;
            yytext.clear();
            while (!yychar.isNull()) {
                if (yychar == quote) {
                    yyinp();
                    break;
                }
                if (yychar == QLatin1Char('\\')) {
                    yyinp();
                    switch (yychar.unicode()) {
                    case '"': yytext += QLatin1Char('"'); yyinp(); break;
                    case '\'': yytext += QLatin1Char('\''); yyinp(); break;
                    case '\\': yytext += QLatin1Char('\\'); yyinp(); break;
                    }
                } else {
                    yytext += yychar;
                    yyinp();
                }
            }
            return STRING_LITERAL;
        }

        default:
            if (ch.isLetterOrNumber() || ch == QLatin1Char('_')) {
                yytext.clear();
                yytext += ch;
                while (yychar.isLetterOrNumber() || yychar == QLatin1Char('_')) {
                    yytext += yychar;
                    yyinp();
                }
                if (yychar == QLatin1Char(':') && yytext == QLatin1String("tag")) {
                    yyinp();
                    return TAG;
                }
                return STRING_LITERAL;
            }
        }

        yytext += ch;
        return UNKNOWN;
    }
};

void ExamplesListModelFilter::parseSearchString(const QString &arg)
{
    QStringList tags;
    QStringList searchTerms;
    SearchStringLexer lex(arg);
    bool isTag = false;
    while (int tk = lex()) {
        if (tk == SearchStringLexer::TAG) {
            isTag = true;
            searchTerms.append(lex.yytext);
        }

        if (tk == SearchStringLexer::STRING_LITERAL) {
            if (isTag) {
                searchTerms.pop_back();
                tags.append(lex.yytext);
                isTag = false;
            } else {
                searchTerms.append(lex.yytext);
            }
        }
    }

    setSearchStrings(searchTerms);
    setFilterTags(tags);
    delayedUpdateFilter();
}

} // namespace Internal
} // namespace QtSupport
