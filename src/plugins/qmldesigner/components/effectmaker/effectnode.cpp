// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "effectnode.h"

#include <QDir>
#include <QFileInfo>

namespace QmlDesigner {

EffectNode::EffectNode(const QString &qenPath)
    : m_qenPath(qenPath)
{
    parse(qenPath);
}

QString EffectNode::qenPath() const
{
    return m_qenPath;
}

QString EffectNode::name() const
{
    return m_name;
}

int EffectNode::nodeId() const
{
    return m_nodeId;
}

QString EffectNode::fragmentCode() const
{
    return m_fragmentCode;
}

QString EffectNode::vertexCode() const
{
    return m_vertexCode;
}

QString EffectNode::qmlCode() const
{
    return m_qmlCode;
}

QString EffectNode::description() const
{
    return m_description;
}

void EffectNode::parse(const QString &qenPath)
{
    const QFileInfo fileInfo = QFileInfo(qenPath);
    m_name = fileInfo.baseName();

    QString iconPath = QStringLiteral("%1/icon/%2.svg").arg(fileInfo.absolutePath(), m_name);
    if (!QFileInfo::exists(iconPath)) {
        QDir parentDir = QDir(fileInfo.absolutePath());
        parentDir.cdUp();

        iconPath = QStringLiteral("%1/%2").arg(parentDir.path(), "placeholder.svg");
    }
    m_iconPath = QUrl::fromLocalFile(iconPath);

    // TODO: QDS-10467
    // Parse the effect from QEN file
    // The process from the older implementation has the concept of `project`
    // and it contains source & dest nodes that we don't need
}


} // namespace QmlDesigner
