/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "versiondialog.h"

#include "coreconstants.h"
#include "icore.h"

#include <utils/qtcassert.h>

#include <QtCore/QDate>
#include <QtCore/QFile>
#include <QtCore/QSysInfo>

#include <QtGui/QDialogButtonBox>
#include <QtGui/QGridLayout>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QTextBrowser>

using namespace Core;
using namespace Core::Internal;
using namespace Core::Constants;

VersionDialog::VersionDialog(QWidget *parent)
    : QDialog(parent)
{
    // We need to set the window icon explicitly here since for some reason the
    // application icon isn't used when the size of the dialog is fixed (at least not on X11/GNOME)
    setWindowIcon(QIcon(QLatin1String(Constants::ICON_QTLOGO_128)));

    setWindowTitle(tr("About Qt Creator"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    QGridLayout *layout = new QGridLayout(this);
    layout->setSizeConstraint(QLayout::SetFixedSize);

    QString version = QLatin1String(IDE_VERSION_LONG);

    QString ideVersionDescription;
#ifdef IDE_VERSION_DESCRIPTION
    ideVersionDescription = tr("(%1)").arg(QLatin1String(IDE_VERSION_DESCRIPTION_STR));
#endif

    QString ideRev;
#ifdef IDE_REVISION
     //: This gets conditionally inserted as argument %8 into the description string.
     ideRev = tr("From revision %1<br/>").arg(QString::fromLatin1(IDE_REVISION_STR).left(10));
#endif

     const QString description = tr(
        "<h3>Qt Creator %1 %8</h3>"
        "Based on Qt %2 (%3 bit)<br/>"
        "<br/>"
        "Built on %4 at %5<br />"
        "<br/>"
        "%9"
        "<br/>"
        "Copyright 2008-%6 %7. All rights reserved.<br/>"
        "<br/>"
        "The program is provided AS IS with NO WARRANTY OF ANY KIND, "
        "INCLUDING THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A "
        "PARTICULAR PURPOSE.<br/>")
        .arg(version, 
             QLatin1String(QT_VERSION_STR), QString::number(QSysInfo::WordSize),
             QLatin1String(__DATE__), QLatin1String(__TIME__), QLatin1String(IDE_YEAR),
             (QLatin1String(IDE_AUTHOR)), ideVersionDescription,
             ideRev);

    QLabel *copyRightLabel = new QLabel(description);
    copyRightLabel->setWordWrap(true);
    copyRightLabel->setOpenExternalLinks(true);
    copyRightLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    QPushButton *closeButton = buttonBox->button(QDialogButtonBox::Close);
    QTC_CHECK(closeButton);
    buttonBox->addButton(closeButton, QDialogButtonBox::ButtonRole(QDialogButtonBox::RejectRole | QDialogButtonBox::AcceptRole));
    connect(buttonBox , SIGNAL(rejected()), this, SLOT(reject()));

    QLabel *logoLabel = new QLabel;
    logoLabel->setPixmap(QPixmap(QLatin1String(Constants::ICON_QTLOGO_128)));
    layout->addWidget(logoLabel , 0, 0, 1, 1);
    layout->addWidget(copyRightLabel, 0, 1, 4, 4);
    layout->addWidget(buttonBox, 4, 0, 1, 5);
}
