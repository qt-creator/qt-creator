// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/mimeutils.h>

#include <QHash>
#include <QSet>

namespace Core {

class EditorType;

namespace Internal {

QHash<Utils::MimeType, EditorType *> userPreferredEditorTypes();
void setUserPreferredEditorTypes(const QHash<Utils::MimeType, EditorType *> &factories);

/* Find the one best matching the mimetype passed in.
 * Recurse over the parent classes of the mimetype to find them. */
template<class EditorTypeLike>
static void mimeTypeFactoryLookup(const Utils::MimeType &mimeType,
                                  const QList<EditorTypeLike *> &allFactories,
                                  QList<EditorTypeLike *> *list)
{
    QSet<EditorTypeLike *> matches;
    // search breadth-first through parent hierarchy, e.g. for hierarchy
    // * application/x-ruby
    //     * application/x-executable
    //         * application/octet-stream
    //     * text/plain
    QList<Utils::MimeType> queue;
    QSet<QString> seen;
    queue.append(mimeType);
    seen.insert(mimeType.name());
    while (!queue.isEmpty()) {
        Utils::MimeType mt = queue.takeFirst();
        // check for matching factories
        for (EditorTypeLike *factory : allFactories) {
            if (!matches.contains(factory)) {
                const QStringList mimeTypes = factory->mimeTypes();
                for (const QString &mimeName : mimeTypes) {
                    if (mt.matchesName(mimeName)) {
                        list->append(factory);
                        matches.insert(factory);
                    }
                }
            }
        }
        // add parent mime types
        const QStringList parentNames = mt.parentMimeTypes();
        for (const QString &parentName : parentNames) {
            const Utils::MimeType parent = Utils::mimeTypeForName(parentName);
            if (parent.isValid()) {
                int seenSize = seen.size();
                seen.insert(parent.name());
                if (seen.size() != seenSize) // not seen before, so add
                    queue.append(parent);
            }
        }
    }
}

} // Internal
} // Core
