// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QWidget>

#include <vector>

QT_BEGIN_NAMESPACE
class QScrollArea;
class QLabel;
class QImage;
class QContextMenuEvent;
QT_END_NAMESPACE

namespace Debugger::Internal {

class ImageWidget;

// Image viewer showing images in scroll area, displays color on click.
class ImageViewer : public QWidget
{
    Q_OBJECT

public:
    explicit ImageViewer(QWidget *parent = nullptr);

    void setImage(const QImage &image);
    void setInfo(const QString &description);

private:
    void contextMenuEvent(QContextMenuEvent *) final;
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
    explicit PlotViewer(QWidget *parent = nullptr);

    using Data = std::vector<double>;
    void setData(const Data &data);
    void setInfo(const QString &description);

    void paintEvent(QPaintEvent *ev) final;

private:
    Data m_data;
    QString m_info;
};

} // Debugger::Internal
