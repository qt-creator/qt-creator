// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>

#include <QToolButton>

namespace TextEditor { class TextEditorWidget; }

QT_BEGIN_NAMESPACE
class QLabel;
class QColor;
class QImage;
QT_END_NAMESPACE

namespace Android::Internal {

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

} // Android::Internal
