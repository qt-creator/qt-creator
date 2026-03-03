// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor_global.h"

#include <utils/aspects.h>
#include <utils/store.h>

namespace TextEditor {

class TEXTEDITOR_EXPORT StorageSettingsData final
{
public:
    StorageSettingsData();

    // calculated based on boolean setting plus file type blacklist examination
    bool removeTrailingWhitespace(const QString &filePattern) const;

    QString m_ignoreFileTypes;
    bool m_cleanWhitespace;
    bool m_inEntireDocument;
    bool m_addFinalNewLine;
    bool m_cleanIndentation;
    bool m_skipTrailingWhitespace;
};

class TEXTEDITOR_EXPORT StorageSettings : public Utils::AspectContainer
{
public:
    StorageSettings();

    // calculated based on boolean setting plus file type blacklist examination
    bool removeTrailingWhitespace(const QString &filePattern) const;

    StorageSettingsData data() const;
    void setData(const StorageSettingsData &data);

    void apply() final;

    Utils::StringAspect ignoreFileTypes{this};
    Utils::BoolAspect cleanWhitespace{this};
    Utils::BoolAspect inEntireDocument{this};
    Utils::BoolAspect addFinalNewLine{this};
    Utils::BoolAspect cleanIndentation{this};
    Utils::BoolAspect skipTrailingWhitespace{this};
};

void setupStorageSettings();

TEXTEDITOR_EXPORT StorageSettings &globalStorageSettings();

} // namespace TextEditor
