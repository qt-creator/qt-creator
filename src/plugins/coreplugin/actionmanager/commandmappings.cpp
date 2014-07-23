/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "commandmappings.h"
#include "commandsfile.h"

#include <coreplugin/dialogs/shortcutsettings.h>

#include <utils/hostosinfo.h>
#include <utils/headerviewstretcher.h>
#include <utils/fancylineedit.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPointer>
#include <QPushButton>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

Q_DECLARE_METATYPE(Core::Internal::ShortcutItem*)

using namespace Utils;

namespace Core {
namespace Internal {

class KeySequenceValidator : public FancyLineEdit
{
public:
    KeySequenceValidator(QWidget *parent, CommandMappings *mappings)
        : FancyLineEdit(parent), m_mappings(mappings)
    {}

    bool validate(const QString &, QString *) const
    {
        return !m_mappings->hasConflicts();
    }

    CommandMappings *m_mappings;
};

class CommandMappingsPrivate
{
public:
    CommandMappingsPrivate(CommandMappings *parent)
        : q(parent), m_widget(0)
    {}

    void setupWidget()
    {
        QTC_CHECK(m_widget == 0);
        m_widget = new QWidget;

        groupBox = new QGroupBox(m_widget);
        groupBox->setTitle(CommandMappings::tr("Command Mappings"));

        filterEdit = new FancyLineEdit(groupBox);
        filterEdit->setFiltering(true);

        commandList = new QTreeWidget(groupBox);
        commandList->setRootIsDecorated(false);
        commandList->setUniformRowHeights(true);
        commandList->setSortingEnabled(true);
        commandList->setColumnCount(3);

        QTreeWidgetItem *item = commandList->headerItem();
        item->setText(2, CommandMappings::tr("Target"));
        item->setText(1, CommandMappings::tr("Label"));
        item->setText(0, CommandMappings::tr("Command"));

        defaultButton = new QPushButton(CommandMappings::tr("Reset All"), groupBox);
        defaultButton->setToolTip(CommandMappings::tr("Reset all to default."));

        importButton = new QPushButton(CommandMappings::tr("Import..."), groupBox);
        exportButton = new QPushButton(CommandMappings::tr("Export..."), groupBox);

        targetEditGroup = new QGroupBox(CommandMappings::tr("Target Identifier"), m_widget);

        targetEdit = new KeySequenceValidator(targetEditGroup, q);
        targetEdit->setAutoHideButton(FancyLineEdit::Right, true);
        targetEdit->setPlaceholderText(QString());
        targetEdit->installEventFilter(q);
        targetEdit->setFiltering(true);

        resetButton = new QPushButton(targetEditGroup);
        resetButton->setToolTip(CommandMappings::tr("Reset to default."));
        resetButton->setText(CommandMappings::tr("Reset"));

        QLabel *infoLabel = new QLabel(targetEditGroup);
        infoLabel->setTextFormat(Qt::RichText);

        QHBoxLayout *hboxLayout1 = new QHBoxLayout();
        hboxLayout1->addWidget(defaultButton);
        hboxLayout1->addStretch();
        hboxLayout1->addWidget(importButton);
        hboxLayout1->addWidget(exportButton);

        QHBoxLayout *hboxLayout = new QHBoxLayout();
        hboxLayout->addWidget(filterEdit);

        QVBoxLayout *vboxLayout1 = new QVBoxLayout(groupBox);
        vboxLayout1->addLayout(hboxLayout);
        vboxLayout1->addWidget(commandList);
        vboxLayout1->addLayout(hboxLayout1);

        QHBoxLayout *hboxLayout2 = new QHBoxLayout();
        hboxLayout2->addWidget(new QLabel(CommandMappings::tr("Target:"), targetEditGroup));
        hboxLayout2->addWidget(targetEdit);
        hboxLayout2->addWidget(resetButton);

        QVBoxLayout *vboxLayout2 = new QVBoxLayout(targetEditGroup);
        vboxLayout2->addLayout(hboxLayout2);
        vboxLayout2->addWidget(infoLabel);

        QVBoxLayout *vboxLayout = new QVBoxLayout(m_widget);
        vboxLayout->addWidget(groupBox);
        vboxLayout->addWidget(targetEditGroup);

        q->connect(targetEdit, SIGNAL(buttonClicked(Utils::FancyLineEdit::Side)),
            SLOT(removeTargetIdentifier()));
        q->connect(resetButton, SIGNAL(clicked()),
            SLOT(resetTargetIdentifier()));
        q->connect(exportButton, SIGNAL(clicked()),
            SLOT(exportAction()));
        q->connect(importButton, SIGNAL(clicked()),
            SLOT(importAction()));
        q->connect(defaultButton, SIGNAL(clicked()),
            SLOT(defaultAction()));

        q->initialize();

        commandList->sortByColumn(0, Qt::AscendingOrder);

        q->connect(filterEdit, SIGNAL(textChanged(QString)),
            SLOT(filterChanged(QString)));
        q->connect(commandList, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
            SLOT(commandChanged(QTreeWidgetItem*)));
        q->connect(targetEdit, SIGNAL(textChanged(QString)),
            SLOT(targetIdentifierChanged()));

        new HeaderViewStretcher(commandList->header(), 1);

        q->commandChanged(0);
    }

    CommandMappings *q;
    QPointer<QWidget> m_widget;

    QGroupBox *groupBox;
    FancyLineEdit *filterEdit;
    QTreeWidget *commandList;
    QPushButton *defaultButton;
    QPushButton *importButton;
    QPushButton *exportButton;
    QGroupBox *targetEditGroup;
    FancyLineEdit *targetEdit;
    QPushButton *resetButton;
};

} // namespace Internal

CommandMappings::CommandMappings(QObject *parent)
    : IOptionsPage(parent), d(new Internal::CommandMappingsPrivate(this))
{
}

CommandMappings::~CommandMappings()
{
   delete d;
}

QWidget *CommandMappings::widget()
{
    if (!d->m_widget)
        d->setupWidget();
    return d->m_widget;
}

void CommandMappings::setImportExportEnabled(bool enabled)
{
    d->importButton->setVisible(enabled);
    d->exportButton->setVisible(enabled);
}

QTreeWidget *CommandMappings::commandList() const
{
    return d->commandList;
}

QLineEdit *CommandMappings::targetEdit() const
{
    return d->targetEdit;
}

void CommandMappings::setPageTitle(const QString &s)
{
    d->groupBox->setTitle(s);
}

void CommandMappings::setTargetLabelText(const QString &s)
{
    d->targetEdit->setText(s);
}

void CommandMappings::setTargetEditTitle(const QString &s)
{
    d->targetEditGroup->setTitle(s);
}

void CommandMappings::setTargetHeader(const QString &s)
{
    d->commandList->setHeaderLabels(QStringList() << tr("Command") << tr("Label") << s);
}

void CommandMappings::finish()
{
    delete d->m_widget;
}

void CommandMappings::commandChanged(QTreeWidgetItem *current)
{
    if (!current || !current->data(0, Qt::UserRole).isValid()) {
        d->targetEdit->clear();
        d->targetEditGroup->setEnabled(false);
        return;
    }
    d->targetEditGroup->setEnabled(true);
}

void CommandMappings::filterChanged(const QString &f)
{
    for (int i = 0; i < d->commandList->topLevelItemCount(); ++i) {
        QTreeWidgetItem *item = d->commandList->topLevelItem(i);
        filter(f, item);
    }
}

bool CommandMappings::hasConflicts() const
{
    return true;
}

bool CommandMappings::filter(const QString &filterString, QTreeWidgetItem *item)
{
    bool visible = filterString.isEmpty();
    int columnCount = item->columnCount();
    for (int i = 0; !visible && i < columnCount; ++i) {
        QString text = item->text(i);
        if (HostOsInfo::isMacHost()) {
            // accept e.g. Cmd+E in the filter. the text shows special fancy characters for Cmd
            if (i == columnCount - 1) {
                QKeySequence key = QKeySequence::fromString(text, QKeySequence::NativeText);
                if (!key.isEmpty()) {
                    text = key.toString(QKeySequence::PortableText);
                    text.replace(QLatin1String("Ctrl"), QLatin1String("Cmd"));
                    text.replace(QLatin1String("Meta"), QLatin1String("Ctrl"));
                    text.replace(QLatin1String("Alt"), QLatin1String("Opt"));
                }
            }
        }
        visible |= (bool)text.contains(filterString, Qt::CaseInsensitive);
    }

    int childCount = item->childCount();
    if (childCount > 0) {
        // force visibility if this item matches
        QString leafFilterString = visible ? QString() : filterString;
        for (int i = 0; i < childCount; ++i) {
            QTreeWidgetItem *citem = item->child(i);
            visible |= !filter(leafFilterString, citem); // parent visible if any child visible
        }
    }
    item->setHidden(!visible);
    return !visible;
}

void CommandMappings::setModified(QTreeWidgetItem *item , bool modified)
{
    QFont f = item->font(0);
    f.setItalic(modified);
    item->setFont(0, f);
    item->setFont(1, f);
    f.setBold(modified);
    item->setFont(2, f);
}

QString CommandMappings::filterText() const
{
    return d->filterEdit ? d->filterEdit->text() : QString();
}

} // namespace Core
