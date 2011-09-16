/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
** Copyright (c) 2010 Denis Mingulov.
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

#include "imageviewer.h"
#include "imageviewerfile.h"
#include "imagevieweractionhandler.h"
#include "imageviewerconstants.h"
#include "imageview.h"
#include "ui_imageviewertoolbar.h"

#include <coreplugin/icore.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <utils/qtcassert.h>

#include <QtCore/QMap>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtGui/QWidget>
#include <QtCore/QtDebug>

namespace ImageViewer {
namespace Internal {

struct ImageViewerPrivate
{
    QString displayName;
    ImageViewerFile *file;
    ImageView *imageView;
    QWidget *toolbar;
    Ui::ImageViewerToolbar ui_toolbar;
};

ImageViewer::ImageViewer(QWidget *parent)
    : IEditor(parent),
    d(new ImageViewerPrivate)
{
    d->file = new ImageViewerFile(this);
    d->imageView = new ImageView();

    setContext(Core::Context(Constants::IMAGEVIEWER_ID));
    setWidget(d->imageView);

    // toolbar
    d->toolbar = new QWidget();
    d->ui_toolbar.setupUi(d->toolbar);

    // icons update - try to use system theme
    updateButtonIconByTheme(d->ui_toolbar.toolButtonZoomIn, "zoom-in");
    updateButtonIconByTheme(d->ui_toolbar.toolButtonZoomOut, "zoom-out");
    updateButtonIconByTheme(d->ui_toolbar.toolButtonOriginalSize, "zoom-original");
    updateButtonIconByTheme(d->ui_toolbar.toolButtonFitToScreen, "zoom-fit-best");
    // a display - something is on the background
    updateButtonIconByTheme(d->ui_toolbar.toolButtonBackground, "video-display");
    // "emblem to specify the directory where the user stores photographs"
    // (photograph has outline - piece of paper)
    updateButtonIconByTheme(d->ui_toolbar.toolButtonOutline, "emblem-photos");

    // connections
    connect(d->file, SIGNAL(changed()), this, SIGNAL(changed()));

    connect(d->ui_toolbar.toolButtonZoomIn, SIGNAL(clicked()),
            d->imageView, SLOT(zoomIn()));
    connect(d->ui_toolbar.toolButtonZoomOut, SIGNAL(clicked()),
            d->imageView, SLOT(zoomOut()));
    connect(d->ui_toolbar.toolButtonFitToScreen, SIGNAL(clicked()),
            d->imageView, SLOT(fitToScreen()));
    connect(d->ui_toolbar.toolButtonOriginalSize, SIGNAL(clicked()),
            d->imageView, SLOT(resetToOriginalSize()));
    connect(d->ui_toolbar.toolButtonBackground, SIGNAL(toggled(bool)),
            d->imageView, SLOT(setViewBackground(bool)));
    connect(d->ui_toolbar.toolButtonOutline, SIGNAL(toggled(bool)),
            d->imageView, SLOT(setViewOutline(bool)));
    connect(d->imageView, SIGNAL(scaleFactorChanged(qreal)),
            this, SLOT(scaleFactorUpdate(qreal)));
}

ImageViewer::~ImageViewer()
{
    delete d->imageView;
    delete d->toolbar;
    delete d;
}

bool ImageViewer::createNew(const QString &contents)
{
    Q_UNUSED(contents)
    return false;
}

bool ImageViewer::open(QString *errorString, const QString &fileName, const QString &realFileName)
{
    if (!d->imageView->openFile(realFileName)) {
        *errorString = tr("Cannot open image file %1").arg(QDir::toNativeSeparators(realFileName));
        return false;
    }
    setDisplayName(QFileInfo(fileName).fileName());
    d->file->setFileName(fileName);
    // d_ptr->file->setMimeType
    emit changed();
    return true;
}

Core::IFile *ImageViewer::file()
{
    return d->file;
}

QString ImageViewer::id() const
{
    return QLatin1String(Constants::IMAGEVIEWER_ID);
}

QString ImageViewer::displayName() const
{
    return d->displayName;
}

void ImageViewer::setDisplayName(const QString &title)
{
    d->displayName = title;
    emit changed();
}

bool ImageViewer::duplicateSupported() const
{
    return false;
}

Core::IEditor *ImageViewer::duplicate(QWidget *parent)
{
    Q_UNUSED(parent);
    return 0;
}

QByteArray ImageViewer::saveState() const
{
    return QByteArray();
}

bool ImageViewer::restoreState(const QByteArray &state)
{
    Q_UNUSED(state);
    return true;
}

int ImageViewer::currentLine() const
{
    return 0;
}

int ImageViewer::currentColumn() const
{
    return 0;
}

bool ImageViewer::isTemporary() const
{
    return false;
}

QWidget *ImageViewer::toolBar()
{
    return d->toolbar;
}

void ImageViewer::scaleFactorUpdate(qreal factor)
{
    const QString info = QString::number(factor * 100, 'f', 2) + QLatin1Char('%');
    d->ui_toolbar.labelInfo->setText(info);
}

bool ImageViewer::updateButtonIconByTheme(QAbstractButton *button, const QString &name)
{
    QTC_ASSERT(button, return false);
    QTC_ASSERT(!name.isEmpty(), return false);

    if (QIcon::hasThemeIcon(name)) {
        button->setIcon(QIcon::fromTheme(name));
        return true;
    }

    return false;
}

void ImageViewer::switchViewBackground()
{
    d->ui_toolbar.toolButtonBackground->click();
}

void ImageViewer::switchViewOutline()
{
    d->ui_toolbar.toolButtonOutline->click();
}

void ImageViewer::zoomIn()
{
    d->ui_toolbar.toolButtonZoomIn->click();
}

void ImageViewer::zoomOut()
{
    d->ui_toolbar.toolButtonZoomOut->click();
}

void ImageViewer::resetToOriginalSize()
{
    d->ui_toolbar.toolButtonOriginalSize->click();
}

void ImageViewer::fitToScreen()
{
    d->ui_toolbar.toolButtonFitToScreen->click();
}

} // namespace Internal
} // namespace ImageViewer
