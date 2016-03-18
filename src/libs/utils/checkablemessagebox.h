/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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
    Q_PROPERTY(QPixmap iconPixmap READ iconPixmap WRITE setIconPixmap)
    Q_PROPERTY(bool isChecked READ isChecked WRITE setChecked)
    Q_PROPERTY(QString checkBoxText READ checkBoxText WRITE setCheckBoxText)
    Q_PROPERTY(QDialogButtonBox::StandardButtons buttons READ standardButtons WRITE setStandardButtons)
    Q_PROPERTY(QDialogButtonBox::StandardButton defaultButton READ defaultButton WRITE setDefaultButton)

public:
    explicit CheckableMessageBox(QWidget *parent);
    virtual ~CheckableMessageBox();

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

    QDialogButtonBox::StandardButtons standardButtons() const;
    void setStandardButtons(QDialogButtonBox::StandardButtons s);
    QPushButton *button(QDialogButtonBox::StandardButton b) const;
    QPushButton *addButton(const QString &text, QDialogButtonBox::ButtonRole role);

    QDialogButtonBox::StandardButton defaultButton() const;
    void setDefaultButton(QDialogButtonBox::StandardButton s);

    // See static QMessageBox::standardPixmap()
    QPixmap iconPixmap() const;
    void setIconPixmap (const QPixmap &p);

    // Query the result
    QAbstractButton *clickedButton() const;
    QDialogButtonBox::StandardButton clickedStandardButton() const;

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
