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

#pragma once

#include "targetselector.h"

QT_BEGIN_NAMESPACE
class QMenu;
class QPushButton;
QT_END_NAMESPACE

namespace ProjectExplorer {
class Target;
class Kit;
namespace Internal {

namespace Ui { class TargetSettingsWidget; }

class TargetSettingsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TargetSettingsWidget(QWidget *parent = 0);
    ~TargetSettingsWidget() override;

    void setCentralWidget(QWidget *widget);

    QString targetNameAt(int index) const;
    int targetCount() const;
    int currentIndex() const;
    int currentSubIndex() const;

public:
    void insertTarget(int index, int subIndex, const QString &name);
    void renameTarget(int index, const QString &name);
    void removeTarget(int index);
    void setCurrentIndex(int index);
    void setCurrentSubIndex(int index);
    void setAddButtonEnabled(bool enabled);
    void setAddButtonMenu(QMenu *menu);
    void setTargetMenu(QMenu *menu);

signals:
    void currentChanged(int targetIndex, int subIndex);
    void manageButtonClicked();
    void duplicateButtonClicked();
    void changeKitButtonClicked();
    void toolTipRequested(const QPoint &globalPosition, int targetIndex);
    void menuShown(int targetIndex);

private:
    Ui::TargetSettingsWidget *ui;

    TargetSelector *m_targetSelector;
    QPushButton *m_addButton;
    QPushButton *m_manageButton;
    QWidget *m_centralWidget = 0;
};

} // namespace Internal
} // namespace ProjectExplorer
