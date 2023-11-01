// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "checkablemessagebox.h"

#include <QAbstractButton>
#include <QDialog>

#include <memory>
#include <optional>

namespace Utils {

class PasswordDialogPrivate;

class QTCREATOR_UTILS_EXPORT ShowPasswordButton : public QAbstractButton
{
public:
    ShowPasswordButton(QWidget *parent = nullptr);

    void paintEvent(QPaintEvent *e) override;
    void enterEvent(QEnterEvent *e) override;
    void leaveEvent(QEvent *e) override;

    QSize sizeHint() const override;

private:
    bool m_containsMouse{false};
};

class QTCREATOR_UTILS_EXPORT PasswordDialog : public QDialog
{
public:
    PasswordDialog(const QString &title,
                   const QString &prompt,
                   const QString &doNotAskAgainLabel,
                   bool withUsername,
                   QWidget *parent = nullptr);
    virtual ~PasswordDialog();

    void setUser(const QString &user);
    QString user() const;

    QString password() const;

    static std::optional<QPair<QString, QString>> getUserAndPassword(
        const QString &title,
        const QString &prompt,
        const QString &doNotAskAgainLabel,
        const QString &userName,
        const CheckableDecider &decider,
        QWidget *parent = nullptr);

    static std::optional<QString> getPassword(const QString &title,
                                              const QString &prompt,
                                              const QString &doNotAskAgainLabel,
                                              const CheckableDecider &decider,
                                              QWidget *parent = nullptr);

private:
    std::unique_ptr<PasswordDialogPrivate> d;
};

} // namespace Utils
