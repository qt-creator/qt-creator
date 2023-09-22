// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <QMap>
#include <QMessageBox>

namespace Utils {

class Key;
class QtcSettings;

class QTCREATOR_UTILS_EXPORT CheckableDecider
{
public:
    CheckableDecider() = default;
    CheckableDecider(const Key &settingsSubKey);
    CheckableDecider(bool *doNotAskAgain);
    CheckableDecider(const std::function<bool()> &should, const std::function<void()> &doNot)
        : shouldAskAgain(should), doNotAskAgain(doNot)
    {}

    std::function<bool()> shouldAskAgain;
    std::function<void()> doNotAskAgain;
};

class QTCREATOR_UTILS_EXPORT CheckableMessageBox
{
public:
    static QMessageBox::StandardButton question(
        QWidget *parent,
        const QString &title,
        const QString &question,
        const CheckableDecider &decider,
        QMessageBox::StandardButtons buttons = QMessageBox::Yes | QMessageBox::No,
        QMessageBox::StandardButton defaultButton = QMessageBox::No,
        QMessageBox::StandardButton acceptButton = QMessageBox::Yes,
        QMap<QMessageBox::StandardButton, QString> buttonTextOverrides = {},
        const QString &msg = {});

    static QMessageBox::StandardButton information(
        QWidget *parent,
        const QString &title,
        const QString &text,
        const CheckableDecider &decider,
        QMessageBox::StandardButtons buttons = QMessageBox::Ok,
        QMessageBox::StandardButton defaultButton = QMessageBox::NoButton,
        QMap<QMessageBox::StandardButton, QString> buttonTextOverrides = {},
        const QString &msg = {});

    // Conversion convenience
    static void resetAllDoNotAskAgainQuestions();
    static bool hasSuppressedQuestions();
    static QString msgDoNotAskAgain();
    static QString msgDoNotShowAgain();

    static void initialize(QtcSettings *settings);
};

} // namespace Utils
