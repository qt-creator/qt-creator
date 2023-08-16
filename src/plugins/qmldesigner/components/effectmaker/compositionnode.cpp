// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "compositionnode.h"

#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

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

    QFile loadFile(qenPath);

    if (!loadFile.open(QIODevice::ReadOnly)) {
        qWarning("Couldn't open effect file.");
        return;
    }

    QByteArray loadData = loadFile.readAll();
    QJsonParseError parseError;
    QJsonDocument jsonDoc(QJsonDocument::fromJson(loadData, &parseError));
    if (parseError.error != QJsonParseError::NoError) {
        QString error = QString("Error parsing the effect node: %1:").arg(qenPath);
        QString errorDetails = QString("%1: %2").arg(parseError.offset).arg(parseError.errorString());
        qWarning() << qPrintable(error);
        qWarning() << qPrintable(errorDetails);
        return;
    }

    QJsonObject json = jsonDoc.object();
    QFileInfo fi(loadFile);

    // TODO: QDS-10467
    // Parse the effect from QEN file
    // The process from the older implementation has the concept of `project`
    // and it contains source & dest nodes that we don't need
}

}
