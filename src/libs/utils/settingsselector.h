/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef SETTINGSSELECTORWIDGET_H
#define SETTINGSSELECTORWIDGET_H

#include "utils_global.h"

#include <QtGui/QWidget>

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
