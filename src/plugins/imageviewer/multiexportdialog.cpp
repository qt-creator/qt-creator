/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "multiexportdialog.h"
#include "exportdialog.h"
#include "imageview.h" // ExportData

#include <coreplugin/coreicons.h>
#include <coreplugin/icore.h>

#include <utils/pathchooser.h>
#include <utils/utilsicons.h>

#include <QApplication>
#include <QDesktopWidget>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QMenu>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidgetAction>

#include <QImageWriter>

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QTextStream>

namespace ImageViewer {
namespace Internal {

static const int standardIconSizesValues[] = {16, 24, 32, 48, 64, 128, 256};

// Helpers to convert a size specifications from QString to QSize
// and vv. The format is '2x4' or '4' as shortcut for '4x4'.
static QSize sizeFromString(const QStringRef &r)
{
    if (r.isEmpty())
        return {};
    const int xPos = r.indexOf('x');
    bool ok;
    const int width = xPos < 0
        ? r.toInt(&ok)
        : r.left(xPos).toInt(&ok);
    if (!ok || width <= 0)
        return {};
    if (xPos < 0)
        return {width, width};
    const int height = r.mid(xPos + 1).toInt(&ok);
    if (!ok || height <= 0)
        return {};
    return {width, height};
}

static void appendSizeSpec(const QSize &size, QString *target)
{
    target->append(QString::number(size.width()));
    if (size.width() != size.height()) {
        target->append('x');
        target->append(QString::number(size.height()));
    }
}

static inline QString sizeToString(const QSize &size)
{
    QString result;
    appendSizeSpec(size, &result);
    return result;
}

static QString sizesToString(const QVector<QSize> &sizes)
{
    QString result;
    for (int i = 0, size = sizes.size(); i < size; ++i) {
        if (i)
            result.append(',');
        appendSizeSpec(sizes.at(i), &result);
    }
    return result;
}

static QVector<QSize> stringToSizes(const QString &s)
{
    QVector<QSize> result;
    const QString trimmed = s.trimmed();
    const QVector<QStringRef> &sizes = trimmed.splitRef(',', QString::SkipEmptyParts);
    result.reserve(sizes.size());
    for (const QStringRef &sizeSpec : sizes) {
        const QSize size = sizeFromString(sizeSpec);
        if (!size.isValid() || size.isEmpty())
            return {};
        else
            result.append(size);
    }
    return result;
}

static QString fileNameForSize(QString pattern, const QSize &s)
{
    pattern.replace("%1", QString::number(s.width()));
    pattern.replace("%2", QString::number(s.height()));
    return pattern;
}

// Helpers for writing/reading the user-specified size specifications
// from/to the settings.
static inline QString settingsGroup() { return QStringLiteral("ExportSvgSizes"); }

static QVector<QSize> readSettings(const QSize &size)
{
    QVector<QSize> result;
    QSettings *settings = Core::ICore::settings();
    settings->beginGroup(settingsGroup());
    const QStringList keys = settings->allKeys();
    const int idx = keys.indexOf(sizeToString(size));
    if (idx >= 0)
        result = stringToSizes(settings->value(keys.at(idx)).toString());
    settings->endGroup();
    return result;
}

static void writeSettings(const QSize &size, const QString &sizeSpec)
{
    QSettings *settings = Core::ICore::settings();
    settings->beginGroup(settingsGroup());
    const QString spec = sizeToString(size);
    settings->setValue(spec, QVariant(sizeSpec));

    // Limit the number of sizes to 10. Remove the
    // first element unless it is the newly added spec.
    QStringList keys = settings->allKeys();
    while (keys.size() > 10) {
        const int existingIndex = keys.indexOf(spec);
        const int removeIndex = existingIndex == 0 ? 1 : 0;
        settings->remove(keys.takeAt(removeIndex));
    }
    settings->endGroup();
}

QVector<QSize> MultiExportDialog::standardIconSizes()
{
    QVector<QSize> result;
    const int size = int(sizeof(standardIconSizesValues) / sizeof(standardIconSizesValues[0]));
    result.reserve(size);
    for (int standardIconSizesValue : standardIconSizesValues)
        result.append(QSize(standardIconSizesValue, standardIconSizesValue));
    return result;
}

// --- MultiExportDialog
MultiExportDialog::MultiExportDialog(QWidget *parent)
    : QDialog(parent)
    , m_pathChooser(new Utils::PathChooser(this))
    , m_sizesLineEdit(new QLineEdit)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    auto formLayout = new QFormLayout(this);

    m_pathChooser->setMinimumWidth(QApplication::desktop()->availableGeometry(this).width() / 5);
    m_pathChooser->setExpectedKind(Utils::PathChooser::SaveFile);
    m_pathChooser->setPromptDialogFilter(ExportDialog::imageNameFilterString());
    const QString pathChooserToolTip =
        tr("Enter a file name containing place holders %1 "
           "which will be replaced by the width and height of the image, respectively.")
          .arg("%1, %2");
    m_pathChooser->setToolTip(pathChooserToolTip);
    QLabel *pathChooserLabel = new QLabel(tr("File:"));
    pathChooserLabel->setToolTip(pathChooserToolTip);
    formLayout->addRow(pathChooserLabel, m_pathChooser);

    auto sizeEditButton = new QToolButton;
    sizeEditButton->setFocusPolicy(Qt::NoFocus);
    sizeEditButton->setIcon(Utils::Icons::ARROW_DOWN.icon());
    auto sizeEditMenu = new QMenu(this);
    sizeEditMenu->addAction(tr("Clear"),
                            m_sizesLineEdit, &QLineEdit::clear);
    sizeEditMenu->addAction(tr("Set Standard Icon Sizes"), this,
                            &MultiExportDialog::setStandardIconSizes);
    sizeEditMenu->addAction(tr("Generate Sizes"), this,
                            &MultiExportDialog::setGeneratedSizes);
    sizeEditButton->setMenu(sizeEditMenu);
    sizeEditButton->setPopupMode(QToolButton::InstantPopup);

    const QString sizesToolTip =
        tr("A comma-separated list of size specifications of the form \"<width>x<height>\".");
    auto sizesLabel = new QLabel(tr("Sizes:"));
    sizesLabel->setToolTip(sizesToolTip);
    formLayout->addRow(sizesLabel, m_sizesLineEdit);
    m_sizesLineEdit->setToolTip(sizesToolTip);
    auto optionsAction = new QWidgetAction(this);
    optionsAction->setDefaultWidget(sizeEditButton);
    m_sizesLineEdit->addAction(optionsAction, QLineEdit::TrailingPosition);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    formLayout->addRow(buttonBox);
}

void MultiExportDialog::setSizes(const QVector<QSize> &s)
{
    m_sizesLineEdit->setText(sizesToString(s));
}

QVector<QSize> MultiExportDialog::sizes() const
{
    return stringToSizes(sizesSpecification());
}

void MultiExportDialog::setStandardIconSizes()
{
    setSizes(standardIconSizes());
}

void MultiExportDialog::setGeneratedSizes()
{
    QVector<QSize> sizes;
    if (m_svgSize.width() >= 16)
        sizes.append(m_svgSize / 2);
    sizes.append(m_svgSize);
    for (int factor = 2; sizes.size() < 4; factor *= 2)
        sizes.append(m_svgSize * factor);
    setSizes(sizes);
}

void MultiExportDialog::suggestSizes()
{
    const QVector<QSize> settingsEntries = readSettings(m_svgSize);
    if (!settingsEntries.isEmpty())
        setSizes(settingsEntries);
    else if (m_svgSize.width() == m_svgSize.height()) // Square: Assume this is an icon
        setStandardIconSizes();
    else
        setGeneratedSizes();
}

QVector<ExportData> MultiExportDialog::exportData() const
{
    const QVector<QSize> sizeList = sizes();
    const QString pattern = exportFileName();
     QVector<ExportData> result;
     result.reserve(sizeList.size());
     for (const QSize &s : sizeList)
         result.append({fileNameForSize(pattern, s), s});
     return result;
}

QString MultiExportDialog::sizesSpecification() const
{
    return m_sizesLineEdit->text().trimmed();
}

void MultiExportDialog::accept()
{
    if (!m_pathChooser->isValid()) {
        QMessageBox::warning(this, windowTitle(), m_pathChooser->errorMessage());
        return;
    }

    const QString &sizeSpec = sizesSpecification();
    if (sizeSpec.isEmpty()) {
        QMessageBox::warning(this, windowTitle(), tr("Please specify some sizes."));
        return;
    }

    const QVector<ExportData> &data = exportData();
    if (data.isEmpty()) {
        QMessageBox::warning(this, windowTitle(),
                             tr("Invalid size specification: %1").arg(sizeSpec));
        return;
    }
    if (data.size() > 1 && data.at(0).fileName == data.at(1).fileName) {
        QMessageBox::warning(this, windowTitle(),
                             tr("The file name must contain one of the placeholders %1, %2.").arg("%1", "%2"));
        return;
    }

    writeSettings(m_svgSize, sizeSpec);

    QStringList existingFiles;
    for (const ExportData &d : data) {
        if (QFileInfo::exists(d.fileName))
            existingFiles.append(d.fileName);
    }
    if (!existingFiles.isEmpty()) {
        const QString message = existingFiles.size() == 1
            ? tr("The file %1 already exists.\nWould you like to overwrite it?")
                .arg(QDir::toNativeSeparators(existingFiles.constFirst()))
            : tr("The files %1 already exist.\nWould you like to overwrite them?")
                .arg(QDir::toNativeSeparators(existingFiles.join(", ")));
        QMessageBox messageBox(QMessageBox::Question, windowTitle(), message,
                               QMessageBox::Yes | QMessageBox::No, this);
        if (messageBox.exec() != QMessageBox::Yes)
            return;
    }

    QDialog::accept();
}

QString MultiExportDialog::exportFileName() const
{
    return m_pathChooser->fileName().toString();
}

void MultiExportDialog::setExportFileName(QString f)
{
    const int lastDot = f.lastIndexOf('.');
    if (lastDot != -1)
        f.insert(lastDot, "-%1");
    m_pathChooser->setFileName(Utils::FileName::fromString(f));
}

} // namespace Internal
} // namespace ImageViewer
