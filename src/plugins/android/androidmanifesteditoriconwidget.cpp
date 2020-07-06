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

#include "androidmanifesteditoriconwidget.h"

#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>
#include <utils/utilsicons.h>

#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QLabel>
#include <QLoggingCategory>
#include <QPainter>
#include <QToolButton>
#include <QVBoxLayout>

namespace Android {
namespace Internal {

namespace {
static Q_LOGGING_CATEGORY(androidManifestEditorLog, "qtc.android.manifestEditor", QtWarningMsg)
const auto fileDialogIconFiles = QWidget::tr("Images (*.png *.jpg *.webp *.svg)");
QString manifestDir(TextEditor::TextEditorWidget *textEditorWidget)
{
    // Get the manifest file's directory from its filepath.
    return textEditorWidget->textDocument()->filePath().toFileInfo().absolutePath();
}
}

AndroidManifestEditorIconWidget::AndroidManifestEditorIconWidget(QWidget *parent) : QWidget(parent)
{

}

AndroidManifestEditorIconWidget::AndroidManifestEditorIconWidget(
        QWidget *parent, const QSize &iconSize, const QSize &buttonSize, const QString &title,
        const QString &tooltip,
        TextEditor::TextEditorWidget *textEditorWidget,
        const QString &targetIconPath,
        const QString &targetIconFileName)
    : QWidget(parent), m_iconSize(iconSize), m_buttonSize(buttonSize),
      m_textEditorWidget(textEditorWidget),
      m_targetIconPath(targetIconPath), m_targetIconFileName(targetIconFileName)
{
    auto iconLayout = new QVBoxLayout(this);
    auto iconTitle = new QLabel(title, this);
    auto iconButtonLayout = new QGridLayout();
    m_button = new QToolButton(this);
    m_button->setMinimumSize(buttonSize);
    m_button->setMaximumSize(buttonSize);
    m_button->setToolTip(tooltip);
    m_button->setIconSize(buttonSize);
    QSize clearAndWarningSize(16, 16);
    QToolButton *clearButton = nullptr;
    if (textEditorWidget) {
        clearButton = new QToolButton(this);
        clearButton->setMinimumSize(clearAndWarningSize);
        clearButton->setMaximumSize(clearAndWarningSize);
        clearButton->setIcon(Utils::Icons::CLOSE_FOREGROUND.icon());
    }
    if (textEditorWidget) {
        m_scaleWarningLabel = new QLabel(this);
        m_scaleWarningLabel->setMinimumSize(clearAndWarningSize);
        m_scaleWarningLabel->setMaximumSize(clearAndWarningSize);
        m_scaleWarningLabel->setPixmap(Utils::Icons::WARNING.icon().pixmap(clearAndWarningSize));
        m_scaleWarningLabel->setToolTip(tr("Icon scaled up."));
        m_scaleWarningLabel->setVisible(false);
    }
    auto label = new QLabel(tr("Click to select..."), parent);
    iconLayout->addWidget(iconTitle);
    iconLayout->setAlignment(iconTitle, Qt::AlignHCenter);
    iconButtonLayout->setColumnMinimumWidth(0, 16);
    iconButtonLayout->addWidget(m_button, 0, 1, 1, 3);
    iconButtonLayout->setAlignment(m_button, Qt::AlignVCenter);
    if (textEditorWidget) {
        iconButtonLayout->addWidget(clearButton, 0, 4, 1, 1);
        iconButtonLayout->setAlignment(clearButton, Qt::AlignTop);
    }
    if (textEditorWidget) {
        iconButtonLayout->addWidget(m_scaleWarningLabel, 0, 0, 1, 1);
        iconButtonLayout->setAlignment(m_scaleWarningLabel, Qt::AlignTop);
    }
    iconLayout->addLayout(iconButtonLayout);
    iconLayout->setAlignment(iconButtonLayout, Qt::AlignHCenter);
    iconLayout->addWidget(label);
    iconLayout->setAlignment(label, Qt::AlignHCenter);
    this->setLayout(iconLayout);
    connect(m_button, &QAbstractButton::clicked,
            this, &AndroidManifestEditorIconWidget::selectIcon);
    if (clearButton)
        connect(clearButton, &QAbstractButton::clicked,
                this, &AndroidManifestEditorIconWidget::clearIcon);
    m_iconSelectionText = tooltip;
}

void AndroidManifestEditorIconWidget::setIcon(const QIcon &icon)
{
    m_button->setIcon(icon);
}

void AndroidManifestEditorIconWidget::clearIcon()
{
    removeIcon();
    iconRemoved();
}

void AndroidManifestEditorIconWidget::loadIcon()
{
    QString baseDir = manifestDir(m_textEditorWidget);
    QString iconFile = baseDir + m_targetIconPath + m_targetIconFileName;
    setIconFromPath(iconFile);
}

void AndroidManifestEditorIconWidget::setIconFromPath(const QString &iconPath)
{
    if (!m_textEditorWidget)
        return;
    m_iconPath = iconPath;
    QString baseDir = manifestDir(m_textEditorWidget);
    QImage original(iconPath);
    if (!original.isNull() && m_scaledToOriginalAspectRatio) {
        if ((original.width() > original.height() && m_buttonSize.height() > m_buttonSize.width())
                || (original.height() > original.width() && m_buttonSize.width() > m_buttonSize.height())) {
            auto width = m_buttonSize.height();
            auto height = m_buttonSize.width();
            m_buttonSize = QSize(width, height);
            m_button->setMinimumSize(m_buttonSize);
            m_button->setMaximumSize(m_buttonSize);
            m_button->setIconSize(m_buttonSize);
            auto targetWidth = m_iconSize.height();
            auto targetHeight = m_iconSize.width();
            m_iconSize = QSize(targetWidth, targetHeight);
        }
    }
    copyIcon();
    QString iconFile = baseDir + m_targetIconPath + m_targetIconFileName;
    m_button->setIcon(QIcon(iconFile));
}

void AndroidManifestEditorIconWidget::selectIcon()
{
    QString file = QFileDialog::getOpenFileName(this, m_iconSelectionText,
                                                QDir::homePath(), fileDialogIconFiles);
    if (file.isEmpty())
        return;
    setIconFromPath(file);
    iconSelected(file, this);
}

void AndroidManifestEditorIconWidget::removeIcon()
{
    QString baseDir = manifestDir(m_textEditorWidget);
    const QString targetPath = baseDir + m_targetIconPath + m_targetIconFileName;
    if (targetPath.isEmpty()) {
        qCDebug(androidManifestEditorLog) << "Icon target path empty, cannot remove icon.";
        return;
    }
    QFile targetFile(targetPath);
    targetFile.remove();
    m_iconPath.clear();
    setScaleWarningLabelVisible(false);
    m_button->setIcon(QIcon());
}

bool AndroidManifestEditorIconWidget::hasIcon() const
{
    return !m_iconPath.isEmpty();
}

void AndroidManifestEditorIconWidget::setScaledToOriginalAspectRatio(bool scaled)
{
    m_scaledToOriginalAspectRatio = scaled;
}

void AndroidManifestEditorIconWidget::setScaledWithoutStretching(bool scaled)
{
    m_scaledWithoutStretching = scaled;
}

void AndroidManifestEditorIconWidget::setTargetIconFileName(const QString &targetIconFileName)
{
    m_targetIconFileName = targetIconFileName;
}

void AndroidManifestEditorIconWidget::setTargetIconPath(const QString &targetIconPath)
{
    m_targetIconPath = targetIconPath;
}

QString AndroidManifestEditorIconWidget::targetIconFileName() const
{
    return m_targetIconFileName;
}

QString AndroidManifestEditorIconWidget::targetIconPath() const
{
    return m_targetIconPath;
}

void AndroidManifestEditorIconWidget::setScaleWarningLabelVisible(bool visible)
{
    if (m_scaleWarningLabel)
        m_scaleWarningLabel->setVisible(visible);
}

static QImage scaleWithoutStretching(const QImage& original, const QSize& targetSize)
{
    QImage ret(targetSize, QImage::Format_ARGB32);
    ret.fill(Qt::white);
    if (targetSize.height() > targetSize.width()) {
        // portrait target, scale to width and paint in the vertical middle
        QImage scaled = original.scaledToWidth(targetSize.width());
        int heightDiffHalf = (targetSize.height() - scaled.height()) / 2;
        QPainter painter(&ret);
        QRect targetRect(0, heightDiffHalf, targetSize.width(), scaled.height());
        QRect sourceRect(0, 0, scaled.width(), scaled.height());
        painter.drawImage(targetRect, scaled, sourceRect);
    } else if (targetSize.width() > targetSize.height()) {
        // landscape target, scale to height and paint in the horizontal middle
        QImage scaled = original.scaledToHeight(targetSize.height());
        int widthDiffHalf = (targetSize.width() - scaled.width()) / 2;
        QPainter painter(&ret);
        QRect targetRect(widthDiffHalf, 0, scaled.width(), targetSize.height());
        QRect sourceRect(0, 0, scaled.width(), scaled.height());
        painter.drawImage(targetRect, scaled, sourceRect);
    } else
        ret = original.scaled(targetSize.width(), targetSize.height(),
                              Qt::KeepAspectRatio, Qt::SmoothTransformation);
    return ret;
}

void AndroidManifestEditorIconWidget::copyIcon()
{
    if (m_targetIconPath.isEmpty())
        return;
    QString baseDir = manifestDir(m_textEditorWidget);
    const QString targetPath = baseDir + m_targetIconPath + m_targetIconFileName;
    if (targetPath.isEmpty()) {
        qCDebug(androidManifestEditorLog) << "Icon target path empty, cannot copy icon.";
        return;
    }
    QImage original(m_iconPath);
    if (m_iconPath != targetPath)
        removeIcon();
    if (original.isNull()) {
        m_iconPath.clear();
        return;
    }
    if (m_iconPath == targetPath)
        return;
    if (!targetPath.isEmpty() && !original.isNull()) {
        QDir dir;
        if (!dir.mkpath(QFileInfo(targetPath).absolutePath())) {
            qCDebug(androidManifestEditorLog) << "Cannot create icon target path.";
            m_iconPath.clear();
            return;
        }
        QImage scaled;
        if (!m_scaledWithoutStretching)
            scaled = original.scaled(m_iconSize.width(), m_iconSize.height(),
                                     Qt::KeepAspectRatio, Qt::SmoothTransformation);
        else
            scaled = scaleWithoutStretching(original, m_iconSize);
        setScaleWarningLabelVisible(scaled.width() > original.width() || scaled.height() > original.height());
        scaled.save(targetPath);
        m_iconPath = targetPath;
    } else {
        m_iconPath.clear();
    }
}

} // namespace Internal
} // namespace Android
