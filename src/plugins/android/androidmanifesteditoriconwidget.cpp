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
        QWidget *parent, const QSize &buttonSize, const QString &title,
        const QString &tooltip,
        TextEditor::TextEditorWidget *textEditorWidget,
        const QString &targetIconPath)
    : QWidget(parent), m_buttonSize(buttonSize),
      m_textEditorWidget(textEditorWidget), m_targetIconPath(targetIconPath)
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
        m_scaleWarningLabel->setToolTip(tr("Icon scaled up"));
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
                this, &AndroidManifestEditorIconWidget::removeIcon);
    m_iconSelectionText = tooltip;
}

void AndroidManifestEditorIconWidget::setIcon(const QIcon &icon)
{
    m_button->setIcon(icon);
}

void AndroidManifestEditorIconWidget::loadIcon()
{
    QString baseDir = manifestDir(m_textEditorWidget);
    QString iconFile = baseDir + m_targetIconPath;
    setIconFromPath(iconFile);
}

void AndroidManifestEditorIconWidget::setIconFromPath(const QString &iconPath)
{
    if (!m_textEditorWidget) {
        iconSelected(iconPath);
        return;
    }
    m_iconPath = iconPath;
    QString baseDir = manifestDir(m_textEditorWidget);
    copyIcon();
    QString iconFile = baseDir + m_targetIconPath;
    m_button->setIcon(QIcon(iconFile));
}

void AndroidManifestEditorIconWidget::selectIcon()
{
    QString file = QFileDialog::getOpenFileName(this, m_iconSelectionText,
                                                QDir::homePath(), fileDialogIconFiles);
    if (file.isEmpty())
        return;
    setIconFromPath(file);
}

void AndroidManifestEditorIconWidget::removeIcon()
{
    QString baseDir = manifestDir(m_textEditorWidget);
    const QString targetPath = baseDir + m_targetIconPath;
    if (targetPath.isEmpty()) {
        qCDebug(androidManifestEditorLog) << "Icon target path empty, cannot remove icon.";
        return;
    }
    QFileInfo targetFile(targetPath);
    if (targetFile.exists()) {
        QDir rmRf(targetFile.absoluteDir());
        rmRf.removeRecursively();
    }
    setScaleWarningLabelVisible(false);
    m_button->setIcon(QIcon());
}

bool AndroidManifestEditorIconWidget::hasIcon()
{
    return !m_iconPath.isEmpty();
}

void AndroidManifestEditorIconWidget::setScaleWarningLabelVisible(bool visible)
{
    if (m_scaleWarningLabel)
        m_scaleWarningLabel->setVisible(visible);
}

void AndroidManifestEditorIconWidget::copyIcon()
{
    if (m_targetIconPath.isEmpty())
        return;
    QString baseDir = manifestDir(m_textEditorWidget);
    const QString targetPath = baseDir + m_targetIconPath;
    if (targetPath.isEmpty()) {
        qCDebug(androidManifestEditorLog) << "Icon target path empty, cannot copy icon.";
        return;
    }
    QFileInfo targetFile(targetPath);
    if (m_iconPath == targetPath)
        return;
    removeIcon();
    QImage original(m_iconPath);
    if (!targetPath.isEmpty() && !original.isNull()) {
        QDir dir;
        if (!dir.mkpath(QFileInfo(targetPath).absolutePath())) {
            qCDebug(androidManifestEditorLog) << "Cannot create icon target path.";
            m_iconPath.clear();
            return;
        }
        QSize targetSize = m_buttonSize;
        QImage scaled = original.scaled(targetSize.width(), targetSize.height(),
                                        Qt::KeepAspectRatio, Qt::SmoothTransformation);
        setScaleWarningLabelVisible(scaled.width() > original.width() || scaled.height() > original.height());
        scaled.save(targetPath);
        m_iconPath = m_targetIconPath;
    } else {
        m_iconPath.clear();
    }
}

} // namespace Internal
} // namespace Android
