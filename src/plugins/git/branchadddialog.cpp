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

#include "branchadddialog.h"
#include "ui_branchadddialog.h"

#include <QPushButton>
#include <QValidator>

namespace Git {
namespace Internal {

/*!
 * \brief Validates the corresponding string as a valid git branch name
 *
 * The class does this by a couple of rules that are applied on the string.
 *
 */
class BranchNameValidator : public QValidator
{
public:
    BranchNameValidator(QObject *parent = 0) :
        QValidator(parent),
        m_invalidChars(QLatin1String(
                                   "\\s"          // no whitespace
                                   "|~"         // no "~"
                                   "|\\^"       // no "^"
                                   "|\\["       // no "["
                                   "|\\.\\."    // no ".."
                                   "|/\\."      // no slashdot
                                   "|:"         // no ":"
                                   "|@\\{"      // no "@{" sequence
                                   "|\\\\"      // no backslash
                                   "|//"    // no double slash
                                   "|^/"  // no leading slash
                                   ))
    {
    }

    ~BranchNameValidator() {}

    State validate(QString &input, int &pos) const
    {
        Q_UNUSED(pos)

        // NoGos

        if (input.contains(m_invalidChars))
            return Invalid;


        // "Intermediate" patterns, may change to Acceptable when user edits further:

        if (input.endsWith(QLatin1String(".lock"))) //..may not end with ".lock"
            return Intermediate;

        if (input.endsWith(QLatin1Char('.'))) // no dot at the end (but allowed in the middle)
            return Intermediate;

        if (input.endsWith(QLatin1Char('/'))) // no slash at the end (but allowed in the middle)
            return Intermediate;

        // is a valid branch name
        return Acceptable;
    }

private:
    const QRegExp m_invalidChars;
};


BranchAddDialog::BranchAddDialog(QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::BranchAddDialog)
{
    m_ui->setupUi(this);
    m_ui->branchNameEdit->setValidator(new BranchNameValidator(this));
    connect(m_ui->branchNameEdit, SIGNAL(textChanged(QString)), this, SLOT(updateButtonStatus()));
}

BranchAddDialog::~BranchAddDialog()
{
    delete m_ui;
}

void BranchAddDialog::setBranchName(const QString &n)
{
    m_ui->branchNameEdit->setText(n);
    m_ui->branchNameEdit->selectAll();
}

QString BranchAddDialog::branchName() const
{
    return m_ui->branchNameEdit->text();
}

void BranchAddDialog::setTrackedBranchName(const QString &name, bool remote)
{
    m_ui->trackingCheckBox->setVisible(true);
    if (!name.isEmpty())
        m_ui->trackingCheckBox->setText(remote ? tr("Track remote branch \'%1\'").arg(name) :
                                                 tr("Track local branch \'%1\'").arg(name));
    else
        m_ui->trackingCheckBox->setVisible(false);
    m_ui->trackingCheckBox->setChecked(remote);
}

bool BranchAddDialog::track()
{
    return m_ui->trackingCheckBox->isChecked();
}

/*! Updates the ok button enabled state of the dialog according to the validity of the branch name. */
void BranchAddDialog::updateButtonStatus()
{
    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(m_ui->branchNameEdit->hasAcceptableInput());
}

} // namespace Internal
} // namespace Git
