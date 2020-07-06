/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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
#include "assetexporter.h"
#include "componentexporter.h"
#include "exportnotification.h"

#include "rewriterview.h"
#include "qmlitemnode.h"
#include "qmlobjectnode.h"
#include "utils/qtcassert.h"
#include "utils/runextensions.h"
#include "variantproperty.h"

#include <QCryptographicHash>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLoggingCategory>
#include <QWaitCondition>

#include <random>
#include <queue>

using namespace ProjectExplorer;
using namespace std;
namespace {
bool makeParentPath(const Utils::FilePath &path)
{
    QDir d;
    return d.mkpath(path.toFileInfo().absolutePath());
}

QByteArray generateHash(const QString &token) {
    static uint counter = 0;
    std::mt19937 gen(std::random_device().operator()());
    std::uniform_int_distribution<> distribution(1, 99999);
    QByteArray data = QString("%1%2%3").arg(token).arg(++counter).arg(distribution(gen)).toLatin1();
    return QCryptographicHash::hash(data, QCryptographicHash::Md5).toHex();
}

Q_LOGGING_CATEGORY(loggerInfo, "qtc.designer.assetExportPlugin.assetExporter", QtInfoMsg)
Q_LOGGING_CATEGORY(loggerWarn, "qtc.designer.assetExportPlugin.assetExporter", QtWarningMsg)
Q_LOGGING_CATEGORY(loggerError, "qtc.designer.assetExportPlugin.assetExporter", QtCriticalMsg)
}

namespace QmlDesigner {

class AssetDumper
{
public:
    AssetDumper();
    ~AssetDumper();

    void dumpAsset(const QPixmap &p, const Utils::FilePath &path);

    /* Keeps on dumping until all assets are dumped, then quits */
    void quitDumper();

    /* Aborts dumping */
    void abortDumper();

private:
    void addAsset(const QPixmap &p, const Utils::FilePath &path);
    void doDumping(QFutureInterface<void> &fi);
    void savePixmap(const QPixmap &p, Utils::FilePath &path) const;

    QFuture<void> m_dumpFuture;
    QMutex m_queueMutex;
    QWaitCondition m_queueCondition;
    std::queue<std::pair<QPixmap, Utils::FilePath>> m_assets;
    std::atomic<bool> m_quitDumper;
};



AssetExporter::AssetExporter(AssetExporterView *view, ProjectExplorer::Project *project, QObject *parent) :
    QObject(parent),
    m_currentState(*this),
    m_project(project),
    m_view(view)
{
    connect(m_view, &AssetExporterView::loadingFinished, this, &AssetExporter::onQmlFileLoaded);
    connect(m_view, &AssetExporterView::loadingError, this, &AssetExporter::notifyLoadError);
}

AssetExporter::~AssetExporter()
{
    cancel();
}

void AssetExporter::exportQml(const Utils::FilePaths &qmlFiles, const Utils::FilePath &exportPath,
                              bool exportAssets)
{
    ExportNotification::addInfo(tr("Exporting metadata at %1. Export assets: ")
                                .arg(exportPath.toUserOutput())
                                .arg(exportAssets? tr("Yes") : tr("No")));
    notifyProgress(0.0);
    m_exportFiles = qmlFiles;
    m_totalFileCount = m_exportFiles.count();
    m_components = QJsonArray();
    m_exportPath = exportPath;
    m_currentState.change(ParsingState::Parsing);
    triggerLoadNextFile();
    if (exportAssets)
        m_assetDumper = make_unique<AssetDumper>();
    else
        m_assetDumper.reset();
}

void AssetExporter::cancel()
{
    if (!m_cancelled) {
        ExportNotification::addInfo(tr("Cancelling export."));
        m_assetDumper.reset();
        m_cancelled = true;
    }
}

bool AssetExporter::isBusy() const
{
    return m_currentState == AssetExporter::ParsingState::Parsing ||
            m_currentState == AssetExporter::ParsingState::ExportingAssets ||
            m_currentState == AssetExporter::ParsingState::WritingJson;
}

Utils::FilePath AssetExporter::exportAsset(const QmlObjectNode &node, const QString &uuid)
{
    if (m_cancelled)
        return {};
    Utils::FilePath assetPath = m_exportPath.pathAppended(QString("assets/%1.png").arg(uuid));
    if (m_assetDumper)
        m_assetDumper->dumpAsset(node.toQmlItemNode().instanceRenderPixmap(), assetPath);
    return assetPath;
}

void AssetExporter::exportComponent(const ModelNode &rootNode)
{
    qCDebug(loggerInfo) << "Exporting component" << rootNode.id();
    Component exporter(*this, rootNode);
    exporter.exportComponent();
    m_components.append(exporter.json());
    notifyProgress((m_totalFileCount - m_exportFiles.count()) * 0.8 / m_totalFileCount);
}

void AssetExporter::notifyLoadError(AssetExporterView::LoadState state)
{
    QString errorStr = tr("Unknown error.");
    switch (state) {
    case AssetExporterView::LoadState::Exausted:
        errorStr = tr("Loading file is taking too long.");
        break;
    case AssetExporterView::LoadState::QmlErrorState:
        errorStr = tr("Cannot parse. QML file has errors.");
        break;
    default:
        return;
    }
    qCDebug(loggerError) << "QML load error:" << errorStr;
    ExportNotification::addError(tr("Loading QML failed. %1").arg(errorStr));
}

void AssetExporter::notifyProgress(double value) const
{
    emit exportProgressChanged(value);
}

void AssetExporter::onQmlFileLoaded()
{
    QTC_ASSERT(m_view && m_view->model(), qCDebug(loggerError) << "Null model"; return);
    qCDebug(loggerInfo) << "Qml file load done" << m_view->model()->fileUrl();
    exportComponent(m_view->rootModelNode());
    QString error;
    if (!m_view->saveQmlFile(&error)) {
        ExportNotification::addError(tr("Error saving QML file. %1")
                                     .arg(error.isEmpty()? tr("Unknown") : error));
    }
    triggerLoadNextFile();
}

QByteArray AssetExporter::generateUuid(const ModelNode &node)
{
    QByteArray uuid;
    do {
        uuid = generateHash(node.id());
    } while (m_usedHashes.contains(uuid));
    m_usedHashes.insert(uuid);
    return uuid;
}

void AssetExporter::triggerLoadNextFile()
{
    QTimer::singleShot(0, this, &AssetExporter::loadNextFile);
}

void AssetExporter::loadNextFile()
{
    if (m_cancelled || m_exportFiles.isEmpty()) {
        notifyProgress(0.8);
        m_currentState.change(ParsingState::ParsingFinished);
        writeMetadata();
        return;
    }

    // Load the next pending file.
    const Utils::FilePath file = m_exportFiles.takeFirst();
    ExportNotification::addInfo(tr("Exporting file %1.").arg(file.toUserOutput()));
    qCDebug(loggerInfo) << "Loading next file" << file;
    m_view->loadQmlFile(file);
}

void AssetExporter::writeMetadata() const
{
    if (m_cancelled) {
        notifyProgress(1.0);
        ExportNotification::addInfo(tr("Export cancelled."));
        m_currentState.change(ParsingState::ExportingDone);
        return;
    }

    Utils::FilePath metadataPath = m_exportPath.pathAppended(m_exportPath.fileName() + ".metadata");
    ExportNotification::addInfo(tr("Writing metadata to file %1.").
                                arg(metadataPath.toUserOutput()));
    makeParentPath(metadataPath);
    m_currentState.change(ParsingState::WritingJson);
    QJsonObject jsonRoot; // TODO: Write plugin info to root
    jsonRoot.insert("artboards", m_components);
    QJsonDocument doc(jsonRoot);
    if (doc.isNull() || doc.isEmpty()) {
        ExportNotification::addError(tr("Empty JSON document."));
    } else {
        Utils::FileSaver saver(metadataPath.toString(), QIODevice::Text);
        saver.write(doc.toJson(QJsonDocument::Indented));
        if (!saver.finalize()) {
            ExportNotification::addError(tr("Writing metadata failed. %1").
                                         arg(saver.errorString()));
        }
    }
    notifyProgress(1.0);
    ExportNotification::addInfo(tr("Export finished."));
    if (m_assetDumper)
        m_assetDumper->quitDumper();
    m_currentState.change(ParsingState::ExportingDone);
}

AssetExporter::State::State(AssetExporter &exporter) :
    m_assetExporter(exporter)
{

}

void AssetExporter::State::change(const ParsingState &state)
{
    qCDebug(loggerInfo()) << "Assetimporter State change: Old: " << m_state << "New: " << state;
    if (m_state != state) {
        m_state = state;
        m_assetExporter.stateChanged(m_state);
    }
}

QDebug operator<<(QDebug os, const AssetExporter::ParsingState &s)
{
    os << static_cast<std::underlying_type<QmlDesigner::AssetExporter::ParsingState>::type>(s);
    return os;
}

AssetDumper::AssetDumper():
    m_quitDumper(false)
{
    m_dumpFuture = Utils::runAsync(&AssetDumper::doDumping, this);
}

AssetDumper::~AssetDumper()
{
    abortDumper();
}

void AssetDumper::dumpAsset(const QPixmap &p, const Utils::FilePath &path)
{
    addAsset(p, path);
}

void AssetDumper::quitDumper()
{
    m_quitDumper = true;
    m_queueCondition.wakeAll();
    if (!m_dumpFuture.isFinished())
        m_dumpFuture.waitForFinished();
}

void AssetDumper::abortDumper()
{
    if (!m_dumpFuture.isFinished()) {
        m_dumpFuture.cancel();
        m_queueCondition.wakeAll();
        m_dumpFuture.waitForFinished();
    }
}

void AssetDumper::addAsset(const QPixmap &p, const Utils::FilePath &path)
{
    QMutexLocker locker(&m_queueMutex);
    qDebug() << "Save Asset:" << path;
    m_assets.push({p, path});
}

void AssetDumper::doDumping(QFutureInterface<void> &fi)
{
    auto haveAsset = [this] (std::pair<QPixmap, Utils::FilePath> *asset) {
        QMutexLocker locker(&m_queueMutex);
        if (m_assets.empty())
            return false;
        *asset = m_assets.front();
        m_assets.pop();
        return true;
    };

    forever {
        std::pair<QPixmap, Utils::FilePath> asset;
        if (haveAsset(&asset)) {
            if (fi.isCanceled())
                break;
            savePixmap(asset.first, asset.second);
        } else {
            if (m_quitDumper)
                break;
            QMutexLocker locker(&m_queueMutex);
            m_queueCondition.wait(&m_queueMutex);
        }

        if (fi.isCanceled())
            break;
    }
    fi.reportFinished();
}

void AssetDumper::savePixmap(const QPixmap &p, Utils::FilePath &path) const
{
    if (p.isNull()) {
        qCDebug(loggerWarn) << "Dumping null pixmap" << path;
        return;
    }

    if (!makeParentPath(path)) {
        ExportNotification::addError(AssetExporter::tr("Error creating asset directory. %1")
                                     .arg(path.fileName()));
        return;
    }

    if (!p.save(path.toString())) {
        ExportNotification::addError(AssetExporter::tr("Error saving asset. %1")
                                     .arg(path.fileName()));
    }
}

}
