// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor_global.h"

#include <utils/store.h>

namespace TextEditor {

class TEXTEDITOR_EXPORT StorageSettings
{
public:
    StorageSettings();

    Utils::Store toMap() const;
    void fromMap(const Utils::Store &map);

    // calculated based on boolean setting plus file type blacklist examination
    bool removeTrailingWhitespace(const QString &filePattern) const;

    bool equals(const StorageSettings &ts) const;
    friend bool operator==(const StorageSettings &t1, const StorageSettings &t2)
    { return t1.equals(t2); }
    friend bool operator!=(const StorageSettings &t1, const StorageSettings &t2)
    { return !t1.equals(t2); }

    QString m_ignoreFileTypes;
    bool m_cleanWhitespace;
    bool m_inEntireDocument;
    bool m_addFinalNewLine;
    bool m_cleanIndentation;
    bool m_skipTrailingWhitespace;
};

} // namespace TextEditor
