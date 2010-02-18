/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include <coreplugin/icore.h>
#include <coreplugin/uniqueidmanager.h>

#include <utils/reloadpromptutils.h>

#include <QtCore/QMap>
#include <QtCore/QFileInfo>
#include <QtGui/QImageReader>
#include <QtGui/QWidget>
#include <QtGui/QGridLayout>
#include <QtGui/QMainWindow>
#include <QtDebug>

namespace {
    const char * const IMAGE_VIEWER_ID = "Editors.ImageViewer";
    const char * const C_IMAGE_VIEWER = "Editors.ImageViewer";
}

using namespace BINEditor::Internal;
using namespace Core;

// #pragma mark -- ImageViewerFactory

ImageViewerFactory::ImageViewerFactory(QObject *parent) :
    Core::IEditorFactory(parent)
{
    QMap<QByteArray, QString> possibleMimeTypes;
    possibleMimeTypes.insert("bmp", QLatin1String("image/bmp"));
    possibleMimeTypes.insert("gif", QLatin1String("image/gif"));
    possibleMimeTypes.insert("ico", QLatin1String("image/x-icon"));
    possibleMimeTypes.insert("jpeg", QLatin1String("image/jpeg"));
    possibleMimeTypes.insert("jpg", QLatin1String("image/jpeg"));
    possibleMimeTypes.insert("mng", QLatin1String("video/x-mng"));
    possibleMimeTypes.insert("pbm", QLatin1String("image/x-portable-bitmap"));
    possibleMimeTypes.insert("pgm", QLatin1String("image/x-portable-graymap"));
    possibleMimeTypes.insert("png", QLatin1String("image/png"));
    possibleMimeTypes.insert("ppm", QLatin1String("image/x-portable-pixmap"));
    possibleMimeTypes.insert("svg", QLatin1String("image/svg+xml"));
    possibleMimeTypes.insert("tif", QLatin1String("image/tiff"));
    possibleMimeTypes.insert("tiff", QLatin1String("image/tiff"));
    possibleMimeTypes.insert("xbm", QLatin1String("image/xbm"));
    possibleMimeTypes.insert("xpm", QLatin1String("image/xpm"));

    QList<QByteArray> supportedFormats = QImageReader::supportedImageFormats();
    foreach (const QByteArray &format, supportedFormats) {
        const QString &value = possibleMimeTypes.value(format);
        if (!value.isEmpty())
            m_mimeTypes.append(value);
    }
}

IEditor *ImageViewerFactory::createEditor(QWidget *parent)
{
    return new ImageViewer(parent);
}

QStringList ImageViewerFactory::mimeTypes() const
{
    return m_mimeTypes;
}

QString ImageViewerFactory::id() const
{
    return QLatin1String(IMAGE_VIEWER_ID);
}

QString ImageViewerFactory::displayName() const
{
    return tr("Image Viewer");
}

Core::IFile *ImageViewerFactory::open(const QString & /* fileName */)
{
    return 0;
}

// #pragma mark -- ImageViewerFile

ImageViewerFile::ImageViewerFile(ImageViewer *parent)
 : Core::IFile(parent),
    m_editor(parent)
{

}

void ImageViewerFile::modified(ReloadBehavior * behavior)
{
    switch (*behavior) {
    case  Core::IFile::ReloadNone:
        return;
    case Core::IFile::ReloadUnmodified:
    case Core::IFile::ReloadAll:
        m_editor->open(m_fileName);
        return;
    case Core::IFile::ReloadPermissions:
        return;
    case Core::IFile::AskForReload:
        break;
    }

    switch (Utils::reloadPrompt(m_fileName, isModified(), Core::ICore::instance()->mainWindow())) {
    case Utils::ReloadCurrent:
        m_editor->open(m_fileName);
        break;
    case Utils::ReloadAll:
        m_editor->open(m_fileName);
        *behavior = Core::IFile::ReloadAll;
        break;
    case Utils::ReloadSkipCurrent:
        break;
    case Utils::ReloadNone:
        *behavior = Core::IFile::ReloadNone;
        break;
    }
}

// #pragma mark -- ImageViewer

ImageViewer::ImageViewer(QObject *parent)
    : IEditor(parent)
{
    m_file = new ImageViewerFile(this);
    m_context << Core::ICore::instance()->uniqueIDManager()->uniqueIdentifier(C_IMAGE_VIEWER);

    m_scrollArea = new QScrollArea;
    m_scrollArea->setWidgetResizable(true);
    m_imageview = new QWidget;
    QGridLayout *layout = new QGridLayout();
    m_label = new QLabel;
    m_label->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    m_label->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    layout->setMargin(0);
    m_imageview->setLayout(layout);
    layout->addWidget(m_label, 0, 0, 1, 1);
}

ImageViewer::~ImageViewer()
{
    delete m_scrollArea;
    // m_file deleted by parent hierarchy
}

QList<int> ImageViewer::context() const
{
    return m_context;
}

QWidget *ImageViewer::widget()
{
    return m_scrollArea;
}

bool ImageViewer::createNew(const QString &contents)
{
    Q_UNUSED(contents)
    return false;
}

bool ImageViewer::open(const QString &fileName)
{
    m_label->setPixmap(QPixmap(fileName));
    m_scrollArea->setWidget(m_imageview);
    setDisplayName(QFileInfo(fileName).fileName());
    m_file->setFileName(fileName);
    // m_file->setMimeType
    emit changed();
    return !m_label->pixmap()->isNull();
}

Core::IFile *ImageViewer::file()
{
    return m_file;
}

QString ImageViewer::id() const
{
    return QLatin1String(IMAGE_VIEWER_ID);
}

QString ImageViewer::displayName() const
{
    return m_displayName;
}

void ImageViewer::setDisplayName(const QString &title)
{
    m_displayName = title;
    emit changed();
}
