// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "refactoringchanges.h"

#include <utils/changeset.h>

#include <QFutureWatcher>
#include <QString>

QT_BEGIN_NAMESPACE
class QChar;
class QTextCursor;
QT_END_NAMESPACE

namespace TextEditor {

class TabSettings;

class Formatter
{
public:
    Formatter() = default;
    virtual ~Formatter() = default;

    virtual QFutureWatcher<Utils::ChangeSet> *format(
        const QTextCursor & /*cursor*/, const TextEditor::TabSettings & /*tabSettings*/)
    {
        return nullptr;
    }

    virtual bool isElectricCharacter(const QChar & /*ch*/) const { return false; }
    virtual bool supportsAutoFormat() const { return false; }
    virtual QFutureWatcher<Utils::ChangeSet> *autoFormat(
        const QTextCursor & /*cursor*/, const TextEditor::TabSettings & /*tabSettings*/)
    {
        return nullptr;
    }

    virtual bool supportsFormatOnSave() const { return false; }
    virtual QFutureWatcher<Utils::ChangeSet> *formatOnSave(
        const QTextCursor & /*cursor*/, const TextEditor::TabSettings & /*tabSettings*/)
    {
        return nullptr;
    }
};

} // namespace TextEditor
