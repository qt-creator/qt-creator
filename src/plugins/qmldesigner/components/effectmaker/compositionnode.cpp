// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "compositionnode.h"

#include "effectutils.h"
#include "effectmakeruniformsmodel.h"
#include "uniform.h"

#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace QmlDesigner {

CompositionNode::CompositionNode(const QString &qenPath)
{
    parse(qenPath);
}

QString CompositionNode::fragmentCode() const
{
    return m_fragmentCode;
}

QString CompositionNode::vertexCode() const
{
    return m_vertexCode;
}

QString CompositionNode::description() const
{
    return m_description;
}

QObject *CompositionNode::uniformsModel()
{
    return &m_unifomrsModel;
}

QStringList CompositionNode::requiredNodes() const
{
    return m_requiredNodes;
}

void CompositionNode::parse(const QString &qenPath)
{

    QFile qenFile(qenPath);

    if (!qenFile.open(QIODevice::ReadOnly)) {
        qWarning("Couldn't open effect file.");
        return;
    }

    QByteArray loadData = qenFile.readAll();
    QJsonParseError parseError;
    QJsonDocument jsonDoc(QJsonDocument::fromJson(loadData, &parseError));
    if (parseError.error != QJsonParseError::NoError) {
        QString error = QString("Error parsing the effect node: %1:").arg(qenPath);
        QString errorDetails = QString("%1: %2").arg(parseError.offset).arg(parseError.errorString());
        qWarning() << qPrintable(error);
        qWarning() << qPrintable(errorDetails);
        return;
    }

    QJsonObject json = jsonDoc.object().value("QEN").toObject();

    int version = -1;
    if (json.contains("version"))
        version = json["version"].toInt(-1);
    if (version != 1) {
        QString error = QString("Error: Unknown effect version (%1)").arg(version);
        qWarning() << qPrintable(error);
        return;
    }

    m_name = json.value("name").toString();
    m_description = json.value("description").toString();
    m_fragmentCode = EffectUtils::codeFromJsonArray(json.value("fragmentCode").toArray());
    m_vertexCode = EffectUtils::codeFromJsonArray(json.value("vertexCode").toArray());

    // parse properties
    QJsonArray jsonProps = json.value("properties").toArray();
    for (const auto /*QJsonValueRef*/ &prop : jsonProps)
        m_unifomrsModel.addUniform(new Uniform(prop.toObject()));
}

} // namespace QmlDesigner
