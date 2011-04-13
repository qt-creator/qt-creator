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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef IMAGEVIEWER_H
#define IMAGEVIEWER_H

#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/ifile.h>

#include <QtCore/QScopedPointer>
#include <QtCore/QStringList>

QT_FORWARD_DECLARE_CLASS(QAbstractButton)
QT_FORWARD_DECLARE_CLASS(QAction)

namespace ImageViewer {
namespace Internal {
class ImageViewerFile;

class ImageViewer : public Core::IEditor
{
    Q_OBJECT
    Q_DISABLE_COPY(ImageViewer)
public:
    explicit ImageViewer(QWidget *parent = 0);
    ~ImageViewer();

    Core::Context context() const;
    QWidget *widget();

    bool createNew(const QString &contents = QString());
    bool open(const QString &fileName = QString());
    Core::IFile *file();
    QString id() const;
    QString displayName() const;
    void setDisplayName(const QString &title);

    bool duplicateSupported() const;
    IEditor *duplicate(QWidget *parent);

    QByteArray saveState() const;
    bool restoreState(const QByteArray &state);

    int currentLine() const;
    int currentColumn() const;

    bool isTemporary() const;

    QWidget *toolBar();

public slots:
    void scaleFactorUpdate(qreal factor);

    void switchViewBackground();
    void switchViewOutline();
    void zoomIn();
    void zoomOut();
    void resetToOriginalSize();
    void fitToScreen();

private:
    /*!
      \brief Try to change button's icon to the one from the current theme.
      \param button Button where an icon should be changed
      \param name Icon name in the in the current icon theme
      \return true if icon is updated, false otherwise
     */
    bool updateButtonIconByTheme(QAbstractButton *button, const QString &name);

private:
    QScopedPointer<struct ImageViewerPrivate> d_ptr;
};

} // namespace Internal
} // namespace ImageViewer

#endif // IMAGEVIEWER_H
