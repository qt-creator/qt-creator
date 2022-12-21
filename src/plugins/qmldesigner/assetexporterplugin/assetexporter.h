// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include "assetexporterview.h"
#include "utils/fileutils.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QSet>

#include <memory>

QT_BEGIN_NAMESPACE
class QJsonArray;
QT_END_NAMESPACE

namespace ProjectExplorer {
class Project;
}

namespace QmlDesigner {
class AssetDumper;
class Component;

class AssetExporter : public QObject
{
    Q_OBJECT

public:

    enum class ParsingState {
        Idle = 0,
        Parsing,
        ParsingFinished,
        ExportingAssets,
        ExportingAssetsFinished,
        WritingJson,
        ExportingDone
    };

    AssetExporter(AssetExporterView *view, ProjectExplorer::Project *project,
                  QObject *parent = nullptr);
    ~AssetExporter();

    void exportQml(const Utils::FilePaths &qmlFiles, const Utils::FilePath &exportPath,
                   bool exportAssets, bool perComponentExport);

    void cancel();
    bool isBusy() const;

    const QPixmap &generateAsset(const ModelNode &node);
    Utils::FilePath assetPath(const ModelNode &node, const Component *component,
                              const QString &suffix = {}) const;
    void exportAsset(const QPixmap &asset, const Utils::FilePath &path);
    QByteArray generateUuid(const ModelNode &node);
    QString componentUuid(const ModelNode &instance) const;

signals:
    void stateChanged(ParsingState);
    void exportProgressChanged(double) const;

private:
    ParsingState currentState() const { return m_currentState.m_state; }
    void exportComponent(const ModelNode &rootNode);
    void writeMetadata() const;
    void notifyLoadError(AssetExporterView::LoadState state);
    void notifyProgress(double value) const;
    void triggerLoadNextFile();
    void loadNextFile();

    void onQmlFileLoaded();
    Utils::FilePath componentExportDir(const Component *component) const;

    void beginExport();
    void preprocessQmlFile(const Utils::FilePath &path);
    bool assignUuids(const ModelNode &root);

private:
    mutable class State {
    public:
        State(AssetExporter&);
        void change(const ParsingState &state);
        operator ParsingState() const { return m_state; }
        AssetExporter &m_assetExporter;
        ParsingState m_state = ParsingState::Idle;
    } m_currentState;
    ProjectExplorer::Project *m_project = nullptr;
    AssetExporterView *m_view = nullptr;
    Utils::FilePaths m_exportFiles;
    unsigned int m_totalFileCount = 0;
    Utils::FilePath m_exportPath;
    QString m_exportFile;
    bool m_perComponentExport = false;
    std::vector<std::unique_ptr<Component>> m_components;
    QHash<QString, QString> m_componentUuidCache;
    QSet<QByteArray> m_usedHashes;
    QHash<QString, QPixmap> m_assets;
    std::unique_ptr<AssetDumper> m_assetDumper;
    bool m_cancelled = false;
};
QDebug operator<< (QDebug os, const QmlDesigner::AssetExporter::ParsingState& s);

} // namespace QmlDesigner
