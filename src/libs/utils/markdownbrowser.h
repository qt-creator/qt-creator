// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "filepath.h"

#include <QTextBrowser>

namespace Utils {

class QTCREATOR_UTILS_EXPORT MarkdownBrowser : public QTextBrowser
{
    Q_OBJECT

public:
    MarkdownBrowser(QWidget *parent = nullptr);

    void setMarkdown(const QString &markdown);
    void setBasePath(const FilePath &filePath);
    void setAllowRemoteImages(bool allow);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
};

} // namespace Utils
