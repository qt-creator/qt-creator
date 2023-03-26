// Copyright (C) 2016 Nicolas Arnaud-Cormos
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/id.h>

#include <QMap>
#include <QVariant>

QT_BEGIN_NAMESPACE
class QDataStream;
QT_END_NAMESPACE

namespace Macros {
namespace Internal {

class MacroEvent
{
public:
    MacroEvent() = default;

    Utils::Id id() const;
    void setId(Utils::Id id);

    QVariant value(quint8 id) const;
    void setValue(quint8 id, const QVariant &value);

    void load(QDataStream &stream);
    void save(QDataStream &stream) const;

private:
    Utils::Id m_id;
    QMap<quint8, QVariant> m_values;
};

} // namespace Internal
} // namespace Macros
