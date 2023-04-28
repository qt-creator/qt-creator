// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "aspects.h"

#include <QMessageBox>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace Utils {

class CheckableMessageBoxPrivate;

class QTCREATOR_UTILS_EXPORT CheckableMessageBox
{
public:
    struct BoolDecision
    {
        QString text;
        bool &value;
    };

    struct SettingsDecision
    {
        QString text;
        QSettings *settings;
        QString settingsSubKey;
    };

    struct AspectDecision
    {
        QString text;
        BoolAspect &aspect;
    };

    using Decider = std::variant<BoolDecision, SettingsDecision, AspectDecision>;

    static Decider make_decider(QSettings *settings,
                                const QString &settingsSubKey,
                                const QString &text = msgDoNotAskAgain())
    {
        return Decider{SettingsDecision{text, settings, settingsSubKey}};
    }

    static Decider make_decider(bool &value, const QString &text = msgDoNotAskAgain())
    {
        return Decider{BoolDecision{text, value}};
    }

    static Decider make_decider(BoolAspect &aspect, const QString &text = msgDoNotAskAgain())
    {
        return Decider{AspectDecision{text, aspect}};
    }

    static QMessageBox::StandardButton question(
        QWidget *parent,
        const QString &title,
        const QString &question,
        std::optional<Decider> decider = std::nullopt,
        QMessageBox::StandardButtons buttons = QMessageBox::Yes | QMessageBox::No,
        QMessageBox::StandardButton defaultButton = QMessageBox::No,
        QMessageBox::StandardButton acceptButton = QMessageBox::Yes,
        QMap<QMessageBox::StandardButton, QString> buttonTextOverrides = {});

    static QMessageBox::StandardButton question(
        QWidget *parent,
        const QString &title,
        const QString &question,
        bool &value,
        QMessageBox::StandardButtons buttons = QMessageBox::Yes | QMessageBox::No,
        QMessageBox::StandardButton defaultButton = QMessageBox::No,
        QMessageBox::StandardButton acceptButton = QMessageBox::Yes,
        QMap<QMessageBox::StandardButton, QString> buttonTextOverrides = {},
        const QString &text = msgDoNotAskAgain())
    {
        Decider decider = make_decider(value, text);
        return CheckableMessageBox::question(parent,
                                             title,
                                             question,
                                             decider,
                                             buttons,
                                             defaultButton,
                                             acceptButton,
                                             buttonTextOverrides);
    }

    static QMessageBox::StandardButton question(
        QWidget *parent,
        const QString &title,
        const QString &question,
        QSettings *settings,
        const QString &settingsSubKey,
        QMessageBox::StandardButtons buttons = QMessageBox::Yes | QMessageBox::No,
        QMessageBox::StandardButton defaultButton = QMessageBox::No,
        QMessageBox::StandardButton acceptButton = QMessageBox::Yes,
        QMap<QMessageBox::StandardButton, QString> buttonTextOverrides = {},
        const QString &text = msgDoNotAskAgain())
    {
        Decider decider = make_decider(settings, settingsSubKey, text);
        return CheckableMessageBox::question(parent,
                                             title,
                                             question,
                                             decider,
                                             buttons,
                                             defaultButton,
                                             acceptButton,
                                             buttonTextOverrides);
    }

    static QMessageBox::StandardButton question(
        QWidget *parent,
        const QString &title,
        const QString &question,
        BoolAspect &aspect,
        QMessageBox::StandardButtons buttons = QMessageBox::Yes | QMessageBox::No,
        QMessageBox::StandardButton defaultButton = QMessageBox::No,
        QMessageBox::StandardButton acceptButton = QMessageBox::Yes,
        QMap<QMessageBox::StandardButton, QString> buttonTextOverrides = {},
        const QString &text = msgDoNotAskAgain())
    {
        Decider decider = make_decider(aspect, text);
        return CheckableMessageBox::question(parent,
                                             title,
                                             question,
                                             decider,
                                             buttons,
                                             defaultButton,
                                             acceptButton,
                                             buttonTextOverrides);
    }

    static QMessageBox::StandardButton information(
        QWidget *parent,
        const QString &title,
        const QString &text,
        std::optional<Decider> decider = std::nullopt,
        QMessageBox::StandardButtons buttons = QMessageBox::Ok,
        QMessageBox::StandardButton defaultButton = QMessageBox::NoButton,
        QMap<QMessageBox::StandardButton, QString> buttonTextOverrides = {});

    static QMessageBox::StandardButton information(
        QWidget *parent,
        const QString &title,
        const QString &information,
        bool &value,
        QMessageBox::StandardButtons buttons = QMessageBox::Ok,
        QMessageBox::StandardButton defaultButton = QMessageBox::NoButton,
        QMap<QMessageBox::StandardButton, QString> buttonTextOverrides = {},
        const QString &text = msgDoNotAskAgain())
    {
        Decider decider = make_decider(value, text);
        return CheckableMessageBox::information(parent,
                                                title,
                                                information,
                                                decider,
                                                buttons,
                                                defaultButton,
                                                buttonTextOverrides);
    }

    static QMessageBox::StandardButton information(
        QWidget *parent,
        const QString &title,
        const QString &information,
        QSettings *settings,
        const QString &settingsSubKey,
        QMessageBox::StandardButtons buttons = QMessageBox::Ok,
        QMessageBox::StandardButton defaultButton = QMessageBox::NoButton,
        QMap<QMessageBox::StandardButton, QString> buttonTextOverrides = {},
        const QString &text = msgDoNotAskAgain())
    {
        Decider decider = make_decider(settings, settingsSubKey, text);
        return CheckableMessageBox::information(parent,
                                                title,
                                                information,
                                                decider,
                                                buttons,
                                                defaultButton,
                                                buttonTextOverrides);
    }

    static QMessageBox::StandardButton information(
        QWidget *parent,
        const QString &title,
        const QString &information,
        BoolAspect &aspect,
        QMessageBox::StandardButtons buttons = QMessageBox::Ok,
        QMessageBox::StandardButton defaultButton = QMessageBox::NoButton,
        QMap<QMessageBox::StandardButton, QString> buttonTextOverrides = {},
        const QString &text = msgDoNotAskAgain())
    {
        Decider decider = make_decider(aspect, text);
        return CheckableMessageBox::information(parent,
                                                title,
                                                information,
                                                decider,
                                                buttons,
                                                defaultButton,
                                                buttonTextOverrides);
    }

    // check and set "ask again" status
    static bool shouldAskAgain(const Decider &decider);
    static void doNotAskAgain(Decider &decider);

    static bool shouldAskAgain(QSettings *settings, const QString &key);
    static void doNotAskAgain(QSettings *settings, const QString &key);

    // Conversion convenience
    static void resetAllDoNotAskAgainQuestions(QSettings *settings);
    static bool hasSuppressedQuestions(QSettings *settings);
    static QString msgDoNotAskAgain();
    static QString msgDoNotShowAgain();
};

} // namespace Utils
