// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "multiexportdialog.h"

#include "exportdialog.h"
#include "imageview.h" // ExportData
#include "imageviewertr.h"

#include <coreplugin/coreicons.h>
#include <coreplugin/icore.h>

#include <utils/pathchooser.h>
#include <utils/stringutils.h>
#include <utils/utilsicons.h>

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QScreen>
#include <QToolButton>
#include <QWidgetAction>

using namespace Utils;

namespace ImageViewer::Internal {

static const int standardIconSizesValues[] = {16, 24, 32, 48, 64, 128, 256};

// Helpers to convert a size specifications from QString to QSize
// and vv. The format is '2x4' or '4' as shortcut for '4x4'.
static QSize sizeFromString(const QString &r)
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
    const QStringList &sizes = trimmed.split(',', Qt::SkipEmptyParts);
    result.reserve(sizes.size());
    for (const QString &sizeSpec : sizes) {
        const QSize size = sizeFromString(sizeSpec);
        if (!size.isValid() || size.isEmpty())
            return {};
        else
            result.append(size);
    }
    return result;
}

static FilePath fileNameForSize(QString pattern, const QSize &s)
{
    pattern.replace("%1", QString::number(s.width()));
    pattern.replace("%2", QString::number(s.height()));
    return FilePath::fromString(pattern);
}

// Helpers for writing/reading the user-specified size specifications
// from/to the settings.
static Key settingsGroup() { return "ExportSvgSizes"; }

static QVector<QSize> readSettings(const QSize &size)
{
    QVector<QSize> result;
    QtcSettings *settings = Core::ICore::settings();
    settings->beginGroup(settingsGroup());
    const QStringList keys = settings->allKeys();
    const int idx = keys.indexOf(sizeToString(size));
    if (idx >= 0)
        result = stringToSizes(settings->value(keyFromString(keys.at(idx))).toString());
    settings->endGroup();
    return result;
}

static void writeSettings(const QSize &size, const QString &sizeSpec)
{
    QtcSettings *settings = Core::ICore::settings();
    settings->beginGroup(settingsGroup());
    const QString spec = sizeToString(size);
    settings->setValue(keyFromString(spec), QVariant(sizeSpec));

    // Limit the number of sizes to 10. Remove the
    // first element unless it is the newly added spec.
    QStringList keys = settings->allKeys();
    while (keys.size() > 10) {
        const int existingIndex = keys.indexOf(spec);
        const int removeIndex = existingIndex == 0 ? 1 : 0;
        settings->remove(keyFromString(keys.takeAt(removeIndex)));
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
    auto formLayout = new QFormLayout(this);

    m_pathChooser->setMinimumWidth(screen()->availableGeometry().width() / 5);
    m_pathChooser->setExpectedKind(Utils::PathChooser::SaveFile);
    m_pathChooser->setPromptDialogFilter(ExportDialog::imageNameFilterString());
    const QString pathChooserToolTip =
        Tr::tr("Enter a file name containing place holders %1 "
           "which will be replaced by the width and height of the image, respectively.")
          .arg("%1, %2");
    m_pathChooser->setToolTip(pathChooserToolTip);
    QLabel *pathChooserLabel = new QLabel(Tr::tr("File:"));
    pathChooserLabel->setToolTip(pathChooserToolTip);
    formLayout->addRow(pathChooserLabel, m_pathChooser);

    auto sizeEditButton = new QToolButton;
    sizeEditButton->setFocusPolicy(Qt::NoFocus);
    sizeEditButton->setIcon(Utils::Icons::ARROW_DOWN.icon());
    auto sizeEditMenu = new QMenu(this);
    sizeEditMenu->addAction(Tr::tr("Clear"),
                            m_sizesLineEdit, &QLineEdit::clear);
    sizeEditMenu->addAction(Tr::tr("Set Standard Icon Sizes"), this,
                            &MultiExportDialog::setStandardIconSizes);
    sizeEditMenu->addAction(Tr::tr("Generate Sizes"), this,
                            &MultiExportDialog::setGeneratedSizes);
    sizeEditButton->setMenu(sizeEditMenu);
    sizeEditButton->setPopupMode(QToolButton::InstantPopup);

    const QString sizesToolTip =
        Tr::tr("A comma-separated list of size specifications of the form \"<width>x<height>\".");
    auto sizesLabel = new QLabel(Tr::tr("Sizes:"));
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
    const QString pattern = exportFileName().toString();
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
        QMessageBox::warning(this, windowTitle(), Tr::tr("Please specify some sizes."));
        return;
    }

    const QVector<ExportData> &data = exportData();
    if (data.isEmpty()) {
        QMessageBox::warning(this, windowTitle(),
                             Tr::tr("Invalid size specification: %1").arg(sizeSpec));
        return;
    }
    if (data.size() > 1 && data.at(0).filePath == data.at(1).filePath) {
        QMessageBox::warning(this, windowTitle(),
                             Tr::tr("The file name must contain one of the placeholders %1, %2.")
                                .arg(QString("%1"), QString("%2")));
        return;
    }

    writeSettings(m_svgSize, sizeSpec);

    FilePaths existingFiles;
    for (const ExportData &d : data) {
        if (d.filePath.exists())
            existingFiles.append(d.filePath);
    }
    if (!existingFiles.isEmpty()) {
        const QString message = existingFiles.size() == 1
            ? Tr::tr("The file %1 already exists.\nWould you like to overwrite it?")
                .arg(existingFiles.constFirst().toUserOutput())
            : Tr::tr("The files %1 already exist.\nWould you like to overwrite them?")
                .arg(FilePath::formatFilePaths(existingFiles, ", "));
        QMessageBox messageBox(QMessageBox::Question, windowTitle(), message,
                               QMessageBox::Yes | QMessageBox::No, this);
        if (messageBox.exec() != QMessageBox::Yes)
            return;
    }

    QDialog::accept();
}

FilePath MultiExportDialog::exportFileName() const
{
    return m_pathChooser->filePath();
}

void MultiExportDialog::setExportFileName(const FilePath &filePath)
{
    FilePath f = filePath;
    QString ff = f.path();
    const int lastDot = ff.lastIndexOf('.');
    if (lastDot != -1) {
        ff.insert(lastDot, "-%1");
        f = f.withNewPath(ff);
    };
    m_pathChooser->setFilePath(f);
}

} // ImageViewer:Internal
