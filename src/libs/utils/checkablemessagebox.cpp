/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "checkablemessagebox.h"
#include "ui_checkablemessagebox.h"

#include <QtGui/QPushButton>
#include <QtCore/QDebug>

/*!
    \class Utils::CheckableMessageBox

     \brief A messagebox suitable for questions with a
     "Do not ask me again" checkbox.

    Emulates the QMessageBox API with
    static conveniences. The message label can open external URLs.
*/

namespace Utils {

struct CheckableMessageBoxPrivate {
    CheckableMessageBoxPrivate() : clickedButton(0) {}

    Ui::CheckableMessageBox ui;
    QAbstractButton *clickedButton;
};

CheckableMessageBox::CheckableMessageBox(QWidget *parent) :
    QDialog(parent),
    d(new CheckableMessageBoxPrivate)
{
    setModal(true);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    d->ui.setupUi(this);
    d->ui.pixmapLabel->setVisible(false);
    connect(d->ui.buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(d->ui.buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    connect(d->ui.buttonBox, SIGNAL(clicked(QAbstractButton*)), this, SLOT(slotClicked(QAbstractButton*)));
}

CheckableMessageBox::~CheckableMessageBox()
{
    delete d;
}

void CheckableMessageBox::slotClicked(QAbstractButton *b)
{
    d->clickedButton = b;
}

QAbstractButton *CheckableMessageBox::clickedButton() const
{
    return d->clickedButton;
}

QDialogButtonBox::StandardButton CheckableMessageBox::clickedStandardButton() const
{
    if (d->clickedButton)
        return d->ui.buttonBox->standardButton(d->clickedButton);
    return QDialogButtonBox::NoButton;
}

QString CheckableMessageBox::text() const
{
    return d->ui.messageLabel->text();
}

void CheckableMessageBox::setText(const QString &t)
{
    d->ui.messageLabel->setText(t);
}

QPixmap CheckableMessageBox::iconPixmap() const
{
    if (const QPixmap *p = d->ui.pixmapLabel->pixmap())
        return QPixmap(*p);
    return QPixmap();
}

void CheckableMessageBox::setIconPixmap(const QPixmap &p)
{
    d->ui.pixmapLabel->setPixmap(p);
    d->ui.pixmapLabel->setVisible(!p.isNull());
}

bool CheckableMessageBox::isChecked() const
{
    return d->ui.checkBox->isChecked();
}

void CheckableMessageBox::setChecked(bool s)
{
    d->ui.checkBox->setChecked(s);
}

QString CheckableMessageBox::checkBoxText() const
{
    return d->ui.checkBox->text();
}

void CheckableMessageBox::setCheckBoxText(const QString &t)
{
    d->ui.checkBox->setText(t);
}

bool CheckableMessageBox::isCheckBoxVisible() const
{
    return d->ui.checkBox->isVisible();
}

void CheckableMessageBox::setCheckBoxVisible(bool v)
{
    d->ui.checkBox->setVisible(v);
}

QDialogButtonBox::StandardButtons CheckableMessageBox::standardButtons() const
{
    return d->ui.buttonBox->standardButtons();
}

void CheckableMessageBox::setStandardButtons(QDialogButtonBox::StandardButtons s)
{
    d->ui.buttonBox->setStandardButtons(s);
}

QPushButton *CheckableMessageBox::button(QDialogButtonBox::StandardButton b) const
{
    return d->ui.buttonBox->button(b);
}

QPushButton *CheckableMessageBox::addButton(const QString &text, QDialogButtonBox::ButtonRole role)
{
    return d->ui.buttonBox->addButton(text, role);
}

QDialogButtonBox::StandardButton CheckableMessageBox::defaultButton() const
{
    foreach (QAbstractButton *b, d->ui.buttonBox->buttons())
        if (QPushButton *pb = qobject_cast<QPushButton *>(b))
            if (pb->isDefault())
               return d->ui.buttonBox->standardButton(pb);
    return QDialogButtonBox::NoButton;
}

void CheckableMessageBox::setDefaultButton(QDialogButtonBox::StandardButton s)
{
    if (QPushButton *b = d->ui.buttonBox->button(s)) {
        b->setDefault(true);
        b->setFocus();
    }
}

QDialogButtonBox::StandardButton
        CheckableMessageBox::question(QWidget *parent,
                                      const QString &title,
                                      const QString &question,
                                      const QString &checkBoxText,
                                      bool *checkBoxSetting,
                                      QDialogButtonBox::StandardButtons buttons,
                                      QDialogButtonBox::StandardButton defaultButton)
{
    CheckableMessageBox mb(parent);
    mb.setWindowTitle(title);
    mb.setIconPixmap(QMessageBox::standardIcon(QMessageBox::Question));
    mb.setText(question);
    mb.setCheckBoxText(checkBoxText);
    mb.setChecked(*checkBoxSetting);
    mb.setStandardButtons(buttons);
    mb.setDefaultButton(defaultButton);
    mb.exec();
    *checkBoxSetting = mb.isChecked();
    return mb.clickedStandardButton();
}

QMessageBox::StandardButton CheckableMessageBox::dialogButtonBoxToMessageBoxButton(QDialogButtonBox::StandardButton db)
{
    return static_cast<QMessageBox::StandardButton>(int(db));
}

} // namespace Utils
