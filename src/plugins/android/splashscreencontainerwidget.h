// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QVector>
#include <QStackedWidget>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QColor;
class QComboBox;
class QToolButton;
QT_END_NAMESPACE

namespace TextEditor { class TextEditorWidget; }

namespace Android {
namespace Internal {

class SplashScreenWidget;

class SplashScreenContainerWidget : public QStackedWidget
{
    Q_OBJECT
public:
    explicit SplashScreenContainerWidget(QWidget *parent,
                                         TextEditor::TextEditorWidget *textEditorWidget);
    void loadImages();
    bool hasImages() const;
    bool hasPortraitImages() const;
    bool hasLandscapeImages() const;
    bool isSticky() const;
    void setSticky(bool sticky);
    QString imageName() const;
    QString portraitImageName() const;
    QString landscapeImageName() const;
    void checkSplashscreenImage(const QString &name);
    bool isSplashscreenEnabled();
signals:
    void splashScreensModified();

private:
    void loadSplashscreenDrawParams(const QString &name);
    void setBackgroundColor(const QColor &color);
    void setImageShowMode(const QString &mode);
    void createSplashscreenThemes();
    void clearAll();

    TextEditor::TextEditorWidget *m_textEditorWidget = nullptr;
    QVector<SplashScreenWidget *> m_imageWidgets;
    QVector<SplashScreenWidget *> m_portraitImageWidgets;
    QVector<SplashScreenWidget *> m_landscapeImageWidgets;
    bool m_splashScreenSticky = false;
    QColor m_splashScreenBackgroundColor;
    QCheckBox *m_stickyCheck = nullptr;
    QComboBox *m_imageShowMode = nullptr;
    QToolButton *m_backgroundColor = nullptr;
    QToolButton *m_masterImage = nullptr;
    QToolButton *m_portraitMasterImage = nullptr;
    QToolButton *m_landscapeMasterImage = nullptr;
    QToolButton *m_convertSplashscreen = nullptr;
};

} // namespace Internal
} // namespace Android
