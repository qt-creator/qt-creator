/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "includesfilter.h"

#include <cpptools/cpptoolsconstants.h>

#include <utils/algorithm.h>

namespace ClangRefactoring {

IncludesFilter::IncludesFilter(SymbolQueryInterface &symbolQuery)
    : m_symbolQuery(symbolQuery)
{
    setId(CppTools::Constants::INCLUDES_FILTER_ID);
    setDisplayName(CppTools::Constants::INCLUDES_FILTER_DISPLAY_NAME);
    setShortcutString(QString(QLatin1Char('a')));
    setIncludedByDefault(true);
    setPriority(ILocatorFilter::Low);
}

QList<Core::LocatorFilterEntry> IncludesFilter::matchesFor(
        QFutureInterface<Core::LocatorFilterEntry> &, const QString &entry)
{
    using EntryList = QList<Core::LocatorFilterEntry>;
    const SymbolString entryString(entry);
    const Includes includes = m_symbolQuery.symbolsContaining(SymbolType::Include, entryString);
    return Utils::transform<EntryList>(includes, [this](const Include &includeInfo) {
        Core::LocatorFilterEntry entry{this,
                                       includeInfo.name.toQString(),
                                       qVariantFromValue(includeInfo)};
        entry.extraInfo = includeInfo.path.path().toQString();
        return entry;
    });
}

void IncludesFilter::accept(Core::LocatorFilterEntry, QString *, int *, int *) const
{
}

void IncludesFilter::refresh(QFutureInterface<void> &)
{
}

} // namespace ClangRefactoring
