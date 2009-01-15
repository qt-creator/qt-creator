/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include "startexternaldialog.h"

#include <QtGui/QFileDialog>
#include <QtGui/QPushButton>

using namespace Debugger::Internal;

StartExternalDialog::StartExternalDialog(QWidget *parent)
  : QDialog(parent)
{
    setupUi(this);
    execFile->setExpectedKind(Core::Utils::PathChooser::File);
    execFile->setPromptDialogTitle(tr("Select Executable"));
    buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);

    //execLabel->setHidden(false);
    //execEdit->setHidden(false);
    //browseButton->setHidden(false);

    execLabel->setText(tr("Executable:"));
    argLabel->setText(tr("Arguments:"));

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

void StartExternalDialog::setExecutableFile(const QString &str)
{
    execFile->setPath(str);
}

void StartExternalDialog::setExecutableArguments(const QString &str)
{
    argsEdit->setText(str);
}

QString StartExternalDialog::executableFile() const
{
    return execFile->path();
}

QString StartExternalDialog::executableArguments() const
{
    return argsEdit->text();
    /*
    bool inQuotes = false;
    QString args = argsEdit->text();
    QChar current;
    QChar last;
    QString arg;

    QStringList result;
    if (!args.isEmpty())
        result << QLatin1String("--args");
    result << execEdit->text();

    for (int i = 0; i < args.length(); ++i) {
        current = args.at(i);

        if (current == QLatin1Char('\"') && last != QLatin1Char('\\')) {
            if (inQuotes && !arg.isEmpty()) {
                result << arg;
                arg.clear();
            }
            inQuotes = !inQuotes;
        } else if (!inQuotes && current == QLatin1Char(' ')) {
            arg = arg.trimmed();
            if (!arg.isEmpty()) {
                result << arg;
                arg.clear();
            }
        } else {
            arg += current;
        }

        last = current;
    }

    if (!arg.isEmpty())
        result << arg;

    return result;
    */
}
