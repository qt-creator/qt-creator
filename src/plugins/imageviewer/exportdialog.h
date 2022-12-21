// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QDialog>

QT_FORWARD_DECLARE_CLASS(QSpinBox)

namespace Utils { class PathChooser; }

namespace ImageViewer::Internal {

struct ExportData;

class ExportDialog : public QDialog
{
public:
    explicit ExportDialog(QWidget *parent = nullptr);

    QSize exportSize() const;
    void setExportSize(const QSize &);

    QString exportFileName() const;
    void setExportFileName(const QString &);

    ExportData exportData() const;

    void accept() override;

    static QString imageNameFilterString();

private:
    void resetExportSize();
    void exportWidthChanged(int width);
    void exportHeightChanged(int height);

    void setExportWidthBlocked(int width);
    void setExportHeightBlocked(int height);

    Utils::PathChooser *m_pathChooser;
    QSpinBox *m_widthSpinBox;
    QSpinBox *m_heightSpinBox;
    QSize m_defaultSize;
    qreal m_aspectRatio;
};

} // ImageViewer::Internal
