/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "checkablemessagebox.h"
#include "ui_checkablemessagebox.h"

#include <QtGui/QPushButton>
#include <QtCore/QDebug>

namespace Utils {

struct CheckableMessageBoxPrivate {
    CheckableMessageBoxPrivate() : clickedButton(0) {}

    Ui::CheckableMessageBox ui;
    QAbstractButton *clickedButton;
};

CheckableMessageBox::CheckableMessageBox(QWidget *parent) :
    QDialog(parent),
    m_d(new CheckableMessageBoxPrivate)
{
    setModal(true);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    m_d->ui.setupUi(this);
    m_d->ui.pixmapLabel->setVisible(false);
    connect(m_d->ui.buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(m_d->ui.buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    connect(m_d->ui.buttonBox, SIGNAL(clicked(QAbstractButton*)), this, SLOT(slotClicked(QAbstractButton*)));
}

CheckableMessageBox::~CheckableMessageBox()
{
    delete m_d;
}

void CheckableMessageBox::slotClicked(QAbstractButton *b)
{
    m_d->clickedButton = b;
}

QAbstractButton *CheckableMessageBox::clickedButton() const
{
    return m_d->clickedButton;
}

QDialogButtonBox::StandardButton CheckableMessageBox::clickedStandardButton() const
{
    if (m_d->clickedButton)
        return m_d->ui.buttonBox->standardButton(m_d->clickedButton);
    return QDialogButtonBox::NoButton;
}

QString CheckableMessageBox::text() const
{
    return m_d->ui.messageLabel->text();
}

void CheckableMessageBox::setText(const QString &t)
{
    m_d->ui.messageLabel->setText(t);
}

QPixmap CheckableMessageBox::iconPixmap() const
{
    if (const QPixmap *p = m_d->ui.pixmapLabel->pixmap())
        return QPixmap(*p);
    return QPixmap();
}

void CheckableMessageBox::setIconPixmap(const QPixmap &p)
{
    m_d->ui.pixmapLabel->setPixmap(p);
    m_d->ui.pixmapLabel->setVisible(!p.isNull());
}

bool CheckableMessageBox::isChecked() const
{
    return m_d->ui.checkBox->isChecked();
}

void CheckableMessageBox::setChecked(bool s)
{
    m_d->ui.checkBox->setChecked(s);
}

QString CheckableMessageBox::checkBoxText() const
{
    return m_d->ui.checkBox->text();
}

void CheckableMessageBox::setCheckBoxText(const QString &t)
{
    m_d->ui.checkBox->setText(t);
}

QDialogButtonBox::StandardButtons CheckableMessageBox::standardButtons() const
{
    return m_d->ui.buttonBox->standardButtons();
}

void CheckableMessageBox::setStandardButtons(QDialogButtonBox::StandardButtons s)
{
    m_d->ui.buttonBox->setStandardButtons(s);
}

QDialogButtonBox::StandardButton CheckableMessageBox::defaultButton() const
{
    foreach (QAbstractButton *b, m_d->ui.buttonBox->buttons())
        if (QPushButton *pb = qobject_cast<QPushButton *>(b))
            if (pb->isDefault())
               return m_d->ui.buttonBox->standardButton(pb);
    return QDialogButtonBox::NoButton;
}

void CheckableMessageBox::setDefaultButton(QDialogButtonBox::StandardButton s)
{
    if (QPushButton *b = m_d->ui.buttonBox->button(s)) {
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
