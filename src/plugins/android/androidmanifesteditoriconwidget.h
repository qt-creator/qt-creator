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

#include <QWidget>

namespace TextEditor {
    class TextEditorWidget;
}

QT_BEGIN_NAMESPACE
class QLabel;
class QToolButton;
QT_END_NAMESPACE

namespace Android {
namespace Internal {

class AndroidManifestEditorIconWidget : public QWidget
{
    Q_OBJECT
public:
    explicit AndroidManifestEditorIconWidget(QWidget *parent);
    AndroidManifestEditorIconWidget(QWidget *parent,
                                    const QSize &iconSize,
                                    const QSize &buttonSize,
                                    const QString &title,
                                    const QString &tooltip,
                                    TextEditor::TextEditorWidget *textEditorWidget = nullptr,
                                    const QString &targetIconPath = {},
                                    const QString &targetIconFileName = {});
    void setIcon(const QIcon &icon);
    void clearIcon();
    void loadIcon();
    void setIconFromPath(const QString &iconPath);
    bool hasIcon() const;
    void setScaledToOriginalAspectRatio(bool scaled);
    void setScaledWithoutStretching(bool scaled);
    void setTargetIconFileName(const QString &targetIconFileName);
    void setTargetIconPath(const QString &targetIconPath);
    QString targetIconFileName() const;
    QString targetIconPath() const;
signals:
    void iconSelected(const QString &path, AndroidManifestEditorIconWidget* iconWidget);
    void iconRemoved();

private:
    void selectIcon();
    void removeIcon();
    void copyIcon();
    void setScaleWarningLabelVisible(bool visible);
private:
    QToolButton *m_button = nullptr;
    QSize m_iconSize;
    QSize m_buttonSize;
    QLabel *m_scaleWarningLabel = nullptr;
    TextEditor::TextEditorWidget *m_textEditorWidget = nullptr;
    QString m_iconPath;
    QString m_targetIconPath;
    QString m_targetIconFileName;
    QString m_iconSelectionText;
    bool m_scaledToOriginalAspectRatio = false;
    bool m_scaledWithoutStretching = false;
};

} // namespace Internal
} // namespace Android
