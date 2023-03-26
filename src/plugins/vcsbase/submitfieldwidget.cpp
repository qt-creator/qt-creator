// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "submitfieldwidget.h"

#include <utils/algorithm.h>
#include <utils/utilsicons.h>

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
    QSignalBlocker blocker(cb);
    cb->setCurrentIndex(index);
}

/*!
    \class VcsBase::SubmitFieldWidget
    \brief The SubmitFieldWidget class is a widget for editing submit message
    fields like "reviewed-by:",
    "signed-off-by:".

    The widget displays the fields in a vertical row of combo boxes or line edit fields
    that is modeled after the target address controls of mail clients.
    When choosing a different field in the combo box, a new row is opened if text
    has been entered for the current field. Optionally, a \gui Browse button and
    completer can be added.
*/

namespace VcsBase {

// Field/Row entry
struct FieldEntry {
    void createGui(const QIcon &removeIcon);
    void deleteGuiLater();

    QComboBox *combo = nullptr;
    QHBoxLayout *layout = nullptr;
    QLineEdit *lineEdit = nullptr;
    QToolBar *toolBar = nullptr;
    QToolButton *clearButton = nullptr;
    QToolButton *browseButton = nullptr;
    int comboIndex = 0;
};

void FieldEntry::createGui(const QIcon &removeIcon)
{
    layout = new QHBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
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

    int indexOf(const QWidget *w) const;
    int findField(const QString &f, int excluded = -1) const;
    inline QString fieldText(int) const;
    inline QString fieldValue(int) const;
    inline void focusField(int);

    const QIcon removeFieldIcon;
    QStringList fields;
    QCompleter *completer = nullptr;

    QList <FieldEntry> fieldEntries;
    QVBoxLayout *layout = nullptr;

    bool hasBrowseButton = false;
    bool allowDuplicateFields = false;
};

SubmitFieldWidgetPrivate::SubmitFieldWidgetPrivate() :
        removeFieldIcon(Utils::Icons::BROKEN.icon())
{
}

int SubmitFieldWidgetPrivate::indexOf(const QWidget *w) const
{
    return Utils::indexOf(fieldEntries, [w](const FieldEntry &fe) {
        return fe.combo == w || fe.browseButton == w || fe.clearButton == w || fe.lineEdit == w;
    });
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
    d->layout->setContentsMargins(0, 0, 0, 0);
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
    for (const FieldEntry &fe : std::as_const(d->fieldEntries))
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
    for (const FieldEntry &fe : std::as_const(d->fieldEntries))
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
    for (const FieldEntry &fe : std::as_const(d->fieldEntries)) {
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

    connect(fe.browseButton, &QAbstractButton::clicked, this, [this, button = fe.browseButton] {
        const int pos = d->indexOf(button);
        emit browseButtonClicked(pos, d->fieldText(pos));
    });
    if (!d->hasBrowseButton)
        fe.browseButton->setVisible(false);

    if (d->completer)
        fe.lineEdit->setCompleter(d->completer);

    connect(fe.combo, &QComboBox::currentIndexChanged, this, [this, combo = fe.combo](int index) {
        slotComboIndexChanged(d->indexOf(combo), index);
    });
    connect(fe.clearButton, &QAbstractButton::clicked, this, [this, button = fe.clearButton] {
        slotRemove(d->indexOf(button));
    });
    d->layout->addLayout(fe.layout);
    d->fieldEntries.push_back(fe);
}

void SubmitFieldWidget::slotRemove(int pos)
{
    if (pos < 0)
        return;
    if (pos == 0)
        d->fieldEntries.front().lineEdit->clear();
    else
        removeField(pos);
}

void SubmitFieldWidget::removeField(int index)
{
    FieldEntry fe = d->fieldEntries.takeAt(index);
    QLayoutItem * item = d->layout->takeAt(index);
    fe.deleteGuiLater();
    delete item;
}

void SubmitFieldWidget::slotComboIndexChanged(int pos, int comboIndex)
{
    if (debug)
        qDebug() << '>' << Q_FUNC_INFO << pos;
    if (pos < 0)
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

}
