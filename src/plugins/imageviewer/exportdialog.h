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

#ifndef EXPORTDIALOG_H
#define EXPORTDIALOG_H

#include <QDialog>

QT_FORWARD_DECLARE_CLASS(QSpinBox)

namespace Utils { class PathChooser; }

namespace ImageViewer {
namespace Internal {

class ExportDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ExportDialog(QWidget *parent = nullptr);

    QSize exportSize() const;
    void setExportSize(const QSize &);

    QString exportFileName() const;
    void setExportFileName(const QString &);

    void accept() override;

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

} // namespace Internal
} // namespace ImageViewer

#endif // EXPORTDIALOG_H
