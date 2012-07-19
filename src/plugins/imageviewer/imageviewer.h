/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Copyright (c) 2010 Denis Mingulov.
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#ifndef IMAGEVIEWER_H
#define IMAGEVIEWER_H

#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/idocument.h>

#include <QScopedPointer>
#include <QStringList>

QT_BEGIN_NAMESPACE
class QAbstractButton;
class QAction;
QT_END_NAMESPACE

namespace ImageViewer {
namespace Internal {
class ImageViewerFile;

class ImageViewer : public Core::IEditor
{
    Q_OBJECT

public:
    explicit ImageViewer(QWidget *parent = 0);
    ~ImageViewer();

    bool createNew(const QString &contents = QString());
    bool open(QString *errorString, const QString &fileName, const QString &realFileName);
    Core::IDocument *document();
    Core::Id id() const;
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
    void imageSizeUpdated(const QSize &size);
    void scaleFactorUpdate(qreal factor);

    void switchViewBackground();
    void switchViewOutline();
    void zoomIn();
    void zoomOut();
    void resetToOriginalSize();
    void fitToScreen();
    void togglePlay();

private slots:
    void playToggled();

private:
    /*!
      \brief Try to change button's icon to the one from the current theme.
      \param button Button where an icon should be changed
      \param name Icon name in the in the current icon theme
      \return true if icon is updated, false otherwise
     */
    bool updateButtonIconByTheme(QAbstractButton *button, const QString &name);
    void setPaused(bool paused);

private:
    struct ImageViewerPrivate *d;
};

} // namespace Internal
} // namespace ImageViewer

#endif // IMAGEVIEWER_H
