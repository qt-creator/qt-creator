/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
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

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/qtcassert.h>

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

ExampleSetModel::ExampleSetModel()
{
    // read extra example sets settings
    QSettings *settings = Core::ICore::settings();
    const QStringList list = settings->value("Help/InstalledExamples", QStringList()).toStringList();
    if (debugExamples())
        qWarning() << "Reading Help/InstalledExamples from settings:" << list;
    for (const QString &item : list) {
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

    connect(QtVersionManager::instance(), &QtVersionManager::qtVersionsLoaded,
            this, &ExampleSetModel::qtVersionManagerLoaded);

    if (auto helpManager = Core::HelpManager::instance()) {
        connect(helpManager, &Core::HelpManager::setupFinished,
                this, &ExampleSetModel::helpManagerInitialized);
    }
}

void ExampleSetModel::recreateModel()
{
    beginResetModel();
    clear();

    QSet<QString> extraManifestDirs;
    for (int i = 0; i < m_extraExampleSets.size(); ++i)  {
        const ExtraExampleSet &set = m_extraExampleSets.at(i);
        QStandardItem *newItem = new QStandardItem();
        newItem->setData(set.displayName, Qt::DisplayRole);
        newItem->setData(set.displayName, Qt::UserRole + 1);
        newItem->setData(QVariant(), Qt::UserRole + 2);
        newItem->setData(i, Qt::UserRole + 3);
        appendRow(newItem);

        extraManifestDirs.insert(set.manifestPath);
    }

    foreach (BaseQtVersion *version, QtVersionManager::versions()) {
        // sanitize away qt versions that have already been added through extra sets
        if (extraManifestDirs.contains(version->documentationPath())) {
            if (debugExamples()) {
                qWarning() << "Not showing Qt version because manifest path is already added through InstalledExamples settings:"
                           << version->displayName();
            }
            continue;
        }
        QStandardItem *newItem = new QStandardItem();
        newItem->setData(version->displayName(), Qt::DisplayRole);
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
    const QString &documentationPath = qtVersion->documentationPath();
    for (int i = 0; i < rowCount(); ++i) {
        if (getType(i) == ExtraExampleSetType
                && m_extraExampleSets.at(getExtraExampleSetIndex(i)).manifestPath == documentationPath)
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
    return ExtraExampleSetType;
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

ExamplesListModel::ExamplesListModel(QObject *parent)
    : QAbstractListModel(parent)
{
    connect(&m_exampleSetModel, &ExampleSetModel::selectedExampleSetChanged,
            this, &ExamplesListModel::updateExamples);
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
    if (!item.hasSourceCode || !QFileInfo::exists(item.projectPath)) {
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

    QStringList sources = m_exampleSetModel.exampleSources(&examplesInstallPath, &demosInstallPath);

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

void ExampleSetModel::updateQtVersionList()
{
    QList<BaseQtVersion*> versions
            = QtVersionManager::sortVersions(
                QtVersionManager::versions(BaseQtVersion::isValidPredicate([](const BaseQtVersion *v) {
        return v->hasExamples() || v->hasDemos();
    })));

    // prioritize default qt version
    ProjectExplorer::Kit *defaultKit = ProjectExplorer::KitManager::defaultKit();
    BaseQtVersion *defaultVersion = QtKitInformation::qtVersion(defaultKit);
    if (defaultVersion && versions.contains(defaultVersion))
        versions.move(versions.indexOf(defaultVersion), 0);

    recreateModel();

    int currentIndex = m_selectedExampleSetIndex;
    if (currentIndex < 0) // reset from settings
        currentIndex = readCurrentIndexFromSettings();

    ExampleSetModel::ExampleSetType currentType = getType(currentIndex);

    if (currentType == ExampleSetModel::InvalidExampleSet) {
        // select examples corresponding to 'highest' Qt version
        BaseQtVersion *highestQt = findHighestQtVersion();
        currentIndex = indexForQtVersion(highestQt);
    } else if (currentType == ExampleSetModel::QtExampleSet) {
        // try to select the previously selected Qt version, or
        // select examples corresponding to 'highest' Qt version
        int currentQtId = getQtId(currentIndex);
        BaseQtVersion *newQtVersion = QtVersionManager::version(currentQtId);
        if (!newQtVersion)
            newQtVersion = findHighestQtVersion();
        currentIndex = indexForQtVersion(newQtVersion);
    } // nothing to do for extra example sets
    selectExampleSet(currentIndex);
    emit selectedExampleSetChanged(currentIndex);
}

BaseQtVersion *ExampleSetModel::findHighestQtVersion() const
{
    BaseQtVersion *newVersion = nullptr;
    const QList<BaseQtVersion *> versions = QtVersionManager::versions();
    for (BaseQtVersion *version : versions) {
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

    return newVersion;
}

QStringList ExampleSetModel::exampleSources(QString *examplesInstallPath, QString *demosInstallPath)
{
    QStringList sources;

    // Qt Creator shipped tutorials
    sources << ":/qtsupport/qtcreator_tutorials.xml";

    QString examplesPath;
    QString demosPath;
    QString manifestScanPath;

    ExampleSetModel::ExampleSetType currentType = getType(m_selectedExampleSetIndex);
    if (currentType == ExampleSetModel::ExtraExampleSetType) {
        int index = getExtraExampleSetIndex(m_selectedExampleSetIndex);
        ExtraExampleSet exampleSet = m_extraExampleSets.at(index);
        manifestScanPath = exampleSet.manifestPath;
        examplesPath = exampleSet.examplesPath;
        demosPath = exampleSet.examplesPath;
    } else if (currentType == ExampleSetModel::QtExampleSet) {
        int qtId = getQtId(m_selectedExampleSetIndex);
        foreach (BaseQtVersion *version, QtVersionManager::versions()) {
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
    if (!index.isValid() || index.row() >= m_exampleItems.count())
        return QVariant();

    const ExampleItem &item = m_exampleItems.at(index.row());
    switch (role)
    {
    case Qt::DisplayRole: // for search only
        return QString(prefixForItem(item) + item.name + ' ' + item.tags.join(' '));
    case Qt::UserRole:
        return QVariant::fromValue<ExampleItem>(item);
    default:
        return QVariant();
    }
}

void ExampleSetModel::selectExampleSet(int index)
{
    if (index != m_selectedExampleSetIndex) {
        m_selectedExampleSetIndex = index;
        writeCurrentIdToSettings(m_selectedExampleSetIndex);
        emit selectedExampleSetChanged(m_selectedExampleSetIndex);
    }
}

void ExampleSetModel::qtVersionManagerLoaded()
{
    m_qtVersionManagerInitialized = true;
    tryToInitialize();
}

void ExampleSetModel::helpManagerInitialized()
{
    m_helpManagerInitialized = true;
    tryToInitialize();
}


void ExampleSetModel::tryToInitialize()
{
    if (m_initalized || !m_qtVersionManagerInitialized || !m_helpManagerInitialized)
        return;

    m_initalized = true;

    connect(QtVersionManager::instance(), &QtVersionManager::qtVersionsChanged,
            this, &ExampleSetModel::updateQtVersionList);
    connect(ProjectExplorer::KitManager::instance(), &ProjectExplorer::KitManager::defaultkitChanged,
            this, &ExampleSetModel::updateQtVersionList);

    updateQtVersionList();
}


ExamplesListModelFilter::ExamplesListModelFilter(ExamplesListModel *sourceModel, bool showTutorialsOnly, QObject *parent) :
    QSortFilterProxyModel(parent),
    m_showTutorialsOnly(showTutorialsOnly)
{
    setSourceModel(sourceModel);
    setDynamicSortFilter(true);
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    sort(0);
}

bool ExamplesListModelFilter::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    const ExampleItem item = sourceModel()->index(sourceRow, 0, sourceParent).data(Qt::UserRole).value<ExampleItem>();

    if (m_showTutorialsOnly && item.type != Tutorial)
        return false;

    if (!m_showTutorialsOnly && item.type != Example && item.type != Demo)
        return false;

    if (!m_filterTags.isEmpty()) {
        return Utils::allOf(m_filterTags, [&item](const QString &filterTag) {
            return item.tags.contains(filterTag);
        });
    }

    if (!m_filterStrings.isEmpty()) {
        for (const QString &subString : m_filterStrings) {
            bool wordMatch = false;
            wordMatch |= bool(item.name.contains(subString, Qt::CaseInsensitive));
            if (wordMatch)
                continue;
            const auto subMatch = [&subString](const QString &elem) { return elem.contains(subString); };
            wordMatch |= Utils::contains(item.tags, subMatch);
            if (wordMatch)
                continue;
            wordMatch |= bool(item.description.contains(subString, Qt::CaseInsensitive));
            if (!wordMatch)
                return false;
        }
    }

    return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
}

void ExamplesListModelFilter::delayedUpdateFilter()
{
    if (m_timerId != 0)
        killTimer(m_timerId);

    m_timerId = startTimer(320);
}

void ExamplesListModelFilter::timerEvent(QTimerEvent *timerEvent)
{
    if (m_timerId == timerEvent->timerId()) {
        invalidateFilter();
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

void ExamplesListModelFilter::setSearchString(const QString &arg)
{
    if (m_searchString == arg)
        return;

    m_searchString = arg;
    m_filterTags.clear();
    m_filterStrings.clear();

    // parse and update
    SearchStringLexer lex(arg);
    bool isTag = false;
    while (int tk = lex()) {
        if (tk == SearchStringLexer::TAG) {
            isTag = true;
            m_filterStrings.append(lex.yytext);
        }

        if (tk == SearchStringLexer::STRING_LITERAL) {
            if (isTag) {
                m_filterStrings.pop_back();
                m_filterTags.append(lex.yytext);
                isTag = false;
            } else {
                m_filterStrings.append(lex.yytext);
            }
        }
    }

    delayedUpdateFilter();
}

} // namespace Internal
} // namespace QtSupport
