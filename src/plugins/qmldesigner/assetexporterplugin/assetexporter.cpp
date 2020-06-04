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

#include "plaintexteditmodifier.h"
#include "rewriterview.h"

#include "projectexplorer/project.h"
#include "projectexplorer/projectnodes.h"

#include "utils/runextensions.h"
#include "utils/qtcassert.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QLoggingCategory>
#include <QMessageBox>

#include <QPlainTextEdit>

using namespace ProjectExplorer;

namespace {
Q_LOGGING_CATEGORY(loggerInfo, "qtc.designer.assetExportPlugin.assetExporter", QtInfoMsg)
Q_LOGGING_CATEGORY(loggerWarn, "qtc.designer.assetExportPlugin.assetExporter", QtWarningMsg)
Q_LOGGING_CATEGORY(loggerError, "qtc.designer.assetExportPlugin.assetExporter", QtCriticalMsg)

void findQmlFiles(QFutureInterface<Utils::FilePath> &f, const Project *project)
{
    if (!project && !f.isCanceled())
        f.reportFinished({});

    int index = 0;
    Utils::FilePaths qmlFiles = project->files([&f, &index](const Node* node) ->bool {
        if (f.isCanceled())
            return false;
        Utils::FilePath path = node->filePath();
        bool isComponent = !path.fileName().isEmpty() && path.fileName().front().isUpper();
        if (isComponent && node->filePath().endsWith(".ui.qml"))
            f.reportResult(path, index++);
        return true;
    });
    f.reportFinished();
}

//static QmlDesigner::Model* createModel(const Utils::FilePath &fileName)
//{
//    QmlDesigner::Model *model = QmlDesigner::Model::create("Item", 2, 7);

//    Utils::FileReader reader;
//    QTC_ASSERT(reader.fetch(fileName.toString()), return nullptr);

//    auto textEdit = new QPlainTextEdit;
//    textEdit->setPlainText(QString::fromUtf8(reader.data()));

//    auto modifier = new QmlDesigner::NotIndentingTextEditModifier(textEdit);
//    modifier->setParent(model);

//    auto rewriterView = new QmlDesigner::RewriterView(QmlDesigner::RewriterView::Validate, model);
//    rewriterView->setCheckSemanticErrors(false);
//    rewriterView->setTextModifier(modifier);

//    model->attachView(rewriterView);

//    QTC_ASSERT(rewriterView->rootModelNode().isValid(), return nullptr);
//    return model;
//}

}

namespace QmlDesigner {

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

bool AssetExporter::preProcessProject()
{
    if (m_preprocessWatcher && !m_preprocessWatcher->isCanceled() &&
            !m_preprocessWatcher->isFinished()) {
        qCDebug(loggerInfo) << "Previous pre-processing not finished.";
        return false;
    }

    m_currentState.change(ParsingState::PreProcessing);
    m_preprocessWatcher.reset(new QFutureWatcher<Utils::FilePath>(this));
    connect(m_preprocessWatcher.get(), &QFutureWatcher<Utils::FilePath>::resultReadyAt, this,
            [this](int index) {
        emit qmlFileResult(m_preprocessWatcher->resultAt(index));
    });

    connect(m_preprocessWatcher.get(), &QFutureWatcher<Utils::FilePath>::finished, this,
            [this] () { m_currentState.change(ParsingState::PreProcessingFinished); });

    QFuture<Utils::FilePath> f = Utils::runAsync(&findQmlFiles, m_project);
    m_preprocessWatcher->setFuture(f);
    return true;
}

void AssetExporter::exportQml(const Utils::FilePaths &qmlFiles, const Utils::FilePath &exportPath,
                              bool exportAssets)
{
    // TODO Asset export
    Q_UNUSED(exportAssets);
    m_exportFiles = qmlFiles;
    m_components = QJsonArray();
    m_exportPath = exportPath;
    m_currentState.change(ParsingState::Parsing);
    triggerLoadNextFile();
}

void AssetExporter::cancel()
{
    if (m_preprocessWatcher && !m_preprocessWatcher->isCanceled() &&
            !m_preprocessWatcher->isFinished()) {
        m_preprocessWatcher->cancel();
        m_preprocessWatcher->waitForFinished();
    }
}

bool AssetExporter::isBusy() const
{
    return m_currentState == AssetExporter::ParsingState::PreProcessing ||
            m_currentState == AssetExporter::ParsingState::Parsing ||
            m_currentState == AssetExporter::ParsingState::ExportingAssets ||
            m_currentState == AssetExporter::ParsingState::WritingJson;
}

void AssetExporter::exportComponent(const ModelNode &rootNode)
{
    qCDebug(loggerInfo) << "Exporting component" << rootNode.id();
    ComponentExporter exporter(rootNode);
    QJsonObject json = exporter.exportComponent();
    m_components.append(json);
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
    // TODO. Communicate errors to user
    qCDebug(loggerError) << "QML load error:" << errorStr;
}

void AssetExporter::onQmlFileLoaded()
{
    QTC_ASSERT(m_view && m_view->model(), qCDebug(loggerError) << "Null model"; return);
    qCDebug(loggerInfo) << "Qml file load done" << m_view->model()->fileUrl();
    exportComponent(m_view->rootModelNode());
    triggerLoadNextFile();
}

void AssetExporter::triggerLoadNextFile()
{
    QTimer::singleShot(0, this, &AssetExporter::loadNextFile);
}

void AssetExporter::loadNextFile()
{
    if (m_exportFiles.isEmpty()) {
        m_currentState.change(ParsingState::ParsingFinished);
        writeMetadata();
        return;
    }

    // Load the next pending file.
    const Utils::FilePath file = m_exportFiles.takeFirst();
    qCDebug(loggerInfo) << "Loading next file" << file;
    m_view->loadQmlFile(file);
}

void AssetExporter::writeMetadata() const
{
    qCDebug(loggerInfo) << "Writing metadata";
    m_currentState.change(ParsingState::WritingJson);
    QJsonObject jsonRoot; // TODO: Write plugin info to root
    jsonRoot.insert("artboards", m_components);
    QJsonDocument doc(jsonRoot);
    if (doc.isNull() || doc.isEmpty()) {
        qCDebug(loggerWarn) << "Empty JSON document";
    } else {
        Utils::FileSaver saver(m_exportPath.toString(), QIODevice::Text);
        saver.write(doc.toJson(QJsonDocument::Indented));
        if (!saver.finalize()) {
            qCDebug(loggerError) << "Cannot write Metadata file: " << saver.errorString();
        }
    }
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

}
