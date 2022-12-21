// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    Utils::visitMimeParents(mimeType, [&](const Utils::MimeType &mt) -> bool {
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
        return true; // continue
    });
}

} // Internal
} // Core
