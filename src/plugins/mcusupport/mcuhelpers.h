// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "mcutarget.h"

#include <QRegularExpression>

namespace McuSupport::Internal {

struct McuTargetDescription;

McuTarget::OS deduceOperatingSystem(const McuTargetDescription &);
QString removeRtosSuffix(const QString &environmentVariable);

template<typename T>
class asKeyValueRange
{
public:
    asKeyValueRange(T &data)
        : m_data{data}
    {}
    auto begin() { return m_data.keyValueBegin(); }
    auto end() { return m_data.keyValueEnd(); }

private:
    T &m_data;
};

} // namespace McuSupport::Internal
