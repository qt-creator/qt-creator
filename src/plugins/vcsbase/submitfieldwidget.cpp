/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "submitfieldwidget.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QToolButton>
#include <QCompleter>
#include <QIcon>
#include <QToolBar>

#include <QList>
#include <QDebug>

enum { debug = 0 };
enum { spacing = 2 };

static void inline setComboBlocked(QComboBox *cb, int index)
{
    const bool blocked = cb->blockSignals(true);
    cb->setCurrentIndex(index);
    cb->blockSignals(blocked);
}

/*!
    \class VcsBase::SubmitFieldWidget
    \brief A widget for editing submit message fields like "reviewed-by:",
    "signed-off-by:".

    It displays them in a vertical row of combo/line edit fields
    that is modeled after the target address controls of mail clients.
    When choosing a different field in the combo, a new row is opened if text
    has been entered for the current field. Optionally, a "Browse..." button and
    completer can be added.
*/

namespace VcsBase {

// Field/Row entry
struct FieldEntry {
    FieldEntry();
    void createGui(const QIcon &removeIcon);
    void deleteGuiLater();

    QComboBox *combo;
    QHBoxLayout *layout;
    QLineEdit *lineEdit;
    QToolBar *toolBar;
    QToolButton *clearButton;
    QToolButton *browseButton;
    int comboIndex;
};

FieldEntry::FieldEntry() :
    combo(0),
    layout(0),
    lineEdit(0),
    toolBar(0),
    clearButton(0),
    browseButton(0),
    comboIndex(0)
{
}

void FieldEntry::createGui(const QIcon &removeIcon)
{
    layout = new QHBoxLayout;
    layout->setMargin(0);
    layout ->setSpacing(spacing);
    combo = new QComboBox;
    layout->addWidget(combo);
    lineEdit = new QLineEdit;
    layout->addWidget(lineEdit);
    toolBar = new QToolBar;
    toolBar->setProperty("_q_custom_style_disabled", QVariant(true));
    layout->addWidget(toolBar);
    clearButton = new QToolButton;
    clearButton->setIcon(removeIcon);
    toolBar->addWidget(clearButton);
    browseButton = new QToolButton;
    browseButton->setText(QLatin1String("..."));
    toolBar->addWidget(browseButton);
}

void FieldEntry::deleteGuiLater()
{
    clearButton->deleteLater();
    browseButton->deleteLater();
    toolBar->deleteLater();
    lineEdit->deleteLater();
    combo->deleteLater();
    layout->deleteLater();
}

// ------- SubmitFieldWidgetPrivate
struct SubmitFieldWidgetPrivate {
    SubmitFieldWidgetPrivate();

    int findSender(const QObject *o) const;
    int findField(const QString &f, int excluded = -1) const;
    inline QString fieldText(int) const;
    inline QString fieldValue(int) const;
    inline void focusField(int);

    const QIcon removeFieldIcon;
    QStringList fields;
    QCompleter *completer;
    bool hasBrowseButton;
    bool allowDuplicateFields;

    QList <FieldEntry> fieldEntries;
    QVBoxLayout *layout;
};

SubmitFieldWidgetPrivate::SubmitFieldWidgetPrivate() :
        removeFieldIcon(QLatin1String(":/vcsbase/images/removesubmitfield.png")),
        completer(0),
        hasBrowseButton(false),
        allowDuplicateFields(false),
        layout(0)
{
}

int SubmitFieldWidgetPrivate::findSender(const QObject *o) const
{
    const int count = fieldEntries.size();
    for (int i = 0; i < count; i++) {
        const FieldEntry &fe = fieldEntries.at(i);
        if (fe.combo == o || fe.browseButton == o || fe.clearButton == o || fe.lineEdit == o)
            return i;
    }
    return -1;
}

int SubmitFieldWidgetPrivate::findField(const QString &ft, int excluded) const
{
    const int count = fieldEntries.size();
    for (int i = 0; i < count; i++)
        if (i != excluded && fieldText(i) == ft)
            return i;
    return -1;
}

QString SubmitFieldWidgetPrivate::fieldText(int pos) const
{
    return fieldEntries.at(pos).combo->currentText();
}

QString SubmitFieldWidgetPrivate::fieldValue(int pos) const
{
    return fieldEntries.at(pos).lineEdit->text();
}

void SubmitFieldWidgetPrivate::focusField(int pos)
{
    fieldEntries.at(pos).lineEdit->setFocus(Qt::TabFocusReason);
}

// SubmitFieldWidget
SubmitFieldWidget::SubmitFieldWidget(QWidget *parent) :
        QWidget(parent),
        d(new SubmitFieldWidgetPrivate)
{
    d->layout = new QVBoxLayout;
    d->layout->setMargin(0);
    d->layout->setSpacing(spacing);
    setLayout(d->layout);
}

SubmitFieldWidget::~SubmitFieldWidget()
{
    delete d;
}

void SubmitFieldWidget::setFields(const QStringList & f)
{
    // remove old fields
    for (int i = d->fieldEntries.size() - 1 ; i >= 0 ; i--)
        removeField(i);

    d->fields = f;
    if (!f.empty())
        createField(f.front());
}

QStringList SubmitFieldWidget::fields() const
{
    return d->fields;
}

bool SubmitFieldWidget::hasBrowseButton() const
{
    return d->hasBrowseButton;
}

void SubmitFieldWidget::setHasBrowseButton(bool on)
{
    if (d->hasBrowseButton == on)
        return;
    d->hasBrowseButton = on;
    foreach (const FieldEntry &fe, d->fieldEntries)
        fe.browseButton->setVisible(on);
}

bool SubmitFieldWidget::allowDuplicateFields() const
{
    return d->allowDuplicateFields;
}

void SubmitFieldWidget::setAllowDuplicateFields(bool v)
{
    d->allowDuplicateFields = v;
}

QCompleter *SubmitFieldWidget::completer() const
{
    return d->completer;
}

void SubmitFieldWidget::setCompleter(QCompleter *c)
{
    if (c == d->completer)
        return;
    d->completer = c;
    foreach (const FieldEntry &fe, d->fieldEntries)
        fe.lineEdit->setCompleter(c);
}

QString SubmitFieldWidget::fieldValue(int pos) const
{
    return d->fieldValue(pos);
}

void SubmitFieldWidget::setFieldValue(int pos, const QString &value)
{
    d->fieldEntries.at(pos).lineEdit->setText(value);
}

QString SubmitFieldWidget::fieldValues() const
{
    const QChar blank = QLatin1Char(' ');
    const QChar newLine = QLatin1Char('\n');
    // Format as "RevBy: value\nSigned-Off: value\n"
    QString rc;
    foreach (const FieldEntry &fe, d->fieldEntries) {
        const QString value = fe.lineEdit->text().trimmed();
        if (!value.isEmpty()) {
            rc += fe.combo->currentText();
            rc += blank;
            rc += value;
            rc += newLine;
        }
    }
    return rc;
}

void SubmitFieldWidget::createField(const QString &f)
{
    FieldEntry fe;
    fe.createGui(d->removeFieldIcon);
    fe.combo->addItems(d->fields);
    if (!f.isEmpty()) {
        const int index = fe.combo->findText(f);
        if (index != -1) {
            setComboBlocked(fe.combo, index);
            fe.comboIndex = index;
        }
    }

    connect(fe.browseButton, SIGNAL(clicked()), this, SLOT(slotBrowseButtonClicked()));
    if (!d->hasBrowseButton)
        fe.browseButton->setVisible(false);

    if (d->completer)
        fe.lineEdit->setCompleter(d->completer);

    connect(fe.combo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(slotComboIndexChanged(int)));
    connect(fe.clearButton, SIGNAL(clicked()),
            this, SLOT(slotRemove()));
    d->layout->addLayout(fe.layout);
    d->fieldEntries.push_back(fe);
}

void SubmitFieldWidget::slotRemove()
{
    // Never remove first entry
    const int index = d->findSender(sender());
    switch (index) {
    case -1:
        break;
    case 0:
        d->fieldEntries.front().lineEdit->clear();
        break;
    default:
        removeField(index);
        break;
    }
}

void SubmitFieldWidget::removeField(int index)
{
    FieldEntry fe = d->fieldEntries.takeAt(index);
    QLayoutItem * item = d->layout->takeAt(index);
    fe.deleteGuiLater();
    delete item;
}

void SubmitFieldWidget::slotComboIndexChanged(int comboIndex)
{
    const int pos = d->findSender(sender());
    if (debug)
        qDebug() << '>' << Q_FUNC_INFO << pos;
    if (pos == -1)
        return;
    // Accept new index or reset combo to previous value?
    int &previousIndex = d->fieldEntries[pos].comboIndex;
    if (comboIndexChange(pos, comboIndex))
        previousIndex = comboIndex;
    else
        setComboBlocked(d->fieldEntries.at(pos).combo, previousIndex);
    if (debug)
        qDebug() << '<' << Q_FUNC_INFO << pos;
}

// Handle change of a combo. Return "false" if the combo
// is to be reset (refuse new field).
bool SubmitFieldWidget::comboIndexChange(int pos, int index)
{
    const QString newField = d->fieldEntries.at(pos).combo->itemText(index);
    // If the field is visible elsewhere: focus the existing one and refuse
    if (!d->allowDuplicateFields) {
        const int existingFieldIndex = d->findField(newField, pos);
        if (existingFieldIndex != -1) {
            d->focusField(existingFieldIndex);
            return false;
        }
    }
    // Empty value: just change the field
    if (d->fieldValue(pos).isEmpty())
        return true;
    // Non-empty: Create a new field and reset the triggering combo
    createField(newField);
    return false;
}

void SubmitFieldWidget::slotBrowseButtonClicked()
{
    const int pos = d->findSender(sender());
    emit browseButtonClicked(pos, d->fieldText(pos));
}

}
