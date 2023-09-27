// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "marginsettings.h"

#include <utils/qtcsettings.h>

using namespace Utils;

namespace TextEditor {

const char showWrapColumnKey[] = "ShowMargin";
const char wrapColumnKey[] = "MarginColumn";
const char groupPostfix[] = "textMarginSettings";
const char useIndenterColumnKey[] = "UseIndenter";
const char tintMarginAreaColumnKey[] = "tintMarginArea";

MarginSettings::MarginSettings()
    : m_showMargin(false)
    , m_tintMarginArea(true)
    , m_useIndenter(false)
    , m_marginColumn(80)
{
}

void MarginSettings::toSettings(QtcSettings *s) const
{
    s->beginGroup(groupPostfix);
    s->setValue(showWrapColumnKey, m_showMargin);
    s->setValue(tintMarginAreaColumnKey, m_tintMarginArea);
    s->setValue(useIndenterColumnKey, m_useIndenter);
    s->setValue(wrapColumnKey, m_marginColumn);
    s->endGroup();
}

void MarginSettings::fromSettings(QtcSettings *s)
{
    s->beginGroup(groupPostfix);
    *this = MarginSettings(); // Assign defaults

    m_showMargin = s->value(showWrapColumnKey, m_showMargin).toBool();
    m_tintMarginArea = s->value(tintMarginAreaColumnKey, m_tintMarginArea).toBool();
    m_useIndenter = s->value(useIndenterColumnKey, m_useIndenter).toBool();
    m_marginColumn = s->value(wrapColumnKey, m_marginColumn).toInt();
    s->endGroup();
}

Store MarginSettings::toMap() const
{
    return {
        {tintMarginAreaColumnKey, m_tintMarginArea},
        {showWrapColumnKey, m_showMargin},
        {useIndenterColumnKey, m_useIndenter},
        {wrapColumnKey, m_marginColumn}
    };
}

void MarginSettings::fromMap(const Store &map)
{
    m_showMargin = map.value(showWrapColumnKey, m_showMargin).toBool();
    m_tintMarginArea = map.value(tintMarginAreaColumnKey, m_tintMarginArea).toBool();
    m_useIndenter = map.value(useIndenterColumnKey, m_useIndenter).toBool();
    m_marginColumn = map.value(wrapColumnKey, m_marginColumn).toInt();
}

bool MarginSettings::equals(const MarginSettings &other) const
{
    return m_showMargin == other.m_showMargin
        && m_tintMarginArea == other.m_tintMarginArea
        && m_useIndenter == other.m_useIndenter
        && m_marginColumn == other.m_marginColumn
        ;
}

} // TextEditor
