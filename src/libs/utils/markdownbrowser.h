// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "filepath.h"

#include <QTextBrowser>
#include <QTextFragment>

QT_BEGIN_NAMESPACE
class QNetworkAccessManager;
class QNetworkRequest;
QT_END_NAMESPACE

namespace Utils {

class QTCREATOR_UTILS_EXPORT MarkdownBrowser : public QTextBrowser
{
    Q_OBJECT
public:
    using RequestHook = std::function<void (QNetworkRequest *)>;

    MarkdownBrowser(QWidget *parent = nullptr);

    void setMarkdown(const QString &markdown);
    QString toMarkdown() const;
    void setBasePath(const FilePath &filePath);
    void setAllowRemoteImages(bool allow);
    void setNetworkAccessManager(QNetworkAccessManager *nam);
    void setRequestHook(const RequestHook &hook);
    void setMaximumCacheSize(qsizetype maxSize);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    void setMargins(const QMargins &margins);
    void setEnableCodeCopyButton(bool enable);

protected:
    void changeEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

    QMimeData *createMimeDataFromSelection() const override;

private:
    void handleAnchorClicked(const QUrl &link);
    void postProcessDocument(bool firstTime);
    void highlightCodeBlock(const QString &language, QTextBlock &block);
    std::optional<std::pair<QTextFragment, QRectF>> findCopyButtonFragmentAt(const QPoint& viewportPos);

private:
    bool m_enableCodeCopyButton = false;
    std::optional<QRectF> m_cachedCopyRect;
};

} // namespace Utils
