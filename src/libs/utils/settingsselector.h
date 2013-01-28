/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
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
