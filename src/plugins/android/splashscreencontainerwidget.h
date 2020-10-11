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
