// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    explicit SettingsSelector(QWidget *parent = nullptr);
    ~SettingsSelector() override;

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
