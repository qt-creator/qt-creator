// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "compositionnode.h"

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

    m_name = json.value("name").toString();

    // parse properties
    QJsonArray properties = json.value("properties").toArray();
    for (const auto /*QJsonValueRef*/ &prop : properties) {
        QJsonObject propObj = prop.toObject();
        propObj.value("name");
        propObj.value("type");
        propObj.value("defaultValue");
        propObj.value("description");
        // TODO
    }

    // parse shaders
    QJsonArray vertexCode = json.value("vertexCode").toArray();
    if (!vertexCode.isEmpty()) {
        // TODO
    }

    QJsonArray fragmentCode = json.value("fragmentCode").toArray();
    if (!fragmentCode.isEmpty()) {
        // TODO
    }
}

} // namespace QmlDesigner
