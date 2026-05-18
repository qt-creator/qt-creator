// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor_global.h"

#include <coreplugin/dialogs/ioptionspage.h>

namespace TextEditor {

class TEXTEDITOR_EXPORT CommentsSettings : public Utils::AspectContainer
{
public:
    enum class CommandPrefix { Auto, At, Backslash };

    class TEXTEDITOR_EXPORT Data
    {
    public:
        void fromMap(const Utils::Store &data);
        void toMap(Utils::Store &data) const;

        CommandPrefix commandPrefix = CommandPrefix::Auto;
        bool enableDoxygen = true;
        bool generateBrief = true;
        bool leadingAsterisks = true;

        friend bool operator==(const Data &a, const Data &b);
        friend bool operator!=(const Data &a, const Data &b) { return !(a == b); }
    };

    Data data() const;
    void setData(const Data &data);

    static CommentsSettings &instance();

    static Utils::Key mainSettingsKey();

    CommentsSettings();

    Utils::TypedSelectionAspect<CommandPrefix> commandPrefix{this};
    Utils::BoolAspect enableDoxygen{this};
    Utils::BoolAspect generateBrief{this};
    Utils::BoolAspect leadingAsterisks{this};
};

namespace Internal {

class CommentsSettingsPage : public Core::IOptionsPage
{
public:
    CommentsSettingsPage();
};

} // namespace Internal
} // namespace TextEditor
