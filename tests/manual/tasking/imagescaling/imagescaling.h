// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef IMAGESCALING_H
#define IMAGESCALING_H

#include <QtWidgets>
#include <QtConcurrent>
#include <QNetworkAccessManager>
#include <optional>

class DownloadDialog;
class Images : public QWidget
{
Q_OBJECT
public:
    Images(QWidget *parent = nullptr);
    ~Images();

    void initLayout(qsizetype count);

    QFuture<QByteArray> download(const QList<QUrl> &urls);
    void updateStatus(const QString &msg);
    void showImages(const QList<QImage> &images);
    void abortDownload();

public slots:
    void process();
    void cancel();

private slots:
    void scaleFinished();

private:
    using OptionalImages = std::optional<QList<QImage>>;
    static OptionalImages scaled(const QList<QByteArray> &data);

    QPushButton *addUrlsButton;
    QPushButton *cancelButton;
    QVBoxLayout *mainLayout;
    QList<QLabel *> labels;
    QGridLayout *imagesLayout;
    QStatusBar *statusBar;
    DownloadDialog *downloadDialog;

    QNetworkAccessManager qnam;
    QList<QSharedPointer<QNetworkReply>> replies;
    QFuture<QByteArray> downloadFuture;
    QFutureWatcher<OptionalImages> scalingWatcher;
};

#endif // IMAGESCALING_H
