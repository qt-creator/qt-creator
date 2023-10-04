// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "versiondialog.h"

#include "coreicons.h"
#include "coreplugintr.h"
#include "coreicons.h"
#include "icore.h"

#include <utils/algorithm.h>
#include <utils/appinfo.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QDialogButtonBox>
#include <QGridLayout>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QLabel>
#include <QPushButton>

using namespace Core;
using namespace Core::Internal;

VersionDialog::VersionDialog(QWidget *parent)
    : QDialog(parent)
{
    // We need to set the window icon explicitly here since for some reason the
    // application icon isn't used when the size of the dialog is fixed (at least not on X11/GNOME)
    if (Utils::HostOsInfo::isLinuxHost())
        setWindowIcon(Icons::QTCREATORLOGO_BIG.icon());

    setWindowTitle(Tr::tr("About %1").arg(QGuiApplication::applicationDisplayName()));
    auto layout = new QGridLayout(this);
    layout->setSizeConstraint(QLayout::SetFixedSize);

    const Utils::AppInfo appInfo = Utils::appInfo();
    QString ideRev;
    if (!appInfo.revision.isEmpty())
        ideRev = Tr::tr("<br/>From revision %1<br/>")
                .arg(appInfo.revisionUrl.isEmpty()
                     ? appInfo.revision
                     : QString::fromLatin1("<a href=\"%1\">%2</a>")
                       .arg(appInfo.revisionUrl, appInfo.revision));
    QString buildDateInfo;
#ifdef QTC_SHOW_BUILD_DATE
     buildDateInfo = Tr::tr("<br/>Built on %1 %2<br/>").arg(QLatin1String(__DATE__), QLatin1String(__TIME__));
#endif

    const QString br = QLatin1String("<br/>");
    const QStringList additionalInfoLines = ICore::additionalAboutInformation();
    const QString additionalInfo =
            QStringList(Utils::transform(additionalInfoLines, &QString::toHtmlEscaped)).join(br);

    const QString description
        = Tr::tr("<h3>%1</h3>"
                 "%2<br/>"
                 "%3"
                 "%4"
                 "%5"
                 "<br/>"
                 "Copyright 2008-%6 %7. All rights reserved.<br/>"
                 "<br/>"
                 "The program is provided AS IS with NO WARRANTY OF ANY KIND, "
                 "INCLUDING THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A "
                 "PARTICULAR PURPOSE.<br/>")
              .arg(ICore::versionString(),
                   ICore::buildCompatibilityString(),
                   buildDateInfo,
                   ideRev,
                   additionalInfo.isEmpty() ? QString() : br + additionalInfo + br,
                   appInfo.year,
                   appInfo.author)
          + "<br/>"
          + Tr::tr("The Qt logo as well as Qt®, Qt Quick®, Built with Qt®, Boot to Qt®, "
                   "Qt Quick Compiler®, Qt Enterprise®, Qt Mobile® and Qt Embedded® are "
                   "registered trademarks of The Qt Company Ltd.");

    QLabel *copyRightLabel = new QLabel(description);
    copyRightLabel->setWordWrap(true);
    copyRightLabel->setOpenExternalLinks(true);
    copyRightLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    QPushButton *closeButton = buttonBox->button(QDialogButtonBox::Close);
    QTC_CHECK(closeButton);
    buttonBox->addButton(closeButton, QDialogButtonBox::ButtonRole(QDialogButtonBox::RejectRole | QDialogButtonBox::AcceptRole));
    connect(buttonBox , &QDialogButtonBox::rejected, this, &QDialog::reject);

    QLabel *logoLabel = new QLabel;
    logoLabel->setPixmap(Icons::QTCREATORLOGO_BIG.pixmap());
    layout->addWidget(logoLabel , 0, 0, 1, 1);
    layout->addWidget(copyRightLabel, 0, 1, 4, 4);
    layout->addWidget(buttonBox, 4, 0, 1, 5);
}

bool VersionDialog::event(QEvent *event)
{
    if (event->type() == QEvent::ShortcutOverride) {
        auto ke = static_cast<QKeyEvent *>(event);
        if (ke->key() == Qt::Key_Escape && !ke->modifiers()) {
            ke->accept();
            return true;
        }
    }
    return QDialog::event(event);
}
