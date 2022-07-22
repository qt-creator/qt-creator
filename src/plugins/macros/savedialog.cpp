/****************************************************************************
**
** Copyright (C) 2016 Nicolas Arnaud-Cormos
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

#include "savedialog.h"

#include <utils/layoutbuilder.h>

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QRegularExpressionValidator>

using namespace Utils;

namespace Macros::Internal {

SaveDialog::SaveDialog(QWidget *parent) :
    QDialog(parent)
{
    resize(219, 91);
    setWindowTitle(tr("Save Macro"));

    m_name = new QLineEdit;
    m_name->setValidator(new QRegularExpressionValidator(QRegularExpression(QLatin1String("\\w*")), this));

    m_description = new QLineEdit;

    auto buttonBox = new QDialogButtonBox;
    buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Save);

    using namespace Layouting;

    Form {
        tr("Name:"), m_name, br,
        tr("Description:"), m_description, br,
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
