/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef SETTINGSSELECTORWIDGET_H
#define SETTINGSSELECTORWIDGET_H

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

private slots:
    void removeButtonClicked();
    void renameButtonClicked();
    void updateButtonState();

private:
    QLabel *m_label;
    QComboBox *m_configurationCombo;
    QPushButton *m_addButton;
    QPushButton *m_removeButton;
    QPushButton *m_renameButton;
};

} // namespace Utils

#endif // SETTINGSSELECTORWIDGET_H
