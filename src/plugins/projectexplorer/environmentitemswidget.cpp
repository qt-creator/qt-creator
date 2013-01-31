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

#include "environmentitemswidget.h"

#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <texteditor/snippets/snippeteditor.h>
#include <texteditor/texteditorsettings.h>

#include <QVBoxLayout>
#include <QDialogButtonBox>

namespace ProjectExplorer {

class EnvironmentItemsWidgetPrivate
{
public:
    QList<Utils::EnvironmentItem> cleanUp(
            const QList<Utils::EnvironmentItem> &items) const;
    TextEditor::BaseTextEditorWidget *m_editor;
};

QList<Utils::EnvironmentItem> EnvironmentItemsWidgetPrivate::cleanUp(
        const QList<Utils::EnvironmentItem> &items) const
{
    QList<Utils::EnvironmentItem> uniqueItems;
    QSet<QString> uniqueSet;
    for (int i = items.count() - 1; i >= 0; i--) {
        Utils::EnvironmentItem item = items.at(i);
        if (Utils::HostOsInfo::isWindowsHost())
            item.name = item.name.toUpper();
        const QString &itemName = item.name;
        QString emptyName = itemName;
        emptyName.remove(QLatin1Char(' '));
        if (!emptyName.isEmpty() && !uniqueSet.contains(itemName)) {
            uniqueItems.prepend(item);
            uniqueSet.insert(itemName);
        }
    }
    return uniqueItems;
}

EnvironmentItemsWidget::EnvironmentItemsWidget(QWidget *parent) :
    QWidget(parent), d(new EnvironmentItemsWidgetPrivate)
{
    d->m_editor = new TextEditor::SnippetEditorWidget(this);
    TextEditor::TextEditorSettings *settings = TextEditor::TextEditorSettings::instance();
    d->m_editor->setFontSettings(settings->fontSettings());
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(d->m_editor);
}

EnvironmentItemsWidget::~EnvironmentItemsWidget()
{
    delete d;
}

void EnvironmentItemsWidget::setEnvironmentItems(const QList<Utils::EnvironmentItem> &items)
{
    QList<Utils::EnvironmentItem> sortedItems = items;
    Utils::EnvironmentItem::sort(&sortedItems);
    QStringList list = Utils::EnvironmentItem::toStringList(sortedItems);
    d->m_editor->document()->setPlainText(list.join(QLatin1String("\n")));
}

QList<Utils::EnvironmentItem> EnvironmentItemsWidget::environmentItems() const
{
    const QStringList list = d->m_editor->document()->toPlainText().split(QLatin1String("\n"));
    QList<Utils::EnvironmentItem> items = Utils::EnvironmentItem::fromStringList(list);
    return d->cleanUp(items);
}



class EnvironmentItemsDialogPrivate
{
public:
    EnvironmentItemsWidget *m_editor;
};

EnvironmentItemsDialog::EnvironmentItemsDialog(QWidget *parent) :
    QDialog(parent), d(new EnvironmentItemsDialogPrivate)
{
    resize(640, 480);
    d->m_editor = new EnvironmentItemsWidget(this);
    QDialogButtonBox *box = new QDialogButtonBox(
            QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
    connect(box, SIGNAL(accepted()), this, SLOT(accept()));
    connect(box, SIGNAL(rejected()), this, SLOT(reject()));
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(d->m_editor);
    layout->addWidget(box);
    setWindowTitle(tr("Edit Environment"));
}

EnvironmentItemsDialog::~EnvironmentItemsDialog()
{
    delete d;
}

void EnvironmentItemsDialog::setEnvironmentItems(const QList<Utils::EnvironmentItem> &items)
{
    d->m_editor->setEnvironmentItems(items);
}

QList<Utils::EnvironmentItem> EnvironmentItemsDialog::environmentItems() const
{
    return d->m_editor->environmentItems();
}

QList<Utils::EnvironmentItem> EnvironmentItemsDialog::getEnvironmentItems(QWidget *parent,
                const QList<Utils::EnvironmentItem> &initial, bool *ok)
{
    EnvironmentItemsDialog dlg(parent);
    dlg.setEnvironmentItems(initial);
    bool result = dlg.exec() == QDialog::Accepted;
    if (ok)
        *ok = result;
    if (result)
        return dlg.environmentItems();
    return QList<Utils::EnvironmentItem>();
}

} // namespace ProjectExplorer
