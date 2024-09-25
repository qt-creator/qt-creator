// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QMetaType>
#include <QSharedPointer>
#include <QString>

#include "corelib_global.h"
#include "modelnode.h"

namespace QmlDesigner {

class CORESHARED_EXPORT PropertyBinding
{
public:
    PropertyBinding();
    PropertyBinding(const QString &value);
    PropertyBinding(const PropertyBinding &other);

    PropertyBinding &operator=(const PropertyBinding &other);

    bool isValid() const;
    QString value() const;

private:
    QString m_value;
};

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::PropertyBinding);
