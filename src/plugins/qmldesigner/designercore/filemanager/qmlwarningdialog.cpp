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

#include "qmlwarningdialog.h"
#include "ui_qmlwarningdialog.h"

#include <qmldesignerplugin.h>

#include <QPushButton>

namespace QmlDesigner {

namespace Internal {

QmlWarningDialog::QmlWarningDialog(QWidget *parent, const QStringList &warnings) :
    QDialog(parent),
    ui(new Ui::QmlWarningDialog),
    m_warnings(warnings)
{
    ui->setupUi(this);
    setResult (0);

    ui->checkBox->setChecked(true);
    connect(ui->buttonBox->button(QDialogButtonBox::Ignore),
            SIGNAL(clicked()), this, SLOT(ignoreButtonPressed()));
    connect(ui->buttonBox->button(QDialogButtonBox::Ok),
            SIGNAL(clicked()), this, SLOT(okButtonPressed()));
    connect(ui->checkBox, SIGNAL(toggled(bool)), this, SLOT(checkBoxToggled(bool)));

    connect(ui->warnings, SIGNAL(linkActivated(QString)), this, SLOT(linkClicked(QString)));

    QString warningText;
    foreach (const QString &string, warnings)
        warningText += QStringLiteral(" ") + string + QStringLiteral("\n");
    ui->warnings->setText(warningText);
}

QmlWarningDialog::~QmlWarningDialog()
{
    delete ui;
}

void QmlWarningDialog::ignoreButtonPressed()
{
    done(0);
}

void QmlWarningDialog::okButtonPressed()
{
    done(-1);
}

bool QmlWarningDialog::warningsEnabled() const
{
    DesignerSettings settings = QmlDesignerPlugin::instance()->settings();
    return settings.value(DesignerSettingsKey::WARNING_FOR_FEATURES_IN_DESIGNER).toBool();
}

void QmlWarningDialog::checkBoxToggled(bool b)
{
    DesignerSettings settings = QmlDesignerPlugin::instance()->settings();
    settings.insert(DesignerSettingsKey::WARNING_FOR_FEATURES_IN_DESIGNER, b);
    QmlDesignerPlugin::instance()->setSettings(settings);
}

void QmlWarningDialog::linkClicked(const QString &link)
{
    done(link.toInt());
}

} //Internal
} //QmlDesigner
