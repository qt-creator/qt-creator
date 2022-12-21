// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

QT_BEGIN_NAMESPACE
class QString;
class QWidget;
QT_END_NAMESPACE

namespace Utils {
class FilePath;

enum ReloadPromptAnswer {
    ReloadCurrent,
    ReloadAll,
    ReloadSkipCurrent,
    ReloadNone,
    ReloadNoneAndDiff,
    CloseCurrent
};

QTCREATOR_UTILS_EXPORT ReloadPromptAnswer reloadPrompt(const FilePath &fileName,
                                                       bool modified,
                                                       bool enableDiffOption,
                                                       QWidget *parent);
QTCREATOR_UTILS_EXPORT ReloadPromptAnswer reloadPrompt(const QString &title,
                                                       const QString &prompt,
                                                       const QString &details,
                                                       bool enableDiffOption,
                                                       QWidget *parent);

enum FileDeletedPromptAnswer {
    FileDeletedClose,
    FileDeletedCloseAll,
    FileDeletedSaveAs,
    FileDeletedSave
};

QTCREATOR_UTILS_EXPORT FileDeletedPromptAnswer fileDeletedPrompt(const QString &fileName,
                                                                 QWidget *parent);

} // namespace Utils
