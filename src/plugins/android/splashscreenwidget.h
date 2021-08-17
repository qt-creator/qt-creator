/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#pragma once

#include <utils/filepath.h>

#include <QToolButton>

namespace TextEditor { class TextEditorWidget; }

QT_BEGIN_NAMESPACE
class QLabel;
class QColor;
class QImage;
QT_END_NAMESPACE

namespace Android {
namespace Internal {

class SplashScreenWidget : public QWidget
{
    Q_OBJECT

    class SplashScreenButton : public QToolButton
    {
    public:
        explicit SplashScreenButton(SplashScreenWidget *parent);
    private:
        void paintEvent(QPaintEvent *event) override;
        SplashScreenWidget *m_parentWidget;
    };

public:
    explicit SplashScreenWidget(QWidget *parent);
    SplashScreenWidget(QWidget *parent,
                       const QSize &size,
                       const QSize &screenSize,
                       const QString &title,
                       const QString &tooltip,
                       const QString &imagePath,
                       int scalingRatio, int maxScalingRatio,
                       TextEditor::TextEditorWidget *textEditorWidget = nullptr);

    bool hasImage() const;
    void clearImage();
    void setBackgroundColor(const QColor &backgroundColor);
    void showImageFullScreen(bool fullScreen);
    void setImageFromPath(const Utils::FilePath &imagePath, bool resize = true);
    void setImageFileName(const QString &imageFileName);
    void loadImage();

signals:
    void imageChanged();

private:
    void selectImage();
    void removeImage();
    void setScaleWarningLabelVisible(bool visible);

private:
    TextEditor::TextEditorWidget *m_textEditorWidget = nullptr;
    QLabel *m_scaleWarningLabel = nullptr;
    SplashScreenButton *m_splashScreenButton = nullptr;
    int m_scalingRatio, m_maxScalingRatio;
    QColor m_backgroundColor = Qt::white;
    QImage m_image;
    QPoint m_imagePosition;
    QString m_imagePath;
    QString m_imageFileName;
    QString m_imageSelectionText;
    QSize m_screenSize;
    bool m_showImageFullScreen = false;
};

} // namespace Internal
} // namespace Android
