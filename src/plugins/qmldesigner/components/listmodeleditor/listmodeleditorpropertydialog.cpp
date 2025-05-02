// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "listmodeleditorpropertydialog.h"

#include <modelutils.h>
#include <theme.h>

#include <QDialogButtonBox>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpressionValidator>
#include <QSpacerItem>

namespace QmlDesigner {

ListModelEditorPropertyDialog::ListModelEditorPropertyDialog(const PropertyNameList &existingProperties)
    : QDialog()
    , m_existingProperties(existingProperties)
    , m_lineEdit(new QLineEdit(this))
    , m_errorLabel(new QLabel(this))
{
    static const QRegularExpression propertyNameRegex{R"(^[a-z_]{1}\w+$)"};

    QLabel *propertyLabel = new QLabel(tr("Property name:"), this);
    m_lineEdit->setValidator(new QRegularExpressionValidator(propertyNameRegex, this));
    connect(m_lineEdit,
            &QLineEdit::textChanged,
            this,
            &ListModelEditorPropertyDialog::secondaryValidation);

    QDialogButtonBox *dialogButtons = new QDialogButtonBox(this);
    m_okButton = dialogButtons->addButton(QDialogButtonBox::Ok);
    dialogButtons->addButton(QDialogButtonBox::Cancel);
    connect(dialogButtons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(dialogButtons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    m_errorLabel->setStyleSheet(
        QString("color: %1").arg(Theme::getColor(Theme::DSwarningColor).name()));

    QVBoxLayout *dialogLayout = new QVBoxLayout(this);
    dialogLayout->addWidget(propertyLabel);
    dialogLayout->addWidget(m_lineEdit);
    dialogLayout->addWidget(m_errorLabel);
    dialogLayout->addSpacerItem(
        new QSpacerItem{1, 10, QSizePolicy::Minimum, QSizePolicy::MinimumExpanding});
    dialogLayout->addWidget(dialogButtons);

    setWindowTitle(tr("Add property"));
    setError({});
}

QString ListModelEditorPropertyDialog::propertyName() const
{
    return m_lineEdit->text();
}

void ListModelEditorPropertyDialog::secondaryValidation(const QString &text)
{
    if (ModelUtils::isQmlKeyword(text))
        setError(tr("`%1` is a QML keyword and can't be used as a property name.").arg(text));
    else if (m_existingProperties.contains(text.toUtf8()))
        setError(tr("`%1` already exists in the property list.").arg(text));
    else
        setError({});
}

void ListModelEditorPropertyDialog::setError(const QString &errorString)
{
    const bool hasError = !errorString.isEmpty();
    const bool nameIsEmpty = propertyName().isEmpty();
    m_errorLabel->setText(errorString);
    m_okButton->setEnabled(!hasError && !nameIsEmpty);
}

} // namespace QmlDesigner
