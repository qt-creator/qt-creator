// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace QmlDesigner {

class CompositionNode : public QObject
{
    Q_OBJECT

public:
    CompositionNode(const QString &qenPath);

    QString fragmentCode() const;
    QString vertexCode() const;
    QString description() const;

private:
    void parse(const QString &qenPath);

    QString m_name;
    QString m_fragmentCode;
    QString m_vertexCode;
    QString m_description;
};

} // namespace QmlDesigner

