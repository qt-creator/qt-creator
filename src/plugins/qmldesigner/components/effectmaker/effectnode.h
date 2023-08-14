// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>
#include <QUrl>

namespace QmlDesigner {

class EffectNode : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString nodeName MEMBER m_name CONSTANT)
    Q_PROPERTY(QUrl nodeIcon MEMBER m_iconPath CONSTANT)

public:
    EffectNode(const QString &qenPath);

    QString qenPath() const;

    QString name() const;
    int nodeId() const;
    QString fragmentCode() const;
    QString vertexCode() const;
    QString qmlCode() const;
    QString description() const;

    bool operator==(const EffectNode &rhs) const noexcept
    {
        return this->m_nodeId == rhs.m_nodeId;
    }
    bool operator!=(const EffectNode &rhs) const noexcept
    {
        return !operator==(rhs);
    }

private:
    void parse(const QString &qenPath);

    QString m_qenPath;
    QString m_name;
    int m_nodeId = -1;
    QString m_fragmentCode;
    QString m_vertexCode;
    QString m_qmlCode;
    QString m_description;
    QUrl m_iconPath;
};

} // namespace QmlDesigner
