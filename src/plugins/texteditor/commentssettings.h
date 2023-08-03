// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor_global.h"

#include <coreplugin/dialogs/ioptionspage.h>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace TextEditor {

class TEXTEDITOR_EXPORT CommentsSettings
{
public:
    class Data {
    public:
        friend bool operator==(const Data &a, const Data &b);
        friend bool operator!=(const Data &a, const Data &b) { return !(a == b); }

        bool enableDoxygen = true;
        bool generateBrief = true;
        bool leadingAsterisks = true;
    };

    static Data data() { return instance().m_data; }
    static void setData(const Data &data);

private:
    CommentsSettings();
    static CommentsSettings &instance();
    void save() const;
    void load();

    Data m_data;
};

namespace Internal {

class CommentsSettingsPage : public Core::IOptionsPage
{
public:
    CommentsSettingsPage();
};

} // namespace Internal

} // namespace TextEditor
