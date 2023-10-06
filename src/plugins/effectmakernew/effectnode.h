// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>
#include <QUrl>

namespace EffectMaker {

class EffectNode : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString nodeName MEMBER m_name CONSTANT)
    Q_PROPERTY(QString nodeDescription MEMBER m_description CONSTANT)
    Q_PROPERTY(QUrl nodeIcon MEMBER m_iconPath CONSTANT)
    Q_PROPERTY(QString nodeQenPath MEMBER m_qenPath CONSTANT)

public:
    EffectNode(const QString &qenPath);

    QString name() const;
    QString description() const;
    QString qenPath() const;

private:
    QString m_name;
    QString m_description;
    QString m_qenPath;
    QUrl m_iconPath;
};

} // namespace EffectMaker

