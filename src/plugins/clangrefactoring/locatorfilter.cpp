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

#include "locatorfilter.h"

#include <cpptools/cpptoolsconstants.h>

#include <utils/algorithm.h>

namespace ClangRefactoring {

LocatorFilter::LocatorFilter(SymbolQueryInterface &symbolQuery)
    : m_symbolQuery(symbolQuery)
{
    setId(CppTools::Constants::LOCATOR_FILTER_ID);
    setDisplayName(CppTools::Constants::LOCATOR_FILTER_DISPLAY_NAME);
    setShortcutString(QString(QLatin1Char(':')));
    setIncludedByDefault(false);
}

QList<Core::LocatorFilterEntry> LocatorFilter::matchesFor(
        QFutureInterface<Core::LocatorFilterEntry> &, const QString &entry)
{
    SymbolString entryString(entry);
    const Classes classes = m_symbolQuery.symbolsContaining(SymbolType::Class, entryString);
    const Enums enums = m_symbolQuery.symbolsContaining(SymbolType::Enum, entryString);
    const Functions functions = m_symbolQuery.functionsContaining(entryString);

    using EntryList = QList<Core::LocatorFilterEntry>;
    auto classEntries = Utils::transform<EntryList>(classes, [this](const Class &classInfo) {
        Core::LocatorFilterEntry entry{this,
                                       classInfo.name.toQString(),
                                       qVariantFromValue(classInfo)};
        entry.extraInfo = classInfo.path.path().toQString();
        return entry;
    });
    auto enumEntries = Utils::transform<EntryList>(enums, [this](const Enum &enumInfo) {
        Core::LocatorFilterEntry entry{this,
                                       enumInfo.name.toQString(),
                                       qVariantFromValue(enumInfo)};
        entry.extraInfo = enumInfo.path.path().toQString();
        return entry;
    });
    auto functionEntries = Utils::transform<EntryList>(functions, [this](const Function &function) {
        const auto data = qVariantFromValue(static_cast<const Symbol &>(function));
        Core::LocatorFilterEntry entry{this, function.name.toQString(), data};
        entry.extraInfo = function.path.path().toQString();
        return entry;
    });

    return classEntries + enumEntries + functionEntries;
}

void LocatorFilter::accept(Core::LocatorFilterEntry, QString *, int *, int *) const
{
}

void LocatorFilter::refresh(QFutureInterface<void> &)
{
}

} // namespace ClangRefactoring
