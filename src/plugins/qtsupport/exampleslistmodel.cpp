// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "exampleslistmodel.h"

#include "examplesparser.h"
#include "qtsupporttr.h"

#include <QBuffer>
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QImageReader>
#include <QPixmapCache>
#include <QUrl>

#include <android/androidconstants.h>
#include <ios/iosconstants.h>
#include <coreplugin/helpmanager.h>
#include <coreplugin/icore.h>

#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtversionmanager.h>

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/filepath.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>
#include <utils/stylehelper.h>

#include <algorithm>
#include <memory>

using namespace Core;
using namespace Utils;

namespace QtSupport {
namespace Internal {

static bool debugExamples()
{
    return qtcEnvironmentVariableIsSet("QTC_DEBUG_EXAMPLESMODEL");
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
        if (debugExamples()) {
            qWarning() << "Adding examples set displayName=" << set.displayName
                       << ", manifestPath=" << set.manifestPath
                       << ", examplesPath=" << set.examplesPath;
        }
        if (!Utils::anyOf(m_extraExampleSets, [&set](const ExtraExampleSet &s) {
                return FilePath::fromString(s.examplesPath).cleanPath()
                           == FilePath::fromString(set.examplesPath).cleanPath()
                       && FilePath::fromString(s.manifestPath).cleanPath()
                              == FilePath::fromString(set.manifestPath).cleanPath();
            })) {
            m_extraExampleSets.append(set);
        } else if (debugExamples()) {
            qWarning() << "Not adding, because example set with same directories exists";
        }
    }
    m_extraExampleSets += pluginRegisteredExampleSets();

    connect(QtVersionManager::instance(), &QtVersionManager::qtVersionsLoaded,
            this, &ExampleSetModel::qtVersionManagerLoaded);

    connect(Core::HelpManager::Signals::instance(),
            &Core::HelpManager::Signals::setupFinished,
            this,
            &ExampleSetModel::helpManagerInitialized);
}

void ExampleSetModel::recreateModel(const QtVersions &qtVersions)
{
    beginResetModel();
    clear();

    QSet<QString> extraManifestDirs;
    for (int i = 0; i < m_extraExampleSets.size(); ++i)  {
        const ExtraExampleSet &set = m_extraExampleSets.at(i);
        auto newItem = new QStandardItem();
        newItem->setData(set.displayName, Qt::DisplayRole);
        newItem->setData(set.displayName, Qt::UserRole + 1);
        newItem->setData(QVariant(), Qt::UserRole + 2);
        newItem->setData(i, Qt::UserRole + 3);
        appendRow(newItem);

        extraManifestDirs.insert(set.manifestPath);
    }

    for (QtVersion *version : qtVersions) {
        // sanitize away qt versions that have already been added through extra sets
        if (extraManifestDirs.contains(version->docsPath().toString())) {
            if (debugExamples()) {
                qWarning() << "Not showing Qt version because manifest path is already added through InstalledExamples settings:"
                           << version->displayName();
            }
            continue;
        }
        auto newItem = new QStandardItem();
        newItem->setData(version->displayName(), Qt::DisplayRole);
        newItem->setData(version->displayName(), Qt::UserRole + 1);
        newItem->setData(version->uniqueId(), Qt::UserRole + 2);
        newItem->setData(QVariant(), Qt::UserRole + 3);
        appendRow(newItem);
    }
    endResetModel();
}

int ExampleSetModel::indexForQtVersion(QtVersion *qtVersion) const
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
    const QString &documentationPath = qtVersion->docsPath().toString();
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

bool ExampleSetModel::selectedQtSupports(const Utils::Id &target) const
{
    return m_selectedQtTypes.contains(target);
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

static QString resourcePath()
{
    // normalize paths so QML doesn't freak out if it's wrongly capitalized on Windows
    return Core::ICore::resourcePath().normalizedPathName().toString();
}

static QPixmap fetchPixmapAndUpdatePixmapCache(const QString &url)
{
    QPixmap pixmap;
    if (QPixmapCache::find(url, &pixmap))
        return pixmap;

    if (url.startsWith("qthelp://")) {
        QByteArray fetchedData = Core::HelpManager::fileData(url);
        if (!fetchedData.isEmpty()) {
            QBuffer imgBuffer(&fetchedData);
            imgBuffer.open(QIODevice::ReadOnly);
            QImageReader reader(&imgBuffer, QFileInfo(url).suffix().toLatin1());
            QImage img = reader.read();
            img.convertTo(QImage::Format_RGB32);
            const int dpr = qApp->devicePixelRatio();
            // boundedTo -> don't scale thumbnails up
            const QSize scaledSize = Core::ListModel::defaultImageSize.boundedTo(img.size()) * dpr;
            pixmap = QPixmap::fromImage(
                img.scaled(scaledSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            pixmap.setDevicePixelRatio(dpr);
        }
    } else {
        pixmap.load(url);

        if (pixmap.isNull())
            pixmap.load(resourcePath() + "/welcomescreen/widgets/" + url);
    }

    QPixmapCache::insert(url, pixmap);

    return pixmap;
}

ExamplesViewController::ExamplesViewController(ExampleSetModel *exampleSetModel,
                                               SectionedGridView *view,
                                               bool isExamples,
                                               QObject *parent)
    : QObject(parent)
    , m_exampleSetModel(exampleSetModel)
    , m_view(view)
    , m_isExamples(isExamples)
{
    if (isExamples) {
        connect(m_exampleSetModel,
                &ExampleSetModel::selectedExampleSetChanged,
                this,
                &ExamplesViewController::updateExamples);
    }
    connect(Core::HelpManager::Signals::instance(),
            &Core::HelpManager::Signals::documentationChanged,
            this,
            &ExamplesViewController::updateExamples);
    view->setPixmapFunction(fetchPixmapAndUpdatePixmapCache);
    updateExamples();
}

static bool isValidExampleOrDemo(ExampleItem *item)
{
    QTC_ASSERT(item, return false);
    if (item->type == Tutorial)
        return true;
    static QString invalidPrefix = QLatin1String("qthelp:////"); /* means that the qthelp url
                                                                    doesn't have any namespace */
    QString reason;
    bool ok = true;
    if (!item->hasSourceCode || !item->projectPath.exists()) {
        ok = false;
        reason = QString::fromLatin1("projectPath \"%1\" empty or does not exist")
                     .arg(item->projectPath.toUserOutput());
    } else if (item->imageUrl.startsWith(invalidPrefix) || !QUrl(item->imageUrl).isValid()) {
        ok = false;
        reason = QString::fromLatin1("imageUrl \"%1\" not valid").arg(item->imageUrl);
    } else if (!item->docUrl.isEmpty()
               && (item->docUrl.startsWith(invalidPrefix) || !QUrl(item->docUrl).isValid())) {
        ok = false;
        reason = QString::fromLatin1("docUrl \"%1\" non-empty but not valid").arg(item->docUrl);
    }
    if (!ok) {
        item->tags.append(QLatin1String("broken"));
        if (debugExamples())
            qWarning() << QString::fromLatin1("ERROR: Item \"%1\" broken: %2").arg(item->name, reason);
    }
    if (debugExamples() && item->description.isEmpty())
        qWarning() << QString::fromLatin1("WARNING: Item \"%1\" has no description").arg(item->name);
    return ok || debugExamples();
}

void ExamplesViewController::updateExamples()
{
    QString examplesInstallPath;
    QString demosInstallPath;

    const QStringList sources = m_exampleSetModel->exampleSources(&examplesInstallPath,
                                                                  &demosInstallPath);

    m_view->clear();

    QList<ExampleItem *> items;
    for (const QString &exampleSource : sources) {
        const auto manifest = FilePath::fromUserInput(exampleSource);
        if (debugExamples()) {
            qWarning() << QString::fromLatin1("Reading file \"%1\"...")
                              .arg(manifest.absoluteFilePath().toUserOutput());
        }

        const expected_str<QList<ExampleItem *>> result
            = parseExamples(manifest,
                            FilePath::fromUserInput(examplesInstallPath),
                            FilePath::fromUserInput(demosInstallPath),
                            m_isExamples);
        if (!result) {
            if (debugExamples()) {
                qWarning() << "ERROR: Could not read examples from" << exampleSource << ":"
                           << result.error();
            }
            continue;
        }
        items += filtered(*result, isValidExampleOrDemo);
    }

    if (m_isExamples) {
        if (m_exampleSetModel->selectedQtSupports(Android::Constants::ANDROID_DEVICE_TYPE)) {
            items = Utils::filtered(items, [](ExampleItem *item) {
                return item->tags.contains("android");
            });
        } else if (m_exampleSetModel->selectedQtSupports(Ios::Constants::IOS_DEVICE_TYPE)) {
            items = Utils::filtered(items,
                                    [](ExampleItem *item) { return item->tags.contains("ios"); });
        }
    }
    Utils::sort(items, [](ExampleItem *first, ExampleItem *second) {
        return first->name.compare(second->name, Qt::CaseInsensitive) < 0;
    });

    QList<ExampleItem *> featured;
    QList<ExampleItem *> other;
    std::tie(featured, other) = Utils::partition(items,
                                                 [](ExampleItem *i) { return i->isHighlighted; });

    if (!featured.isEmpty()) {
        m_view->addSection({Tr::tr("Featured", "Category for highlighted examples"), 0},
                           static_container_cast<ListItem *>(featured));
    }
    m_view->addSection({Tr::tr("Other", "Category for all other examples"), 1},
                       static_container_cast<ListItem *>(other));
}

void ExampleSetModel::updateQtVersionList()
{
    QtVersions versions = QtVersionManager::sortVersions(QtVersionManager::versions(
        [](const QtVersion *v) { return v->hasExamples() || v->hasDemos(); }));

    // prioritize default qt version
    ProjectExplorer::Kit *defaultKit = ProjectExplorer::KitManager::defaultKit();
    QtVersion *defaultVersion = QtKitAspect::qtVersion(defaultKit);
    if (defaultVersion && versions.contains(defaultVersion))
        versions.move(versions.indexOf(defaultVersion), 0);

    recreateModel(versions);

    int currentIndex = m_selectedExampleSetIndex;
    if (currentIndex < 0) // reset from settings
        currentIndex = readCurrentIndexFromSettings();

    ExampleSetModel::ExampleSetType currentType = getType(currentIndex);

    if (currentType == ExampleSetModel::InvalidExampleSet) {
        // select examples corresponding to 'highest' Qt version
        QtVersion *highestQt = findHighestQtVersion(versions);
        currentIndex = indexForQtVersion(highestQt);
    } else if (currentType == ExampleSetModel::QtExampleSet) {
        // try to select the previously selected Qt version, or
        // select examples corresponding to 'highest' Qt version
        int currentQtId = getQtId(currentIndex);
        QtVersion *newQtVersion = QtVersionManager::version(currentQtId);
        if (!newQtVersion)
            newQtVersion = findHighestQtVersion(versions);
        currentIndex = indexForQtVersion(newQtVersion);
    } // nothing to do for extra example sets
    // Make sure to select something even if the above failed
    if (currentIndex < 0 && rowCount() > 0)
        currentIndex = 0; // simply select first
    selectExampleSet(currentIndex);
    emit selectedExampleSetChanged(currentIndex);
}

QtVersion *ExampleSetModel::findHighestQtVersion(const QtVersions &versions) const
{
    QtVersion *newVersion = nullptr;
    for (QtVersion *version : versions) {
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
        const int qtId = getQtId(m_selectedExampleSetIndex);
        const QtVersions versions = QtVersionManager::versions();
        for (QtVersion *version : versions) {
            if (version->uniqueId() == qtId) {
                manifestScanPath = version->docsPath().toString();
                examplesPath = version->examplesPath().toString();
                demosPath = version->demosPath().toString();
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
        const QFileInfoList subDirs = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (QFileInfo subDir : subDirs) {
            fis << QDir(subDir.absoluteFilePath()).entryInfoList(examplesPattern);
            fis << QDir(subDir.absoluteFilePath()).entryInfoList(demosPattern);
        }
        for (const QFileInfo &fi : std::as_const(fis))
            sources.append(fi.filePath());
    }
    if (examplesInstallPath)
        *examplesInstallPath = examplesPath;
    if (demosInstallPath)
        *demosInstallPath = demosPath;

    return sources;
}

void ExampleSetModel::selectExampleSet(int index)
{
    if (index != m_selectedExampleSetIndex) {
        m_selectedExampleSetIndex = index;
        writeCurrentIdToSettings(m_selectedExampleSetIndex);
        if (getType(m_selectedExampleSetIndex) == ExampleSetModel::QtExampleSet) {
            QtVersion *selectedQtVersion = QtVersionManager::version(getQtId(m_selectedExampleSetIndex));
            m_selectedQtTypes = selectedQtVersion->targetDeviceTypes();
        } else {
            m_selectedQtTypes.clear();
        }
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
    if (m_initalized)
        return;
    if (!m_qtVersionManagerInitialized)
        return;
    if (!m_helpManagerInitialized)
        return;

    m_initalized = true;

    connect(QtVersionManager::instance(), &QtVersionManager::qtVersionsChanged,
            this, &ExampleSetModel::updateQtVersionList);
    connect(ProjectExplorer::KitManager::instance(), &ProjectExplorer::KitManager::defaultkitChanged,
            this, &ExampleSetModel::updateQtVersionList);

    updateQtVersionList();
}

} // namespace Internal
} // namespace QtSupport
