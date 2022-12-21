// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <QDialogButtonBox>
#include <QMessageBox>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace Utils {

class CheckableMessageBoxPrivate;

class QTCREATOR_UTILS_EXPORT CheckableMessageBox : public QDialog
{
    Q_OBJECT
    Q_PROPERTY(QString text READ text WRITE setText)
    Q_PROPERTY(QMessageBox::Icon icon READ icon WRITE setIcon)
    Q_PROPERTY(bool isChecked READ isChecked WRITE setChecked)
    Q_PROPERTY(QString checkBoxText READ checkBoxText WRITE setCheckBoxText)
    Q_PROPERTY(QDialogButtonBox::StandardButtons buttons READ standardButtons WRITE setStandardButtons)
    Q_PROPERTY(QDialogButtonBox::StandardButton defaultButton READ defaultButton WRITE setDefaultButton)

public:
    explicit CheckableMessageBox(QWidget *parent);
    ~CheckableMessageBox() override;

    static QDialogButtonBox::StandardButton
        question(QWidget *parent,
                 const QString &title,
                 const QString &question,
                 const QString &checkBoxText,
                 bool *checkBoxSetting,
                 QDialogButtonBox::StandardButtons buttons = QDialogButtonBox::Yes|QDialogButtonBox::No,
                 QDialogButtonBox::StandardButton defaultButton = QDialogButtonBox::No);

    static QDialogButtonBox::StandardButton
        information(QWidget *parent,
                 const QString &title,
                 const QString &text,
                 const QString &checkBoxText,
                 bool *checkBoxSetting,
                 QDialogButtonBox::StandardButtons buttons = QDialogButtonBox::Ok,
                 QDialogButtonBox::StandardButton defaultButton = QDialogButtonBox::NoButton);

    static QDialogButtonBox::StandardButton
        doNotAskAgainQuestion(QWidget *parent,
                              const QString &title,
                              const QString &text,
                              QSettings *settings,
                              const QString &settingsSubKey,
                              QDialogButtonBox::StandardButtons buttons = QDialogButtonBox::Yes|QDialogButtonBox::No,
                              QDialogButtonBox::StandardButton defaultButton = QDialogButtonBox::No,
                              QDialogButtonBox::StandardButton acceptButton = QDialogButtonBox::Yes);

    static QDialogButtonBox::StandardButton
        doNotShowAgainInformation(QWidget *parent,
                                  const QString &title,
                                  const QString &text,
                                  QSettings *settings,
                                  const QString &settingsSubKey,
                                  QDialogButtonBox::StandardButtons buttons = QDialogButtonBox::Ok,
                                  QDialogButtonBox::StandardButton defaultButton = QDialogButtonBox::NoButton);

    QString text() const;
    void setText(const QString &);

    bool isChecked() const;
    void setChecked(bool s);

    QString checkBoxText() const;
    void setCheckBoxText(const QString &);

    bool isCheckBoxVisible() const;
    void setCheckBoxVisible(bool);

    QString detailedText() const;
    void setDetailedText(const QString &text);

    QDialogButtonBox::StandardButtons standardButtons() const;
    void setStandardButtons(QDialogButtonBox::StandardButtons s);
    QPushButton *button(QDialogButtonBox::StandardButton b) const;
    QPushButton *addButton(const QString &text, QDialogButtonBox::ButtonRole role);

    QDialogButtonBox::StandardButton defaultButton() const;
    void setDefaultButton(QDialogButtonBox::StandardButton s);

    QMessageBox::Icon icon() const;
    void setIcon(QMessageBox::Icon icon);

    // Query the result
    QAbstractButton *clickedButton() const;
    QDialogButtonBox::StandardButton clickedStandardButton() const;

    // check and set "ask again" status
    static bool shouldAskAgain(QSettings *settings, const QString &settingsSubKey);
    static void doNotAskAgain(QSettings *settings, const QString &settingsSubKey);

    // Conversion convenience
    static QMessageBox::StandardButton dialogButtonBoxToMessageBoxButton(QDialogButtonBox::StandardButton);
    static void resetAllDoNotAskAgainQuestions(QSettings *settings);
    static bool hasSuppressedQuestions(QSettings *settings);
    static QString msgDoNotAskAgain();
    static QString msgDoNotShowAgain();

private:
    CheckableMessageBoxPrivate *d;
};

} // namespace Utils
