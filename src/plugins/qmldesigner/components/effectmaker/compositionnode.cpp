// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "compositionnode.h"

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

    m_name = json.value("name").toString(); //TODO: there is a difference between name and type
    m_description = json.value("description").toString();
    m_fragmentCode = codeFromJsonArray(json.value("fragmentCode").toArray());
    m_vertexCode = codeFromJsonArray(json.value("vertexCode").toArray());

    // parse properties
    QJsonArray properties = json.value("properties").toArray();
    for (const auto /*QJsonValueRef*/ &prop : properties) {
        QJsonObject propObj = prop.toObject();
        Uniform *u = new Uniform(propObj);
        Q_UNUSED(u)

        // TODO
        propObj.value("name");
        propObj.value("type");
        propObj.value("defaultValue");
        propObj.value("description");
    }
}

QString CompositionNode::codeFromJsonArray(const QJsonArray &codeArray)
{
    if (codeArray.isEmpty())
        return {};

    QString codeString;
    for (const auto &element : codeArray)
        codeString += element.toString() + '\n';

    codeString.chop(1); // Remove last '\n'
    return codeString;
}

} // namespace QmlDesigner
