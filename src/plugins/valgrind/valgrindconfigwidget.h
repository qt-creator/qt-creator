/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Author: Milian Wolff, KDAB (milian.wolff@kdab.com)
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

#include <QWidget>

QT_BEGIN_NAMESPACE
class QStandardItemModel;
QT_END_NAMESPACE

namespace Valgrind {
namespace Internal {

namespace Ui { class ValgrindConfigWidget; }

class ValgrindBaseSettings;

class ValgrindConfigWidget : public QWidget
{
    Q_OBJECT

public:
    ValgrindConfigWidget(ValgrindBaseSettings *settings, QWidget *parent, bool global);
    virtual ~ValgrindConfigWidget();

    void setSuppressions(const QStringList &files);
    QStringList suppressions() const;

public Q_SLOTS:
    void slotAddSuppression();
    void slotRemoveSuppression();
    void slotSuppressionsRemoved(const QStringList &files);
    void slotSuppressionsAdded(const QStringList &files);
    void slotSuppressionSelectionChanged();

private slots:
    void updateUi();

private:
    ValgrindBaseSettings *m_settings;
    Ui::ValgrindConfigWidget *m_ui;
    QStandardItemModel *m_model;
};

} // namespace Internal
} // namespace Valgrind
