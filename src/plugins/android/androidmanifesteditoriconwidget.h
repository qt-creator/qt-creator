// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>

#include <QWidget>

namespace TextEditor { class TextEditorWidget; }

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
    void setIconFromPath(const Utils::FilePath &iconPath);
    bool hasIcon() const;
    void setScaledToOriginalAspectRatio(bool scaled);
    void setScaledWithoutStretching(bool scaled);
    void setTargetIconFileName(const QString &targetIconFileName);
    void setTargetIconPath(const QString &targetIconPath);
    QString targetIconFileName() const;
    QString targetIconPath() const;

signals:
    void iconSelected(const Utils::FilePath &path);
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
    Utils::FilePath m_iconPath;
    QString m_targetIconPath;
    QString m_targetIconFileName;
    QString m_iconSelectionText;
    bool m_scaledToOriginalAspectRatio = false;
    bool m_scaledWithoutStretching = false;
};

} // namespace Internal
} // namespace Android
