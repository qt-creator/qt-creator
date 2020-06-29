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
                   bool exportAssets = false);

    void cancel();
    bool isBusy() const;

    Utils::FilePath exportAsset(const QmlObjectNode& node, const QString &uuid);
    QByteArray generateUuid(const ModelNode &node);

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
    QJsonArray m_components;
    QSet<QByteArray> m_usedHashes;
    std::unique_ptr<AssetDumper> m_assetDumper;
    bool m_cancelled = false;
};
QDebug operator<< (QDebug os, const QmlDesigner::AssetExporter::ParsingState& s);

} // namespace QmlDesigner
