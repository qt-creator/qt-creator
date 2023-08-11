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
    EffectNode(const QString &name, const QString &iconPath);

private:
    QString m_name;
    QUrl m_iconPath;
};

} // namespace QmlDesigner
