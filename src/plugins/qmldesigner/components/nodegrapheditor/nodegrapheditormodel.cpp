// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "nodegrapheditormodel.h"

#include "nodegrapheditorview.h"

#include "nodegrapheditorview.h"
#include <qmldesignerplugin.h>
#include <QFileInfo>
namespace QmlDesigner {

NodeGraphEditorModel::NodeGraphEditorModel(NodeGraphEditorView *nodeGraphEditorView)
    : QStandardItemModel(nodeGraphEditorView)
    , m_editorView(nodeGraphEditorView)
{
}

void NodeGraphEditorModel::saveFile(QString fileName){
    auto directory = saveDirectory();
    auto saveFile = QFile(directory  + '/' + fileName + ".qng");
    if (!saveFile.open(QIODevice::WriteOnly)) {
        QString error = QString("Error: Couldn't save node graph file");
        qCritical() << error;
        return;
    }
    auto str = m_graphData.toStdString();
    saveFile.write(str.c_str());
    saveFile.close();
}


void NodeGraphEditorModel::openFile(QString filePath){
    const QString nodeGraphName = QFileInfo(filePath).baseName();
    setCurrentFileName(nodeGraphName);
    QFile nodeGraphFile(filePath);
    if (!nodeGraphFile.open(QIODevice::ReadOnly)) {
        QString error = QString("Couldn't open node graph file: '%1'").arg(filePath);
        qWarning() << qPrintable(error);
        return;
    }
     QByteArray data = nodeGraphFile.readAll();
        m_graphData = QString::fromStdString(data.toStdString());
        graphDataChanged();
        nodeGraphLoaded();

    nodeGraphFile.close();
    if (data.isEmpty())
        return;
}
QString NodeGraphEditorModel::saveDirectory(){
    if (m_saveDirectory!="") {
        return m_saveDirectory;
    } else {
           const QString assetDir = "nodegraphs";
          QString targetDir = QmlDesignerPlugin::instance()->documentManager().currentProjectDirPath().toString();
          QString adjustedDefaultDirectory = targetDir;
    Utils::FilePath contentPath = QmlDesignerPlugin::instance()->documentManager().currentResourcePath();
    Utils::FilePath assetPath = contentPath.pathAppended(assetDir);
    if (!assetPath.exists())
        assetPath.createDir();

    if (assetPath.exists() && assetPath.isDir())
        adjustedDefaultDirectory = assetPath.toString();
        m_saveDirectory  = adjustedDefaultDirectory;
    }
    return m_saveDirectory;
}

void NodeGraphEditorModel::openFileName(QString fileName){
    auto directory = saveDirectory();

    const QString nodeGraphName = QFileInfo(fileName).baseName();
    setCurrentFileName(nodeGraphName);
    QFile nodeGraphFile(directory + "/" + fileName + ".qng");
    if (!nodeGraphFile.open(QIODevice::ReadOnly)) {
        QString error = QString("Couldn't open node graph file: '%1'").arg(nodeGraphFile.fileName());
        qWarning() << qPrintable(error);
        return;
    }
     QByteArray data = nodeGraphFile.readAll();
        m_graphData = QString::fromStdString(data.toStdString());
        graphDataChanged();
        nodeGraphLoaded();

    nodeGraphFile.close();
    if (data.isEmpty())
        return;
}


bool NodeGraphEditorModel::hasUnsavedChanges() const
{
    return m_hasUnsavedChanges;
}

void NodeGraphEditorModel::setHasUnsavedChanges(bool val)
{
    if (m_hasUnsavedChanges == val)
        return;

    m_hasUnsavedChanges = val;
    emit hasUnsavedChangesChanged();
}


    QString NodeGraphEditorModel::currentFileName() const{
            return m_currentFileName;
        }

    void NodeGraphEditorModel::setCurrentFileName(const QString &newCurrentFileName){
    if (m_currentFileName == newCurrentFileName)
        return;

    m_currentFileName = newCurrentFileName;
    emit currentFileNameChanged();
    }

    void NodeGraphEditorModel::setGraphData(QString val){
          if (m_graphData == val)
        return;

    m_graphData = val;
    emit graphDataChanged();
    };

    QString NodeGraphEditorModel::graphData(){return m_graphData;}
} // namespace QmlDesigner
