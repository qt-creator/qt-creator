/**************************************************************************
**
** Copyright (C) 2013 Denis Mingulov.
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

#include "imageviewerfile.h"
#include "imageviewer.h"

#include <coreplugin/icore.h>
#include <coreplugin/id.h>

#include <utils/reloadpromptutils.h>

#include <QMap>
#include <QFileInfo>
#include <QDebug>

namespace ImageViewer {
namespace Internal {

struct ImageViewerFilePrivate
{
    QString fileName;
    QString mimeType;
    ImageViewer *editor;
};

ImageViewerFile::ImageViewerFile(ImageViewer *parent)
    : Core::IDocument(parent),
    d(new ImageViewerFilePrivate)
{
    d->editor = parent;
}

ImageViewerFile::~ImageViewerFile()
{
    delete d;
}

Core::IDocument::ReloadBehavior ImageViewerFile::reloadBehavior(ChangeTrigger state, ChangeType type) const
{
    if (type == TypeRemoved || type == TypePermissions)
        return BehaviorSilent;
    if (type == TypeContents && state == TriggerInternal && !isModified())
        return BehaviorSilent;
    return BehaviorAsk;
}

bool ImageViewerFile::reload(QString *errorString,
                             Core::IDocument::ReloadFlag flag,
                             Core::IDocument::ChangeType type)
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
    const QString oldFilename = d->fileName;
    d->fileName = newName;
    d->editor->setDisplayName(QFileInfo(d->fileName).fileName());
    emit fileNameChanged(oldFilename, newName);
    emit changed();
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
