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
#include "componentexporter.h"
#include "assetexporter.h"
#include "assetexportpluginconstants.h"
#include "exportnotification.h"
#include "parsers/modelnodeparser.h"

#include "model.h"
#include "nodeabstractproperty.h"
#include "rewriterview.h"

#include "utils/qtcassert.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QLoggingCategory>

namespace {
Q_LOGGING_CATEGORY(loggerInfo, "qtc.designer.assetExportPlugin.modelExporter", QtInfoMsg)

static void populateLineage(const QmlDesigner::ModelNode &node, QByteArrayList &lineage)
{
    if (!node.isValid() || node.type().isEmpty())
        return;
    lineage.append(node.type());
    if (node.hasParentProperty())
        populateLineage(node.parentProperty().parentModelNode(), lineage);
}

}

namespace QmlDesigner {

std::vector<std::unique_ptr<Internal::NodeParserCreatorBase>> Component::m_readers;
Component::Component(AssetExporter &exporter, const ModelNode &rootNode):
    m_exporter(exporter),
    m_rootNode(rootNode)
{

}

QJsonObject Component::json() const
{
    return m_json;
}

AssetExporter &Component::exporter()
{
    return m_exporter;
}

void Component::exportComponent()
{
    QTC_ASSERT(m_rootNode.isValid(), return);
    m_json = nodeToJson(m_rootNode);
    addImports();
}

ModelNodeParser *Component::createNodeParser(const ModelNode &node) const
{
    QByteArrayList lineage;
    populateLineage(node, lineage);
    std::unique_ptr<ModelNodeParser> reader;
    for (auto &parserCreator: m_readers) {
        std::unique_ptr<ModelNodeParser> r(parserCreator->instance(lineage, node));
        if (r->isExportable()) {
            if (reader) {
                if (reader->priority() < r->priority())
                    reader = std::move(r);
            } else {
                reader = std::move(r);
            }
        }
    }

    if (!reader)
        qCDebug(loggerInfo()) << "No parser for node" << node;

    return reader.release();
}

QJsonObject Component::nodeToJson(const ModelNode &node)
{
    QJsonObject jsonObject;
    std::unique_ptr<ModelNodeParser> parser(createNodeParser(node));
    if (parser) {
        if (parser->uuid().isEmpty()) {
            // Assign an unique identifier to the node.
            QByteArray uuid = m_exporter.generateUuid(node);
            node.setAuxiliaryData(Constants::UuidAuxTag, QString::fromLatin1(uuid));
            node.model()->rewriterView()->writeAuxiliaryData();
        }
        jsonObject = parser->json(*this);
    } else {
        ExportNotification::addError(tr("Error exporting component %1. Parser unavailable.")
                                     .arg(node.id()));
    }

    QJsonArray children;
    for (const ModelNode &childnode : node.directSubModelNodes())
        children.append(nodeToJson(childnode));

    if (!children.isEmpty())
        jsonObject.insert("children", children);

    return jsonObject;
}

void Component::addImports()
{
    QJsonArray importsArray;
    for (const Import &import : m_rootNode.model()->imports())
        importsArray.append(import.toString());

    if (!importsArray.empty())
        m_json.insert(Constants::ImportsTag, importsArray);
}


} // namespace QmlDesigner
