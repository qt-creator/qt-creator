// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmldesignercorelib_global.h>

#include <QSharedPointer>
#include <QString>

#include <vector>

namespace QmlDesigner {

class EnumerationMetaInfo
{
public:
    EnumerationMetaInfo(QSharedPointer<class NodeMetaInfoPrivate> nodeMetaInfoPrivateData,
                        const TypeName &enumeration);
    ~EnumerationMetaInfo();

private:
    QSharedPointer<class NodeMetaInfoPrivate> m_nodeMetaInfoPrivateData;
    const TypeName &m_enumeration;
};

using EnumerationMetaInfos = std::vector<EnumerationMetaInfo>;

} // namespace QmlDesigner
