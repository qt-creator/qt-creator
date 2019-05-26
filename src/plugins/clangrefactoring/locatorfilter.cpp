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

QList<Core::LocatorFilterEntry> LocatorFilter::matchesFor(
        QFutureInterface<Core::LocatorFilterEntry> &, const QString &searchTerm)
{
    using EntryList = QList<Core::LocatorFilterEntry>;
    Utils::SmallString sqlSearchTerm{searchTerm};
    sqlSearchTerm.replace('*', '%');
    sqlSearchTerm.append("%");
    const Symbols records = m_symbolQuery.symbols(m_symbolKinds, sqlSearchTerm);
    return Utils::transform<EntryList>(records, [this](const Symbol &record) {
        Core::LocatorFilterEntry entry{this,
                                       record.name.toQString(),
                                       QVariant::fromValue(record)};
        return entry;
    });
}

void LocatorFilter::accept(Core::LocatorFilterEntry locatorFilterEntry, QString *, int *, int *) const
{
    const Symbol symbol = locatorFilterEntry.internalData.value<Symbol>();

    const auto sourceLocation = m_symbolQuery.locationForSymbolId(symbol.symbolId,
                                                                  ClangBackEnd::SourceLocationKind::Definition);
    if (sourceLocation)
        m_editorManager.openEditorAt(sourceLocation->filePathId, sourceLocation->lineColumn);
}

void LocatorFilter::refresh(QFutureInterface<void> &)
{
}

} // namespace ClangRefactoring
