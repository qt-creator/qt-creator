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

#pragma once

#include <QDialog>

#include <QPair>
#include <QSize>
#include <QVector>

QT_FORWARD_DECLARE_CLASS(QLineEdit)

namespace Utils { class PathChooser; }

namespace ImageViewer {
namespace Internal {

struct ExportData;

class MultiExportDialog : public QDialog
{
    Q_OBJECT
public:
    explicit MultiExportDialog(QWidget *parent = nullptr);

    QString exportFileName() const;
    void setExportFileName(QString);

    void accept() override;

    void setSizes(const QVector<QSize> &);
    QVector<QSize> sizes() const;

    QVector<ExportData> exportData() const;

    static QVector<QSize> standardIconSizes();

    QSize svgSize() const { return m_svgSize; }
    void setSvgSize(const QSize &svgSize) { m_svgSize = svgSize; }

public slots:
    void setStandardIconSizes();
    void setGeneratedSizes();
    void suggestSizes();

private:
    QString sizesSpecification() const;

    Utils::PathChooser *m_pathChooser;
    QLineEdit *m_sizesLineEdit;
    QSize m_svgSize;

};

} // namespace Internal
} // namespace ImageViewer
