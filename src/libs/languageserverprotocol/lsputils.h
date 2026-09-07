// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "languageserverprotocol_global.h"
#include "lsptypes.h"

#include <utils/filepath.h>
#include <utils/mimeutils.h>

QT_BEGIN_NAMESPACE
class QTextCursor;
class QTextDocument;
QT_END_NAMESPACE

namespace LanguageServerProtocol {

inline bool operator<(const Position &first, const Position &second)
{
    return first.line() < second.line()
           || (first.line() == second.line() && first.character() < second.character());
}
inline bool operator>(const Position &first, const Position &second) { return second < first; }
inline bool operator<=(const Position &first, const Position &second) { return !(second < first); }
inline bool operator>=(const Position &first, const Position &second) { return !(first < second); }

/// The document uri of  path, as the protocol spells it.
LANGUAGESERVERPROTOCOL_EXPORT QString uriFromPath(const Utils::FilePath &path);
/// The path  uri denotes, empty for a uri that does not name a local file.
LANGUAGESERVERPROTOCOL_EXPORT Utils::FilePath pathFromUri(const QString &uri);

LANGUAGESERVERPROTOCOL_EXPORT Position positionOf(const QTextCursor &cursor);
LANGUAGESERVERPROTOCOL_EXPORT int positionInDocument(const Position &position,
                                                      const QTextDocument *document);
LANGUAGESERVERPROTOCOL_EXPORT QTextCursor toTextCursor(const Position &position,
                                                        QTextDocument *document);
LANGUAGESERVERPROTOCOL_EXPORT Position withOffset(const Position &position, int offset,
                                                   const QTextDocument *document);

/// The text of a value that is either a plain string or markup.
LANGUAGESERVERPROTOCOL_EXPORT QString plainText(const std::variant<QString, MarkupContent> &value);

/// The language id the protocol uses for \a mimeType, empty if it defines none.
LANGUAGESERVERPROTOCOL_EXPORT QString mimeTypeToLanguageId(const Utils::MimeType &mimeType);
LANGUAGESERVERPROTOCOL_EXPORT QString mimeTypeToLanguageId(const QString &mimeTypeName);

/// Whether \a filter selects the document \a filePath of \a mimeType.
LANGUAGESERVERPROTOCOL_EXPORT bool applies(const DocumentFilter &filter,
                                            const Utils::FilePath &filePath,
                                            const Utils::MimeType &mimeType = {});
/// Whether any filter of \a selector applies, and true for a selector that is not set.
LANGUAGESERVERPROTOCOL_EXPORT bool applies(const std::optional<DocumentSelector> &selector,
                                            const Utils::FilePath &filePath,
                                            const Utils::MimeType &mimeType = {});

LANGUAGESERVERPROTOCOL_EXPORT Range rangeOf(const QTextCursor &cursor);
LANGUAGESERVERPROTOCOL_EXPORT QTextCursor toSelection(const Range &range,
                                                       QTextDocument *document);

inline bool isEmpty(const Range &range) { return range.start() == range.end(); }
inline bool contains(const Range &range, const Position &position)
{
    return range.start() <= position && position <= range.end();
}
LANGUAGESERVERPROTOCOL_EXPORT bool contains(const Range &range, const Range &other);
LANGUAGESERVERPROTOCOL_EXPORT bool isLeftOf(const Range &range, const Range &other);
LANGUAGESERVERPROTOCOL_EXPORT bool overlaps(const Range &range, const Range &other);

} // namespace LanguageServerProtocol
