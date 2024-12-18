// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "nodegrapheditormodel.h"

#include "nodegrapheditorview.h"

#include "nodegrapheditorview.h"
#include <qmldesignerplugin.h>

#include <QFile>
#include <QFileInfo>
#include <QList>
#include <QMap>
#include <QString>

#include <QuickQanava>

namespace QmlDesigner {

enum class FileType
{
    Binary,
    Text
};

static bool writeToFile(const QByteArray &buf, const QString &filename, FileType fileType)
{
    QDir().mkpath(QFileInfo(filename).path());
    QFile f(filename);
    QIODevice::OpenMode flags = QIODevice::WriteOnly | QIODevice::Truncate;
    if (fileType == FileType::Text)
        flags |= QIODevice::Text;
    if (!f.open(flags)) {
        qWarning() << "Failed to open file for writing:" << filename;
        return false;
    }
    f.write(buf);
    return true;
}

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


void NodeGraphEditorModel::createQmlComponent(qan::Graph* graph)
{
    qan::Node* root = nullptr;
    for ( const auto& node : std::as_const(graph->get_nodes()) ) {
        const QString className = QString::fromUtf8( node->getItem()->metaObject()->className() );
        if (className.startsWith("Material")) {
            root = node;
            break;
        }
    }

    qCritical() << "createQmlComponent" << root;
    if (!root)
        return;

    int indentation = 1;
    int textureCounter = 0;
    QMap<QString, QString> translator;
    QString s;

    const QList<QString> allowedTypes{
        "nge::BaseColor",
        "nge::Metalness",
        "nge::Roughness",
        "Texture",
    };

    const QList<QString> requireQuotation{
        "QColor",
        "QUrl",
    };

    const auto makeIndent = [](int v){
        QString s;
        for (int i = 0; i < v; i++) {
            s += "    ";
        }
        return s;
    };

    const std::function<void(qan::Node* node)> scan = [&allowedTypes, &scan, &s, &translator, &requireQuotation, &textureCounter, &indentation, &makeIndent](qan::Node* node){
        for ( const auto& qi : std::as_const( node->getItem()->getPorts() ) ) {
            const auto port = qobject_cast<qan::PortItem*>(qi);
            if ( port->getInEdgeItems().size() > 0 ) {
                const auto dataName = port->property("dataName").toString();
                const auto dataType = port->property("dataType").toString();
                const auto edge = port->getInEdgeItems().at(port->getInEdgeItems().size() - 1)->getEdge();

                if (!edge) {
                    continue;
                }

                if ( dataType == "nge::BaseColor" ) {
                    translator = {
                        {"color", "baseColor"},
                        {"channel", "baseColorChannel"},
                        {"map", "baseColorMap"},
                        {"singleChannelEnabled", "baseColorSingleChannelEnabled"},
                    };
                }
                else if ( dataType == "nge::Metalness" ) {
                    translator = {
                        {"channel", "metalnessChannel"},
                        {"map", "metalnessMap"},
                        {"metalness", "metalness"},
                    };
                }
                else if ( dataType == "nge::Roughness" ) {
                    translator = {
                        {"channel", "roughnessChannel"},
                        {"map", "roughnessMap"},
                        {"roughness", "roughness"},
                    };
                }

                if ( dataName != "" ) {
                    QObject* obj = edge->getDestination()->getItem()->property("value").value<QObject*>();
                    const auto value = obj->property(dataName.toUtf8().constData());
                    QString valueString = requireQuotation.contains(dataType) ? QString{"\"%1\""}.arg(value.toString()) : value.toString();

                    if (valueString.isEmpty() && dataType == "Texture") {
                        valueString=QString{"texture_%1"}.arg(textureCounter++);
                    }

                    valueString.remove("image://qmldesigner_nodegrapheditor/");

                    const QString key = translator.contains(dataName) ? translator[dataName] : dataName;
                        s += QString{"%1%2: %3\n"}.arg(makeIndent(indentation), key, valueString);

                    if ( dataType == "Texture" ) {
                        s += QString{"%1Texture {\n"}.arg(makeIndent(indentation++));
                        s += QString{"%1id: %2\n"}.arg(makeIndent(indentation), valueString);
                    }
                }

                if (allowedTypes.contains(dataType))  {
                    scan(edge->getSource());
                }

                if ( dataType == "Texture" ) {
                    s += QString{"%1}\n"}.arg(makeIndent(--indentation));
                }
            }
        }
    };

    s += QString{
R"(import QtQuick
import QtQuick3D

PrincipledMaterial {
    id: root
)"
    };

    scan(root);

    s += QString{
R"(}
)"
    };

    Utils::FilePath path = DocumentManager::currentResourcePath().pathAppended(m_currentFileName + ".qml");
    qCritical() << path.toString();
    writeToFile(s.toUtf8(), path.toString(), FileType::Text);
}


} // namespace QmlDesigner
