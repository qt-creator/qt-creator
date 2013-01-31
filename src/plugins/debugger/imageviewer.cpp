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

#include "imageviewer.h"
#include "debuggerinternalconstants.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>

#include <QLabel>
#include <QVBoxLayout>
#include <QImage>
#include <QPixmap>
#include <QPoint>
#include <QMouseEvent>
#include <QScrollArea>
#include <QMenu>
#include <QAction>
#include <QKeySequence>
#include <QClipboard>
#include <QApplication>
#include <QPainter>
#include <QDir>
#include <QTemporaryFile>
#include <QVariant>

// Widget showing the image in a 1-pixel frame with context menu.
class ImageWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ImageWidget(QWidget *parent = 0) : QWidget(parent) {}

    void setImage(const QImage &image);
    const QImage &image() const { return  m_image; }

signals:
    void clicked(const QString &message);

protected:
    void paintEvent(QPaintEvent *);
    void mousePressEvent(QMouseEvent *ev);

private:
    QImage m_image;
};

void ImageWidget::setImage(const QImage &image)
{
    setFixedSize(image.size() + QSize(2, 2));
    m_image = image;
}

void ImageWidget::mousePressEvent(QMouseEvent *ev)
{
    const QPoint imagePos = ev->pos() - QPoint(1, 1);
    if (m_image.isNull() || imagePos.x() < 0 || imagePos.y() < 0 || imagePos.x() >= m_image.width() || imagePos.y() >= m_image.height()) {
        emit clicked(QString());
    } else {
        const QRgb color = m_image.pixel(imagePos);
        const QString message =
            ImageViewer::tr("Color at %1,%2: red: %3 green: %4 blue: %5 alpha: %6").
            arg(imagePos.x()).arg(imagePos.y()).
            arg(qRed(color)).arg(qGreen(color)).arg(qBlue(color)).arg(qAlpha(color));
        emit clicked(message);
    }
}

void ImageWidget::paintEvent(QPaintEvent *)
{
    if (m_image.isNull())
        return;
    QPainter painter(this);
    QRect rect(QPoint(0, 0), m_image.size() + QSize(1, 1));
    painter.drawRect(rect);
    painter.drawImage(QPoint(1, 1), m_image);
}

ImageViewer::ImageViewer(QWidget *parent)
    : QWidget(parent)
    , m_scrollArea(new QScrollArea(this))
    , m_imageWidget(new ImageWidget)
    , m_infoLabel(new QLabel)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(m_infoLabel);
    mainLayout->addWidget(m_scrollArea);
    m_scrollArea->setWidget(m_imageWidget);
    connect(m_imageWidget, SIGNAL(clicked(QString)), this, SLOT(clicked(QString)));
}

void ImageViewer::setImage(const QImage &i)
{
    m_imageWidget->setImage(i);
    m_info = tr("Size: %1x%2, %3 byte, format: %4, depth: %5")
            .arg(i.width()).arg(i.height()).arg(i.byteCount()).arg(i.format()).arg(i.depth());
    clicked(QString());
}

void ImageViewer::clicked(const QString &message)
{
    const QString text = m_info + QLatin1Char('\n')
        + (message.isEmpty() ? tr("<Click to display color>") : message);
    m_infoLabel->setText(text);
}

// Open Qt Creator's image viewer
static void openImageViewer(const QImage &image)
{
    QString fileName = QDir::tempPath();
    if (!fileName.endsWith(QLatin1Char('/')))
        fileName += QLatin1Char('/');
    fileName += QLatin1String("qtcreatorXXXXXX.png");
    {
        QTemporaryFile temporaryFile(fileName);
        temporaryFile.setAutoRemove(false);
        image.save(&temporaryFile);
        fileName = temporaryFile.fileName();
        temporaryFile.close();
    }
    if (Core::IEditor *e = Core::EditorManager::instance()->openEditor(fileName))
        e->setProperty(Debugger::Constants::OPENED_BY_DEBUGGER, QVariant(true));
}

void ImageViewer::contextMenuEvent(QContextMenuEvent *ev)
{
    // Offer copy and open in Creator's image viewer.
    const QImage &image = m_imageWidget->image();
    const bool hasImage = !image.isNull();
    QMenu menu;
    QAction *copyAction = menu.addAction(tr("Copy Image"));
    copyAction->setShortcut(QKeySequence::Copy);
    QAction *imageViewerAction = menu.addAction(tr("Open Image Viewer"));
    copyAction->setEnabled(hasImage);
    imageViewerAction->setEnabled(hasImage);
    QAction *action = menu.exec(ev->globalPos());
    if (action == copyAction) {
        QApplication::clipboard()->setImage(image);
    } else if (action == imageViewerAction) {
        openImageViewer(image);
    }
}

#include "imageviewer.moc"
