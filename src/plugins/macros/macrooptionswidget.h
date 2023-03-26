// Copyright (C) 2016 Nicolas Arnaud-Cormos
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

#include <QStringList>
#include <QMap>

QT_BEGIN_NAMESPACE
class QGroupBox;
class QLineEdit;
class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;
QT_END_NAMESPACE

namespace Macros {
namespace Internal {

class MacroOptionsWidget final : public Core::IOptionsPageWidget
{
    Q_OBJECT

public:
    MacroOptionsWidget();
    ~MacroOptionsWidget() final;

    void initialize();

    void apply() final;

private:
    void remove();
    void changeCurrentItem(QTreeWidgetItem *current);

    void createTable();

    void changeDescription(const QString &description);

private:
    QStringList m_macroToRemove;
    bool m_changingCurrent = false;

    QMap<QString, QString> m_macroToChange;

    QTreeWidget *m_treeWidget;
    QPushButton *m_removeButton;
    QGroupBox *m_macroGroup;
    QLineEdit *m_description;
};

} // namespace Internal
} // namespace Macros
