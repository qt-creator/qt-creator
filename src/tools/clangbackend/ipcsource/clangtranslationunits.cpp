/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "clangtranslationunits.h"

#include "clangexceptions.h"
#include "clangtranslationunit.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QFileInfo>
#include <QLoggingCategory>
#include <QUuid>

#include <algorithm>

Q_LOGGING_CATEGORY(tuLog, "qtc.clangbackend.translationunits");

namespace ClangBackEnd {

TranslationUnits::TranslationUnits(const Utf8String &filePath)
    : m_filePath(filePath)
{
}

TranslationUnits::~TranslationUnits()
{
    foreach (const TranslationUnitData &unit, m_tuDatas) {
        clang_disposeTranslationUnit(unit.cxTranslationUnit);
        clang_disposeIndex(unit.cxIndex);
    }
}

TranslationUnit TranslationUnits::createAndAppend()
{
    const Utf8String id = Utf8String::fromByteArray(QUuid::createUuid().toByteArray());
    qCDebug(tuLog) << "Creating TranslationUnit" << id << "for" << QFileInfo(m_filePath).fileName();

    m_tuDatas.append(TranslationUnitData(id));
    TranslationUnitData &translationUnitData = m_tuDatas.last();

    return toTranslationUnit(translationUnitData);
}

TranslationUnit TranslationUnits::get(PreferredTranslationUnit type)
{
    if (m_tuDatas.isEmpty())
        throw TranslationUnitDoesNotExist(m_filePath);

    if (m_tuDatas.size() == 1)
        return toTranslationUnit(m_tuDatas.first());

    if (areAllTranslationUnitsParsed())
        return getPreferredTranslationUnit(type);
    else if (type == PreferredTranslationUnit::LastUninitialized)
        return toTranslationUnit(m_tuDatas.last());

    return toTranslationUnit(m_tuDatas.first());
}

void TranslationUnits::updateParseTimePoint(const Utf8String &translationUnitId,
                                            TimePoint timePoint)
{
    TranslationUnitData &unit = findUnit(translationUnitId);

    QTC_CHECK(timePoint != TimePoint());
    unit.parseTimePoint = timePoint;

    qCDebug(tuLog) << "Updated" << translationUnitId << "for" << QFileInfo(m_filePath).fileName()
        << "RecentlyParsed:" << get(PreferredTranslationUnit::RecentlyParsed).id()
        << "PreviouslyParsed:" << get(PreferredTranslationUnit::PreviouslyParsed).id();
}

TimePoint TranslationUnits::parseTimePoint(const Utf8String &translationUnitId)
{
    return findUnit(translationUnitId).parseTimePoint;
}

bool TranslationUnits::areAllTranslationUnitsParsed() const
{
    return Utils::allOf(m_tuDatas, [](const TranslationUnitData &unit) {
        return unit.parseTimePoint != TimePoint();
    });
}

int TranslationUnits::size() const
{
    return m_tuDatas.size();
}

TranslationUnit TranslationUnits::getPreferredTranslationUnit(PreferredTranslationUnit type)
{
    using TuData = TranslationUnitData;

    const auto lessThan = [](const TuData &a, const TuData &b) {
        return a.parseTimePoint < b.parseTimePoint;
    };
    auto translationUnitData = type == PreferredTranslationUnit::RecentlyParsed
            ? std::max_element(m_tuDatas.begin(), m_tuDatas.end(), lessThan)
            : std::min_element(m_tuDatas.begin(), m_tuDatas.end(), lessThan);

    if (translationUnitData == m_tuDatas.end())
        throw TranslationUnitDoesNotExist(m_filePath);

    return toTranslationUnit(*translationUnitData);
}

TranslationUnits::TranslationUnitData &TranslationUnits::findUnit(
        const Utf8String &translationUnitId)
{
    for (TranslationUnitData &unit : m_tuDatas) {
        if (translationUnitId == unit.id)
            return unit;
    }

    throw TranslationUnitDoesNotExist(m_filePath);
}

TranslationUnit TranslationUnits::toTranslationUnit(TranslationUnits::TranslationUnitData &unit)
{
    return TranslationUnit(unit.id,
                           m_filePath,
                           unit.cxIndex,
                           unit.cxTranslationUnit);
}

} // namespace ClangBackEnd
