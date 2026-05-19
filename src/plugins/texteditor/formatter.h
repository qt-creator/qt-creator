// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

#include <functional>

QT_BEGIN_NAMESPACE
class QTextCursor;
QT_END_NAMESPACE

namespace Utils { class ChangeSet; }

namespace TextEditor {

class TabSettingsData;

using FormatCallback = std::function<void(const Utils::ChangeSet &)>;

class Formatter
{
public:
    enum class FormatMode {
        Range, // default
        FullDocument,
    };
    Formatter() = default;
    virtual ~Formatter() = default;

    virtual void format(const QTextCursor & /*cursor*/,
                        const TabSettingsData & /*tabSettings*/,
                        const FormatCallback & /*callback*/) {}
    virtual void setMode(FormatMode) {}
};

} // namespace TextEditor
