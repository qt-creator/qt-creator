// Copyright (C) 2016 Nicolas Arnaud-Cormos
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "macrooptionspage.h"

#include "macro.h"
#include "macromanager.h"
#include "macrosconstants.h"
#include "macrostr.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>

#include <texteditor/texteditorconstants.h>

#include <utils/layoutbuilder.h>

#include <QAction>
#include <QDir>
#include <QFileInfo>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMap>
#include <QPushButton>
#include <QStringList>
#include <QTreeWidget>
#include <QTreeWidgetItem>

namespace Macros::Internal {

const int NAME_ROLE = Qt::UserRole;
const int WRITE_ROLE = Qt::UserRole + 1;

class MacroOptionsWidget final : public Core::IOptionsPageWidget
{
public:
    MacroOptionsWidget();

    void initialize();

    void apply() final;

private:
    void remove();
    void changeCurrentItem(QTreeWidgetItem *current);

    void createTable();

    void changeDescription(const QString &description);

    QStringList m_macroToRemove;
    bool m_changingCurrent = false;

    QMap<QString, QString> m_macroToChange;

    QTreeWidget *m_treeWidget;
    QPushButton *m_removeButton;
    QGroupBox *m_macroGroup;
    QLineEdit *m_description;
};

MacroOptionsWidget::MacroOptionsWidget()
{
    m_treeWidget = new QTreeWidget;
    m_treeWidget->setTextElideMode(Qt::ElideLeft);
    m_treeWidget->setUniformRowHeights(true);
    m_treeWidget->setSortingEnabled(true);
    m_treeWidget->setColumnCount(3);
    m_treeWidget->header()->setSortIndicatorShown(true);
    m_treeWidget->header()->setStretchLastSection(true);
    m_treeWidget->header()->setSortIndicator(0, Qt::AscendingOrder);
    m_treeWidget->setHeaderLabels({Tr::tr("Name"), Tr::tr("Description"), Tr::tr("Shortcut")});

    m_description = new QLineEdit;

    m_removeButton = new QPushButton(Tr::tr("Remove"));

    m_macroGroup = new QGroupBox(Tr::tr("Macro"), this);

    using namespace Layouting;

    Row {
        Tr::tr("Description:"), m_description
    }.attachTo(m_macroGroup);

    Column {
        Group {
            title(Tr::tr("Preferences")),
            Row {
                m_treeWidget,
                Column { m_removeButton, st },
            }
        },
        m_macroGroup
    }.attachTo(this);

    connect(m_treeWidget, &QTreeWidget::currentItemChanged,
            this, &MacroOptionsWidget::changeCurrentItem);
    connect(m_removeButton, &QPushButton::clicked,
            this, &MacroOptionsWidget::remove);
    connect(m_description, &QLineEdit::textChanged,
            this, &MacroOptionsWidget::changeDescription);

    initialize();
}

void MacroOptionsWidget::initialize()
{
    m_macroToRemove.clear();
    m_macroToChange.clear();
    m_treeWidget->clear();
    changeCurrentItem(nullptr);

    // Create the treeview
    createTable();
}

void MacroOptionsWidget::createTable()
{
    QDir dir(MacroManager::macrosDirectory());
    const Utils::Id base = Utils::Id(Constants::PREFIX_MACRO);
    for (Macro *macro : MacroManager::macros()) {
        QFileInfo fileInfo(macro->fileName());
        if (fileInfo.absoluteDir() == dir.absolutePath()) {
            auto macroItem = new QTreeWidgetItem(m_treeWidget);
            macroItem->setText(0, macro->displayName());
            macroItem->setText(1, macro->description());
            macroItem->setData(0, NAME_ROLE, macro->displayName());
            macroItem->setData(0, WRITE_ROLE, macro->isWritable());

            Core::Command *command =
                    Core::ActionManager::command(base.withSuffix(macro->displayName()));
            if (command && command->action()) {
                macroItem->setText(2,
                                   command->action()->shortcut().toString(QKeySequence::NativeText));
            }
        }
    }
}

void MacroOptionsWidget::changeCurrentItem(QTreeWidgetItem *current)
{
    m_changingCurrent = true;
    m_removeButton->setEnabled(current);
    m_macroGroup->setEnabled(current);
    if (!current) {
        m_description->clear();
    } else {
        m_description->setText(current->text(1));
        m_description->setEnabled(current->data(0, WRITE_ROLE).toBool());
    }
    m_changingCurrent = false;
}

void MacroOptionsWidget::remove()
{
    QTreeWidgetItem *current = m_treeWidget->currentItem();
    m_macroToRemove.append(current->data(0, NAME_ROLE).toString());
    delete current;
}

void MacroOptionsWidget::apply()
{
    // Remove macro
    for (const QString &name : std::as_const(m_macroToRemove)) {
        MacroManager::instance()->deleteMacro(name);
        m_macroToChange.remove(name);
    }

    // Change macro
    for (auto it = m_macroToChange.cbegin(), end = m_macroToChange.cend(); it != end; ++it)
        MacroManager::instance()->changeMacro(it.key(), it.value());

    // Reinitialize the page
    initialize();
}

void MacroOptionsWidget::changeDescription(const QString &description)
{
    QTreeWidgetItem *current = m_treeWidget->currentItem();
    if (m_changingCurrent || !current)
        return;

    QString macroName = current->data(0, NAME_ROLE).toString();
    m_macroToChange[macroName] = description;
    current->setText(1, description);
    QFont font = current->font(1);
    font.setItalic(true);
    current->setFont(1, font);
}

MacroOptionsPage::MacroOptionsPage()
{
    setId(Constants::M_OPTIONS_PAGE);
    setDisplayName(Tr::tr("Macros"));
    setCategory(TextEditor::Constants::TEXT_EDITOR_SETTINGS_CATEGORY);
    setWidgetCreator([] { return new MacroOptionsWidget; });
}

} // Macros::Internal
