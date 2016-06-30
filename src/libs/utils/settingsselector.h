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

#include "utils_global.h"

#include <QWidget>

QT_BEGIN_NAMESPACE
class QAbstractItemModel;
class QComboBox;
class QLabel;
class QMenu;
class QPushButton;
class QString;
class QVariant;
QT_END_NAMESPACE

namespace Utils {

class QTCREATOR_UTILS_EXPORT SettingsSelector : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsSelector(QWidget *parent = 0);
    ~SettingsSelector();

    void setConfigurationModel(QAbstractItemModel *model);
    QAbstractItemModel *configurationModel() const;

    void setLabelText(const QString &text);
    QString labelText() const;
    void setCurrentIndex(int index);

    void setAddMenu(QMenu *);
    QMenu *addMenu() const;

    int currentIndex() const;

signals:
    void add();
    void remove(int index);
    void rename(int index, const QString &newName);
    void currentChanged(int index);

private:
    void removeButtonClicked();
    void renameButtonClicked();
    void updateButtonState();

    QLabel *m_label;
    QComboBox *m_configurationCombo;
    QPushButton *m_addButton;
    QPushButton *m_removeButton;
    QPushButton *m_renameButton;
};

} // namespace Utils
