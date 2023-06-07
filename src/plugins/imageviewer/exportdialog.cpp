// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "exportdialog.h"

#include "imageview.h" // ExportData
#include "imageviewertr.h"

#include <coreplugin/coreicons.h>

#include <utils/pathchooser.h>

#include <QDialogButtonBox>
#include <QDir>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QImageWriter>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QMimeDatabase>
#include <QScreen>
#include <QSpinBox>
#include <QToolButton>

using namespace Utils;

namespace ImageViewer::Internal {

enum { exportMinimumSize = 1, exportMaximumSize = 2000 };

QString ExportDialog::imageNameFilterString()
{
    static QString result;
    if (result.isEmpty()) {
        QMimeDatabase mimeDatabase;
        const QString separator = ";;";
        const QList<QByteArray> mimeTypes = QImageWriter::supportedMimeTypes();
        for (const QByteArray &mimeType : mimeTypes) {
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
    auto formLayout = new QFormLayout(this);

    m_pathChooser->setMinimumWidth(screen()->availableGeometry().width() / 5);
    m_pathChooser->setExpectedKind(Utils::PathChooser::SaveFile);
    m_pathChooser->setPromptDialogFilter(imageNameFilterString());
    formLayout->addRow(Tr::tr("File:"), m_pathChooser);

    auto sizeLayout = new QHBoxLayout;
    m_widthSpinBox->setMinimum(exportMinimumSize);
    m_widthSpinBox->setMaximum(exportMaximumSize);
    connect(m_widthSpinBox, &QSpinBox::valueChanged, this, &ExportDialog::exportWidthChanged);
    sizeLayout->addWidget(m_widthSpinBox);
    //: Multiplication, as in 32x32
    sizeLayout->addWidget(new QLabel(Tr::tr("x")));
    m_heightSpinBox->setMinimum(exportMinimumSize);
    m_heightSpinBox->setMaximum(exportMaximumSize);
    connect(m_heightSpinBox, &QSpinBox::valueChanged, this, &ExportDialog::exportHeightChanged);
    sizeLayout->addWidget(m_heightSpinBox);
    auto resetButton = new QToolButton(this);
    resetButton->setIcon(QIcon(":/qt-project.org/styles/commonstyle/images/refresh-32.png"));
    sizeLayout->addWidget(resetButton);
    sizeLayout->addStretch();
    connect(resetButton, &QAbstractButton::clicked, this, &ExportDialog::resetExportSize);
    formLayout->addRow(Tr::tr("Size:"), sizeLayout);

    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
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
    const FilePath filePath = exportFileName();
    if (filePath.exists()) {
        const QString question = Tr::tr("%1 already exists.\nWould you like to overwrite it?")
            .arg(filePath.toUserOutput());
        if (QMessageBox::question(this, windowTitle(), question, QMessageBox::Yes | QMessageBox::No) !=  QMessageBox::Yes)
            return;
    }
    QDialog::accept();
}

QSize ExportDialog::exportSize() const
{
    return {m_widthSpinBox->value(), m_heightSpinBox->value()};
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

FilePath ExportDialog::exportFileName() const
{
    return m_pathChooser->filePath();
}

void ExportDialog::setExportFileName(const FilePath &f)
{
    m_pathChooser->setFilePath(f);
}

ExportData ExportDialog::exportData() const
{
    return {exportFileName(), exportSize()};
}

} // ImageViewer::Internal
