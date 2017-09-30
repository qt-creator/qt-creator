/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "exportdialog.h"

#include <coreplugin/coreicons.h>

#include <utils/pathchooser.h>

#include <QApplication>
#include <QDesktopWidget>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QToolButton>
#include <QVBoxLayout>

#include <QImageWriter>

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QMimeType>

namespace ImageViewer {
namespace Internal {

enum { exportMinimumSize = 1, exportMaximumSize = 2000 };

static QString imageNameFilterString()
{
    static QString result;
    if (result.isEmpty()) {
        QMimeDatabase mimeDatabase;
        const QString separator = QStringLiteral(";;");
        foreach (const QByteArray &mimeType, QImageWriter::supportedMimeTypes()) {
            const QString filter = mimeDatabase.mimeTypeForName(QLatin1String(mimeType)).filterString();
            if (!filter.isEmpty()) {
                if (mimeType == QByteArrayLiteral("image/png")) {
                    if (!result.isEmpty())
                        result.prepend(separator);
                    result.prepend(filter);
                } else {
                    if (!result.isEmpty())
                        result.append(separator);
                    result.append(filter);
                }
            }
        }
    }
    return result;
}

ExportDialog::ExportDialog(QWidget *parent)
    : QDialog(parent)
    , m_pathChooser(new Utils::PathChooser(this))
    , m_widthSpinBox(new QSpinBox(this))
    , m_heightSpinBox(new QSpinBox(this))
    , m_aspectRatio(1)
{
    typedef void (QSpinBox::*QSpinBoxIntSignal)(int);

    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    QFormLayout *formLayout = new QFormLayout(this);

    m_pathChooser->setMinimumWidth(QApplication::desktop()->availableGeometry(this).width() / 5);
    m_pathChooser->setExpectedKind(Utils::PathChooser::SaveFile);
    m_pathChooser->setPromptDialogFilter(imageNameFilterString());
    formLayout->addRow(tr("File:"), m_pathChooser);

    QHBoxLayout *sizeLayout = new QHBoxLayout;
    m_widthSpinBox->setMinimum(exportMinimumSize);
    m_widthSpinBox->setMaximum(exportMaximumSize);
    connect(m_widthSpinBox, static_cast<QSpinBoxIntSignal>(&QSpinBox::valueChanged),
            this, &ExportDialog::exportWidthChanged);
    sizeLayout->addWidget(m_widthSpinBox);
    //: Multiplication, as in 32x32
    sizeLayout->addWidget(new QLabel(tr("x")));
    m_heightSpinBox->setMinimum(exportMinimumSize);
    m_heightSpinBox->setMaximum(exportMaximumSize);
    connect(m_heightSpinBox, static_cast<QSpinBoxIntSignal>(&QSpinBox::valueChanged),
            this, &ExportDialog::exportHeightChanged);
    sizeLayout->addWidget(m_heightSpinBox);
    QToolButton *resetButton = new QToolButton(this);
    resetButton->setIcon(QIcon(QStringLiteral(":/qt-project.org/styles/commonstyle/images/refresh-32.png")));
    sizeLayout->addWidget(resetButton);
    sizeLayout->addStretch();
    connect(resetButton, &QAbstractButton::clicked, this, &ExportDialog::resetExportSize);
    formLayout->addRow(tr("Size:"), sizeLayout);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    formLayout->addRow(buttonBox);
}

void ExportDialog::accept()
{
    if (!m_pathChooser->isValid()) {
        QMessageBox::warning(this, windowTitle(), m_pathChooser->errorMessage());
        return;
    }
    const QString fileName = exportFileName();
    if (QFileInfo::exists(fileName)) {
        const QString question = tr("%1 already exists.\nWould you like to overwrite it?")
            .arg(QDir::toNativeSeparators(fileName));
        if (QMessageBox::question(this, windowTitle(), question, QMessageBox::Yes | QMessageBox::No) !=  QMessageBox::Yes)
            return;
    }
    QDialog::accept();
}

QSize ExportDialog::exportSize() const
{
    return QSize(m_widthSpinBox->value(), m_heightSpinBox->value());
}

void ExportDialog::setExportSize(const QSize &size)
{
    m_defaultSize = size;
    const QSizeF defaultSizeF(m_defaultSize);
    m_aspectRatio = defaultSizeF.width() / defaultSizeF.height();
    setExportWidthBlocked(size.width());
    setExportHeightBlocked(size.height());
}

void ExportDialog::resetExportSize()
{
    setExportWidthBlocked(m_defaultSize.width());
    setExportHeightBlocked(m_defaultSize.height());
}

void ExportDialog::setExportWidthBlocked(int width)
{
    if (m_widthSpinBox->value() != width) {
        QSignalBlocker blocker(m_widthSpinBox);
        m_widthSpinBox->setValue(width);
    }
}

void ExportDialog::setExportHeightBlocked(int height)
{
    if (m_heightSpinBox->value() != height) {
        QSignalBlocker blocker(m_heightSpinBox);
        m_heightSpinBox->setValue(height);
    }
}

void ExportDialog::exportWidthChanged(int width)
{
    const bool square = m_defaultSize.width() == m_defaultSize.height();
    setExportHeightBlocked(square ? width : qRound(qreal(width) / m_aspectRatio));
}

void ExportDialog::exportHeightChanged(int height)
{
    const bool square = m_defaultSize.width() == m_defaultSize.height();
    setExportWidthBlocked(square ? height : qRound(qreal(height) * m_aspectRatio));
}

QString ExportDialog::exportFileName() const
{
    return m_pathChooser->fileName().toString();
}

void ExportDialog::setExportFileName(const QString &f)
{
    m_pathChooser->setFileName(Utils::FileName::fromString(f));
}

} // namespace Internal
} // namespace ImageViewer
