/****************************************************************************
**
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

#ifndef IMAGEVIEWER_H
#define IMAGEVIEWER_H

#include <QWidget>

#include <vector>

QT_BEGIN_NAMESPACE
class QScrollArea;
class QLabel;
class QImage;
class QContextMenuEvent;
QT_END_NAMESPACE

class ImageWidget;

// Image viewer showing images in scroll area, displays color on click.
class ImageViewer : public QWidget
{
    Q_OBJECT
public:
    explicit ImageViewer(QWidget *parent = 0);

    void setImage(const QImage &image);
    void setInfo(const QString &description);

protected:
    void contextMenuEvent(QContextMenuEvent *);

private:
    void clicked(const QString &);

    QScrollArea *m_scrollArea;
    ImageWidget *m_imageWidget;
    QLabel *m_infoLabel;
    QString m_info;
};

class PlotViewer : public QWidget
{
    Q_OBJECT
public:
    explicit PlotViewer(QWidget *parent = 0);

    typedef std::vector<double> Data;
    void setData(const Data &data);
    void setInfo(const QString &description);

    void paintEvent(QPaintEvent *ev);

private:
    Data m_data;
    QString m_info;
};

#endif // IMAGEVIEWER_H
