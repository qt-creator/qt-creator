// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmetatype.h>
#include <QString>
#include <QDataStream>

#include "nodeinstanceglobal.h"

namespace QmlDesigner {

class MockupTypeContainer
{
    friend QDataStream &operator>>(QDataStream &in, MockupTypeContainer &container);

public:
    MockupTypeContainer();
    MockupTypeContainer(const TypeName &type, const QString &importUri, int majorVersion, int minorVersion, bool isItem);

    TypeName typeName() const
    { return m_typeName; }

    QString importUri() const;

    int majorVersion() const
    { return m_majorVersion; }

    int minorVersion() const
    { return m_minorVersion; }

    bool isItem() const
    { return m_isItem; }

private:
    TypeName m_typeName;
    QString m_importUri;
    int m_majorVersion = -1;
    int m_minorVersion = -1;
    bool m_isItem = false;
};


QDataStream &operator<<(QDataStream &out, const MockupTypeContainer &container);
QDataStream &operator>>(QDataStream &in, MockupTypeContainer &container);

QDebug operator <<(QDebug debug, const MockupTypeContainer &container);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::MockupTypeContainer)
