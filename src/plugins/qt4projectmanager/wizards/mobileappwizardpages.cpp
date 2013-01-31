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

#include "mobileappwizardpages.h"
#include "ui_mobileappwizardgenericoptionspage.h"
#include "ui_mobileappwizardmaemooptionspage.h"
#include "ui_mobileappwizardharmattanoptionspage.h"
#include <coreplugin/coreconstants.h>
#include <utils/fileutils.h>

#include <QTemporaryFile>
#include <QDesktopServices>
#include <QFileDialog>
#include <QFileDialog>
#include <QMessageBox>

namespace Qt4ProjectManager {
namespace Internal {

class MobileAppWizardGenericOptionsPagePrivate
{
    Ui::MobileAppWizardGenericOptionsPage ui;
    friend class MobileAppWizardGenericOptionsPage;
};

class MobileAppWizardMaemoOptionsPagePrivate
{
    Ui::MobileAppWizardMaemoOptionsPage ui;
    QSize iconSize;
    QString pngIcon;
    friend class MobileAppWizardMaemoOptionsPage;
};

class MobileAppWizardHarmattanOptionsPagePrivate
{
    Ui::MobileAppWizardHarmattanOptionsPage ui;
    QSize iconSize;
    QString pngIcon;
    friend class MobileAppWizardHarmattanOptionsPage;
};

MobileAppWizardGenericOptionsPage::MobileAppWizardGenericOptionsPage(QWidget *parent)
    : QWizardPage(parent)
    , d(new MobileAppWizardGenericOptionsPagePrivate)
{
    d->ui.setupUi(this);
    d->ui.orientationBehaviorComboBox->addItem(tr("Automatically Rotate Orientation"),
        AbstractMobileApp::ScreenOrientationAuto);
    d->ui.orientationBehaviorComboBox->addItem(tr("Lock to Landscape Orientation"),
        AbstractMobileApp::ScreenOrientationLockLandscape);
    d->ui.orientationBehaviorComboBox->addItem(tr("Lock to Portrait Orientation"),
        AbstractMobileApp::ScreenOrientationLockPortrait);
}

MobileAppWizardGenericOptionsPage::~MobileAppWizardGenericOptionsPage()
{
    delete d;
}

void MobileAppWizardGenericOptionsPage::setOrientation(AbstractMobileApp::ScreenOrientation orientation)
{
    QComboBox *const comboBox = d->ui.orientationBehaviorComboBox;
    for (int i = 0; i < comboBox->count(); ++i) {
        if (comboBox->itemData(i).toInt() == static_cast<int>(orientation)) {
            comboBox->setCurrentIndex(i);
            break;
        }
    }
}

AbstractMobileApp::ScreenOrientation MobileAppWizardGenericOptionsPage::orientation() const
{
    QComboBox *const comboBox = d->ui.orientationBehaviorComboBox;
    const int index = comboBox->currentIndex();
    return static_cast<AbstractMobileApp::ScreenOrientation>(comboBox->itemData(index).toInt());
}


MobileAppWizardMaemoOptionsPage::MobileAppWizardMaemoOptionsPage(QWidget *parent)
    : QWizardPage(parent)
    , d(new MobileAppWizardMaemoOptionsPagePrivate)
{
    d->ui.setupUi(this);
    d->iconSize = QSize(64, 64);
    d->ui.pngIconButton->setIconSize(d->iconSize);
    connect(d->ui.pngIconButton, SIGNAL(clicked()), this, SLOT(openPngIcon()));
}

MobileAppWizardMaemoOptionsPage::~MobileAppWizardMaemoOptionsPage()
{
    delete d;
}

QString MobileAppWizardMaemoOptionsPage::pngIcon() const
{
    return d->pngIcon;
}


class PngIconScaler : public QObject
{
    Q_OBJECT
public:
    PngIconScaler(const QSize &expectedSize, const QString &iconPath)
        : m_expectedSize(expectedSize)
        , m_iconPath(iconPath)
        , m_pixmap(iconPath)
    {
    }

    bool hasRightSize() const { return m_expectedSize == m_pixmap.size(); }
    QPixmap pixmap() const { return m_pixmap; }

    bool scale(QString *newPath) {
        const QMessageBox::StandardButton button
                = QMessageBox::warning(QApplication::activeWindow(),
                                       tr("Wrong Icon Size"),
                                       tr("The icon needs to be %1x%2 pixels big, "
                                          "but is not. Do you want Qt Creator to scale it?")
                                       .arg(m_expectedSize.width()).arg(m_expectedSize.height()),
                                       QMessageBox::Ok | QMessageBox::Cancel);
        if (button != QMessageBox::Ok)
            return false;

        m_pixmap = m_pixmap.scaled(m_expectedSize);
        Utils::TempFileSaver saver;
        saver.setAutoRemove(false);
        if (!saver.hasError())
            saver.setResult(m_pixmap.save(
                                saver.file(), QFileInfo(m_iconPath).suffix().toLatin1().constData()));
        if (!saver.finalize()) {
            QMessageBox::critical(QApplication::activeWindow(),
                                  tr("File Error"),
                                  tr("Could not copy icon file: %1").arg(saver.errorString()));
            return false;
        }
        *newPath = saver.fileName();
        return true;
    }
private:
    QSize m_expectedSize;
    QString m_iconPath;
    QPixmap m_pixmap;
};


void MobileAppWizardMaemoOptionsPage::setPngIcon(const QString &icon)
{
    QString actualIconPath;
    PngIconScaler scaler(d->iconSize, icon);
    if (scaler.hasRightSize()) {
        actualIconPath = icon;
    } else {
        if (!scaler.scale(&actualIconPath))
            return;
    }

    d->ui.pngIconButton->setIcon(scaler.pixmap());
    d->pngIcon = actualIconPath;
}

void MobileAppWizardMaemoOptionsPage::openPngIcon()
{
    const QString iconPath = QFileDialog::getOpenFileName(this,
        d->ui.appIconLabel->text(), d->pngIcon,
        QLatin1String("*.png"));
    if (!iconPath.isEmpty())
        setPngIcon(iconPath);
}

MobileAppWizardHarmattanOptionsPage::MobileAppWizardHarmattanOptionsPage(QWidget *parent)
    : QWizardPage(parent)
    , d(new MobileAppWizardHarmattanOptionsPagePrivate)
{
    d->ui.setupUi(this);
    d->iconSize = QSize(80, 80);
    d->ui.pngIconButton->setIconSize(d->iconSize);
    connect(d->ui.pngIconButton, SIGNAL(clicked()), this, SLOT(openPngIcon()));
}

MobileAppWizardHarmattanOptionsPage::~MobileAppWizardHarmattanOptionsPage()
{
    delete d;
}

QString MobileAppWizardHarmattanOptionsPage::pngIcon() const
{
    return d->pngIcon;
}

void MobileAppWizardHarmattanOptionsPage::setPngIcon(const QString &icon)
{
    QString actualIconPath;
    PngIconScaler scaler(d->iconSize, icon);
    if (scaler.hasRightSize()) {
        actualIconPath = icon;
    } else {
        if (!scaler.scale(&actualIconPath))
            return;
    }

    d->ui.pngIconButton->setIcon(scaler.pixmap());
    d->pngIcon = actualIconPath;
}

void MobileAppWizardHarmattanOptionsPage::openPngIcon()
{
    const QString iconPath = QFileDialog::getOpenFileName(this,
        d->ui.appIconLabel->text(), d->pngIcon,
        QLatin1String("*.png"));
    if (!iconPath.isEmpty())
        setPngIcon(iconPath);
}

void MobileAppWizardHarmattanOptionsPage::setBoosterOptionEnabled(bool enable)
{
    d->ui.makeBoostableCheckBox->setEnabled(enable);
    d->ui.makeBoostableCheckBox->setChecked(enable);
}

bool MobileAppWizardHarmattanOptionsPage::supportsBooster() const
{
    return d->ui.makeBoostableCheckBox->isChecked();
}

} // namespace Internal
} // namespace Qt4ProjectManager

#include "mobileappwizardpages.moc"
