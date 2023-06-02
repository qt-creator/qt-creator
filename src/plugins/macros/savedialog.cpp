// Copyright (C) 2016 Nicolas Arnaud-Cormos
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "savedialog.h"

#include "macrostr.h"

#include <utils/layoutbuilder.h>

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QRegularExpressionValidator>

namespace Macros::Internal {

SaveDialog::SaveDialog(QWidget *parent) :
    QDialog(parent)
{
    resize(219, 91);
    setWindowTitle(Tr::tr("Save Macro"));

    m_name = new QLineEdit;
    m_name->setValidator(new QRegularExpressionValidator(QRegularExpression(QLatin1String("\\w*")), this));

    m_description = new QLineEdit;

    auto buttonBox = new QDialogButtonBox;
    buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Save);

    using namespace Layouting;

    Form {
        Tr::tr("Name:"), m_name, br,
        Tr::tr("Description:"), m_description, br,
        buttonBox
    }.attachTo(this);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

SaveDialog::~SaveDialog() = default;

QString SaveDialog::name() const
{
    return m_name->text();
}

QString SaveDialog::description() const
{
    return m_description->text();
}

} // Macros::Internal
