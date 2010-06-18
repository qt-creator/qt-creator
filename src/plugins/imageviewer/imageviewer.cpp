/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
** Copyright (c) 2010 Denis Mingulov.
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "imageviewer.h"
#include "imageviewerfile.h"
#include "imageviewerconstants.h"
#include "imageview.h"
#include "ui_imageviewertoolbar.h"

#include <coreplugin/icore.h>
#include <coreplugin/uniqueidmanager.h>

#include <QtCore/QMap>
#include <QtCore/QFileInfo>
#include <QtGui/QWidget>
#include <QtCore/QtDebug>

namespace ImageViewer {
namespace Internal {

struct ImageViewerPrivate
{
    QList<int> context;
    QString displayName;
    ImageViewerFile *file;
    ImageView *imageView;
    QWidget *toolbar;
    Ui::ImageViewerToolbar ui_toolbar;
};

ImageViewer::ImageViewer(QWidget *parent)
    : IEditor(parent),
    d_ptr(new ImageViewerPrivate)
{
    d_ptr->file = new ImageViewerFile(this);
    d_ptr->context << Core::ICore::instance()->uniqueIDManager()
            ->uniqueIdentifier(Constants::IMAGEVIEWER_ID);

    d_ptr->imageView = new ImageView();

    // toolbar
    d_ptr->toolbar = new QWidget();
    d_ptr->ui_toolbar.setupUi(d_ptr->toolbar);

    // connections
    connect(d_ptr->file, SIGNAL(changed()), this, SIGNAL(changed()));

    connect(d_ptr->ui_toolbar.toolButtonZoomIn, SIGNAL(clicked()),
            d_ptr->imageView, SLOT(zoomIn()));
    connect(d_ptr->ui_toolbar.toolButtonZoomOut, SIGNAL(clicked()),
            d_ptr->imageView, SLOT(zoomOut()));
    connect(d_ptr->ui_toolbar.toolButtonFitToScreen, SIGNAL(clicked()),
            d_ptr->imageView, SLOT(fitToScreen()));
    connect(d_ptr->ui_toolbar.toolButtonOriginalSize, SIGNAL(clicked()),
            d_ptr->imageView, SLOT(resetToOriginalSize()));
    connect(d_ptr->ui_toolbar.toolButtonBackground, SIGNAL(toggled(bool)),
            d_ptr->imageView, SLOT(setViewBackground(bool)));
    connect(d_ptr->ui_toolbar.toolButtonOutline, SIGNAL(toggled(bool)),
            d_ptr->imageView, SLOT(setViewOutline(bool)));
    connect(d_ptr->imageView, SIGNAL(scaleFactorChanged(qreal)),
            this, SLOT(scaleFactorUpdate(qreal)));
}

ImageViewer::~ImageViewer()
{
    delete d_ptr->imageView;
    delete d_ptr->toolbar;
}

QList<int> ImageViewer::context() const
{
    return d_ptr->context;
}

QWidget *ImageViewer::widget()
{
    return d_ptr->imageView;
}

bool ImageViewer::createNew(const QString &contents)
{
    Q_UNUSED(contents)
    return false;
}

bool ImageViewer::open(const QString &fileName)
{
    if (!d_ptr->imageView->openFile(fileName))
        return false;
    setDisplayName(QFileInfo(fileName).fileName());
    d_ptr->file->setFileName(fileName);
    // d_ptr->file->setMimeType
    emit changed();
    return true;
}

Core::IFile *ImageViewer::file()
{
    return d_ptr->file;
}

QString ImageViewer::id() const
{
    return QLatin1String(Constants::IMAGEVIEWER_ID);
}

QString ImageViewer::displayName() const
{
    return d_ptr->displayName;
}

void ImageViewer::setDisplayName(const QString &title)
{
    d_ptr->displayName = title;
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
    return d_ptr->toolbar;
}

void ImageViewer::scaleFactorUpdate(qreal factor)
{
    QString info = tr("%1%").arg(QString::number(factor * 100, 'f', 2));
    d_ptr->ui_toolbar.labelInfo->setText(info);
}

} // namespace Internal
} // namespace ImageViewer
