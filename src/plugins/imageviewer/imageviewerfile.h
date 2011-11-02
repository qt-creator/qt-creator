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

#ifndef IMAGEVIEWERFILE_H
#define IMAGEVIEWERFILE_H

#include <coreplugin/ifile.h>

#include <QtCore/QScopedPointer>

namespace ImageViewer {
namespace Internal {

class ImageViewer;

class ImageViewerFile : public Core::IFile
{
    Q_OBJECT
public:
    explicit ImageViewerFile(ImageViewer *parent = 0);
    ~ImageViewerFile();

    bool save(QString *errorString, const QString &fileName, bool autoSave);
    void rename(const QString &newName);
    QString fileName() const;

    QString defaultPath() const;
    QString suggestedFileName() const;
    QString mimeType() const;

    bool isModified() const;
    bool isReadOnly() const;
    bool isSaveAsAllowed() const;

    bool reload(QString *errorString, ReloadFlag flag, ChangeType type);

    void setMimetype(const QString &mimetype);
    void setFileName(const QString &filename);

private:
    struct ImageViewerFilePrivate *d;
};

} // namespace Internal
} // namespace ImageViewer

#endif // IMAGEVIEWERFILE_H
