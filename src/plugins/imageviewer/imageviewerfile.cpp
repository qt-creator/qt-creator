/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
** Copyright (c) 2010 Denis Mingulov.
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "imageviewerfile.h"
#include "imageviewer.h"

#include <coreplugin/icore.h>
#include <coreplugin/id.h>

#include <utils/reloadpromptutils.h>

#include <QtCore/QMap>
#include <QtCore/QFileInfo>
#include <QtCore/QtDebug>

namespace ImageViewer {
namespace Internal {

struct ImageViewerFilePrivate
{
    QString fileName;
    QString mimeType;
    ImageViewer *editor;
};

ImageViewerFile::ImageViewerFile(ImageViewer *parent)
    : Core::IFile(parent),
    d(new ImageViewerFilePrivate)
{
    d->editor = parent;
}

ImageViewerFile::~ImageViewerFile()
{
    delete d;
}

bool ImageViewerFile::reload(QString *errorString,
                             Core::IFile::ReloadFlag flag,
                             Core::IFile::ChangeType type)
{
    if (flag == FlagIgnore)
        return true;
    if (type == TypePermissions) {
        emit changed();
        return true;
    }
    return d->editor->open(errorString, d->fileName, d->fileName);
}

bool ImageViewerFile::save(QString *errorString, const QString &fileName, bool autoSave)
{
    Q_UNUSED(errorString)
    Q_UNUSED(fileName);
    Q_UNUSED(autoSave)
    return false;
}

void ImageViewerFile::rename(const QString &newName)
{
    d->fileName = newName;
}

QString ImageViewerFile::fileName() const
{
    return d->fileName;
}

QString ImageViewerFile::defaultPath() const
{
    return QString();
}

QString ImageViewerFile::suggestedFileName() const
{
    return QString();
}

QString ImageViewerFile::mimeType() const
{
    return d->mimeType;
}

bool ImageViewerFile::isModified() const
{
    return false;
}

bool ImageViewerFile::isReadOnly() const
{
    return true;
}

bool ImageViewerFile::isSaveAsAllowed() const
{
    return false;
}

void ImageViewerFile::setMimetype(const QString &mimetype)
{
    d->mimeType = mimetype;
    emit changed();
}

void ImageViewerFile::setFileName(const QString &filename)
{
    d->fileName = filename;
    emit changed();
}

} // namespace Internal
} // namespace ImageViewer
