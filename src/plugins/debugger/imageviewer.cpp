/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "imageviewer.h"
#include "debuggerinternalconstants.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/idocument.h>

#include <utils/temporaryfile.h>

#include <QAction>
#include <QLabel>
#include <QMenu>
#include <QImage>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QScrollArea>
#include <QClipboard>
#include <QApplication>
#include <QPainter>
#include <QDir>

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
    update();
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
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(m_infoLabel);
    mainLayout->addWidget(m_scrollArea);
    m_scrollArea->setWidget(m_imageWidget);
    connect(m_imageWidget, &ImageWidget::clicked, this, &ImageViewer::clicked);
}

void ImageViewer::setImage(const QImage &image)
{
    m_imageWidget->setImage(image);
    clicked(QString());
}

void ImageViewer::setInfo(const QString &info)
{
    m_info = info;
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
    QString fileName;
    {
        Utils::TemporaryFile temporaryFile("qtcreatorXXXXXX.png");
        temporaryFile.setAutoRemove(false);
        image.save(&temporaryFile);
        fileName = temporaryFile.fileName();
        temporaryFile.close();
    }
    if (Core::IEditor *e = Core::EditorManager::openEditor(fileName))
        e->document()->setProperty(Debugger::Constants::OPENED_BY_DEBUGGER, QVariant(true));
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
    if (action == copyAction)
        QApplication::clipboard()->setImage(image);
    else if (action == imageViewerAction)
        openImageViewer(image);
}


//
//
//

PlotViewer::PlotViewer(QWidget *parent)
    : QWidget(parent)
{
}

void PlotViewer::setData(const PlotViewer::Data &data)
{
    m_data = data;
    update();
}

void PlotViewer::setInfo(const QString &description)
{
    m_info = description;
    update();
}

void PlotViewer::paintEvent(QPaintEvent *)
{
    QPainter pain(this);

    const int n = int(m_data.size());
    const int w = width();
    const int h = height();
    const int b = 10; // Border width.

    pain.fillRect(rect(), Qt::white);

    double ymin = 0;
    double ymax = 0;
    for (int i = 0; i < n; ++i) {
        const double v = m_data.at(i);
        if (v < ymin)
            ymin = v;
        else if (v > ymax)
            ymax = v;
    }

    const double d = ymin == ymax ? (h / 2 - b) : (ymax - ymin);
    const int k = 1; // Length of cross marker arms.

    for (int i = 0; i + 1 < n; ++i) {
        // Lines between points.
        const int x1 = b + i * (w - 2 * b) / (n - 1);
        const int x2 = b + (i + 1) * (w - 2 * b) / (n - 1);
        const int y1 = h - (b + int((m_data[i] - ymin) * (h - 2 * b) / d));
        const int y2 = h - (b + int((m_data[i + 1] - ymin) * (h - 2 * b) / d));
        pain.drawLine(x1, y1, x2, y2);

        if (i == 0) {
            // Little cross marker on first point
            pain.drawLine(x1 - k, y1 - k, x1 + k, y1 + k);
            pain.drawLine(x1 + k, y1 - k, x1 - k, y1 + k);
        }
        // ... and all subsequent points.
        pain.drawLine(x2 - k, y2 - k, x2 + k, y2 + k);
        pain.drawLine(x2 + k, y2 - k, x2 - k, y2 + k);
    }

    if (n) {
        pain.drawText(10, 10,
        QString::fromLatin1("%5 items. X: %1..%2, Y: %3...%4").arg(0).arg(n).arg(ymin).arg(ymax).arg(n));
    } else {
        pain.drawText(10, 10,
        QString::fromLatin1("Container is empty"));
    }
}

#include "imageviewer.moc"
