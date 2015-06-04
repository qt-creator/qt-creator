/**************************************************************************
**
** Copyright (C) 2015 Denis Mingulov.
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef IMAGEVIEWERFILE_H
#define IMAGEVIEWERFILE_H

#include <coreplugin/idocument.h>

QT_BEGIN_NAMESPACE
class QGraphicsItem;
class QMovie;
class QPixmap;

#ifndef QT_NO_SVG
class QGraphicsSvgItem;
#endif

QT_END_NAMESPACE

namespace ImageViewer {
namespace Internal {

class ImageViewer;

class ImageViewerFile : public Core::IDocument
{
    Q_OBJECT

public:
    enum ImageType {
        TypeInvalid,
        TypeSvg,
        TypeMovie,
        TypePixmap
    };

    ImageViewerFile();
    ~ImageViewerFile() override;

    OpenResult open(QString *errorString, const QString &fileName,
                    const QString &realFileName) override;
    bool save(QString *errorString, const QString &fileName, bool autoSave) override;
    bool setContents(const QByteArray &contents) override;

    QString defaultPath() const override;
    QString suggestedFileName() const override;

    bool isModified() const override;
    bool isSaveAsAllowed() const override;

    ReloadBehavior reloadBehavior(ChangeTrigger state, ChangeType type) const override;
    bool reload(QString *errorString, ReloadFlag flag, ChangeType type) override;

    bool isPaused() const;
    void setPaused(bool paused);

    QGraphicsItem *createGraphicsItem() const;
    ImageType type() const;

    void updateVisibility();

signals:
    void openFinished(bool success);
    void imageSizeChanged(const QSize &size);
    void isPausedChanged(bool paused);

private:
    void cleanUp();
    OpenResult openImpl(QString *errorString, const QString &fileName);

    ImageType m_type = TypeInvalid;
#ifndef QT_NO_SVG
    mutable QGraphicsSvgItem *m_tempSvgItem = 0;
#endif
    QMovie *m_movie = 0;
    QPixmap *m_pixmap = 0;
    bool m_isPaused = false;
};

} // namespace Internal
} // namespace ImageViewer

#endif // IMAGEVIEWERFILE_H
