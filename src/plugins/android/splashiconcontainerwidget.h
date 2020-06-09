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
#include <QWidget>

QT_BEGIN_NAMESPACE
class QCheckBox;
QT_END_NAMESPACE

namespace TextEditor {
    class TextEditorWidget;
}

namespace Android {
namespace Internal {

class AndroidManifestEditorIconWidget;

class SplashIconContainerWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SplashIconContainerWidget(QWidget *parent,
                                       TextEditor::TextEditorWidget *textEditorWidget);
    void loadImages();
    bool hasImages() const;
    bool hasPortraitImages() const;
    bool hasLandscapeImages() const;
    bool isSticky() const;
    void setSticky(bool sticky);
    QString imageFileName() const;
    QString landscapeImageFileName() const;
    QString portraitImageFileName() const;
    void setImageFileName(const QString &name);
    void setLandscapeImageFileName(const QString &name);
    void setPortraitImageFileName(const QString &name);
signals:
    void splashScreensModified();
private:
    void imageSelected(const QString &path, AndroidManifestEditorIconWidget *iconWidget);
private:
    TextEditor::TextEditorWidget *m_textEditorWidget = nullptr;
    QVector<AndroidManifestEditorIconWidget *> m_imageButtons;
    QVector<AndroidManifestEditorIconWidget *> m_portraitImageButtons;
    QVector<AndroidManifestEditorIconWidget *> m_landscapeImageButtons;
    bool m_splashScreenSticky = false;
    QCheckBox *m_stickyCheck = nullptr;
    QString m_imageFileName = QLatin1String("logo");
    QString m_landscapeImageFileName = QLatin1String("logo_landscape");
    QString m_portraitImageFileName = QLatin1String("logo_portrait");
};

} // namespace Internal
} // namespace Android
