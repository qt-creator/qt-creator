// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QDialog>
#include <QPair>
#include <QSize>
#include <QVector>

QT_BEGIN_NAMESPACE
class QLineEdit;
QT_END_NAMESPACE

namespace Utils {
class FilePath;
class PathChooser;
} // Utils

namespace ImageViewer::Internal {

struct ExportData;

class MultiExportDialog : public QDialog
{
public:
    explicit MultiExportDialog(QWidget *parent = nullptr);

    Utils::FilePath exportFileName() const;
    void setExportFileName(const Utils::FilePath &);

    void accept() override;

    void setSizes(const QVector<QSize> &);
    QVector<QSize> sizes() const;

    QVector<ExportData> exportData() const;

    static QVector<QSize> standardIconSizes();

    QSize svgSize() const { return m_svgSize; }
    void setSvgSize(const QSize &svgSize) { m_svgSize = svgSize; }

    void suggestSizes();

private:
    void setStandardIconSizes();
    void setGeneratedSizes();

    QString sizesSpecification() const;

    Utils::PathChooser *m_pathChooser;
    QLineEdit *m_sizesLineEdit;
    QSize m_svgSize;

};

} // ImageViewer::Internal
