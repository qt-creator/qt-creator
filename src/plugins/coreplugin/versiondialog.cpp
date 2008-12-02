/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
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
** rights. These rights are described in the Nokia Qt GPL Exception version
** 1.2, included in the file GPL_EXCEPTION.txt in this package.  
** 
***************************************************************************/
#include "versiondialog.h"
#include "coreconstants.h"
#include "coreimpl.h"

using namespace Core;
using namespace Core::Internal;
using namespace Core::Constants;

#include <QtCore/QDate>
#include <QtCore/QFile>
#include <QtGui/QGridLayout>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QTextBrowser>

VersionDialog::VersionDialog(QWidget *parent):
    QDialog(parent)
{
    // We need to set the window icon explicitly here since for some reason the
    // application icon isn't used when the size of the dialog is fixed (at least not on X11/GNOME)
    setWindowIcon(QIcon(":/qworkbench/images/qtcreator_logo_128.png"));

    setWindowTitle(tr("About Qt Creator"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    QGridLayout *layout = new QGridLayout(this);
    layout->setSizeConstraint(QLayout::SetFixedSize);

    QString version = QLatin1String(IDE_VERSION_LONG);
    version += QDate(2007, 25, 10).toString(Qt::SystemLocaleDate);

    const QString description = tr(
        "<h3>Qt Creator %1</h3>"
        "Based on Qt %2<br/>"
        "<br/>"
        "Built on " __DATE__ " at " __TIME__ "<br />"
#ifdef IDE_REVISION
        "Using revision %5<br/>"
#endif
        "<br/>"
        "<br/>"
        "Copyright 2006-%3 %4. All rights reserved.<br/>"
        "<br/>"
        "The program is provided AS IS with NO WARRANTY OF ANY KIND, "
        "INCLUDING THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A "
        "PARTICULAR PURPOSE.<br/>")
        .arg(version, QLatin1String(QT_VERSION_STR), QLatin1String(IDE_YEAR), (QLatin1String(IDE_AUTHOR))
#ifdef IDE_REVISION
             , QString(IDE_REVISION_STR).left(10)
#endif
             );

    QLabel *copyRightLabel = new QLabel(description);
    copyRightLabel->setWordWrap(true);
    copyRightLabel->setOpenExternalLinks(true);
    copyRightLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    QPushButton *closeButton = buttonBox->button(QDialogButtonBox::Close);
    Q_ASSERT(closeButton);
    buttonBox->addButton(closeButton, QDialogButtonBox::ButtonRole(QDialogButtonBox::RejectRole | QDialogButtonBox::AcceptRole));
    connect(buttonBox , SIGNAL(rejected()), this, SLOT(reject()));

    buttonBox->addButton(tr("Show License"), QDialogButtonBox::HelpRole);
    connect(buttonBox , SIGNAL(helpRequested()), this, SLOT(popupLicense()));

    QLabel *logoLabel = new QLabel;
    logoLabel->setPixmap(QPixmap(QLatin1String(":/qworkbench/images/qtcreator_logo_128.png")));
    layout->addWidget(logoLabel , 0, 0, 1, 1);
    layout->addWidget(copyRightLabel, 0, 1, 4, 4);
    layout->addWidget(buttonBox, 4, 0, 1, 5);
}

void VersionDialog::popupLicense()
{
    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle("License");
    QVBoxLayout *layout = new QVBoxLayout(dialog);
    QTextBrowser *licenseBrowser = new QTextBrowser(dialog);
    layout->addWidget(licenseBrowser);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    connect(buttonBox , SIGNAL(rejected()), dialog, SLOT(reject()));
    layout->addWidget(buttonBox);

    // Read file into string
    ICore * core = CoreImpl::instance();
    Q_ASSERT(core != NULL);
    QString fileName = core->resourcePath() + "/license.txt";
    QFile file(fileName);

    QString licenseText;
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        licenseText = "File '" + fileName + "' could not be read.";
    else
        licenseText = file.readAll();

    licenseBrowser->setPlainText(licenseText);

    dialog->setMinimumSize(QSize(550, 690));
    dialog->exec();
    delete dialog;
}
