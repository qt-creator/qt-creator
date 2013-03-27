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

#include "commandmappings.h"
#include "shortcutsettings.h"
#include "ui_commandmappings.h"
#include "commandsfile.h"

#include <utils/hostosinfo.h>
#include <utils/headerviewstretcher.h>

#include <QTreeWidgetItem>
#include <QDebug>

Q_DECLARE_METATYPE(Core::Internal::ShortcutItem*)

using namespace Core;
using namespace Core::Internal;

CommandMappings::CommandMappings(QObject *parent)
    : IOptionsPage(parent), m_page(0)
{
}

// IOptionsPage

QWidget *CommandMappings::createPage(QWidget *parent)
{
    m_page = new Ui::CommandMappings();
    QWidget *w = new QWidget(parent);
    m_page->setupUi(w);
    m_page->targetEdit->setAutoHideButton(Utils::FancyLineEdit::Right, true);
    m_page->targetEdit->setPlaceholderText(QString());
    m_page->targetEdit->installEventFilter(this);

    connect(m_page->targetEdit, SIGNAL(buttonClicked(Utils::FancyLineEdit::Side)),
        this, SLOT(removeTargetIdentifier()));
    connect(m_page->resetButton, SIGNAL(clicked()),
        this, SLOT(resetTargetIdentifier()));
    connect(m_page->exportButton, SIGNAL(clicked()),
        this, SLOT(exportAction()));
    connect(m_page->importButton, SIGNAL(clicked()),
        this, SLOT(importAction()));
    connect(m_page->defaultButton, SIGNAL(clicked()),
        this, SLOT(defaultAction()));

    initialize();

    m_page->commandList->sortByColumn(0, Qt::AscendingOrder);

    connect(m_page->filterEdit, SIGNAL(textChanged(QString)),
        this, SLOT(filterChanged(QString)));
    connect(m_page->commandList, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
        this, SLOT(commandChanged(QTreeWidgetItem*)));
    connect(m_page->targetEdit, SIGNAL(textChanged(QString)),
        this, SLOT(targetIdentifierChanged()));

    new Utils::HeaderViewStretcher(m_page->commandList->header(), 1);

    commandChanged(0);

    return w;
}

void CommandMappings::setImportExportEnabled(bool enabled)
{
    m_page->importButton->setVisible(enabled);
    m_page->exportButton->setVisible(enabled);
}

QTreeWidget *CommandMappings::commandList() const
{
    return m_page->commandList;
}

QLineEdit *CommandMappings::targetEdit() const
{
    return m_page->targetEdit;
}

void CommandMappings::setPageTitle(const QString &s)
{
    m_page->groupBox->setTitle(s);
}

void CommandMappings::setTargetLabelText(const QString &s)
{
    m_page->targetEditLabel->setText(s);
}

void CommandMappings::setTargetEditTitle(const QString &s)
{
    m_page->targetEditGroup->setTitle(s);
}

void CommandMappings::setTargetHeader(const QString &s)
{
    m_page->commandList->setHeaderLabels(QStringList() << tr("Command") << tr("Label") << s);
}

void CommandMappings::finish()
{
    if (!m_page) // page was never shown
        return;
    delete m_page;
    m_page = 0;
}

void CommandMappings::commandChanged(QTreeWidgetItem *current)
{
    if (!current || !current->data(0, Qt::UserRole).isValid()) {
        m_page->targetEdit->setText(QString());
        m_page->targetEditGroup->setEnabled(false);
        return;
    }
    m_page->targetEditGroup->setEnabled(true);
}

void CommandMappings::filterChanged(const QString &f)
{
    if (!m_page)
        return;
    for (int i=0; i<m_page->commandList->topLevelItemCount(); ++i) {
        QTreeWidgetItem *item = m_page->commandList->topLevelItem(i);
        filter(f, item);
    }
}

bool CommandMappings::filter(const QString &filterString, QTreeWidgetItem *item)
{
    bool visible = filterString.isEmpty();
    int columnCount = item->columnCount();
    for (int i = 0; !visible && i < columnCount; ++i) {
        QString text = item->text(i);
        if (Utils::HostOsInfo::isMacHost()) {
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
    if (!m_page)
        return QString();
    return m_page->filterEdit->text();
}
