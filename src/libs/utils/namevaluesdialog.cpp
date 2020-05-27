/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "namevaluesdialog.h"

#include <utils/environment.h>
#include <utils/hostosinfo.h>

#include <QDialogButtonBox>
#include <QLabel>
#include <QPlainTextEdit>
#include <QSet>
#include <QVBoxLayout>

namespace Utils {

namespace Internal {

static EnvironmentItems cleanUp(const EnvironmentItems &items)
{
    EnvironmentItems uniqueItems;
    QSet<QString> uniqueSet;
    for (int i = items.count() - 1; i >= 0; i--) {
        EnvironmentItem item = items.at(i);
        if (HostOsInfo::isWindowsHost())
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

NameValueItemsWidget::NameValueItemsWidget(QWidget *parent)
    : QWidget(parent)
{
    m_editor = new QPlainTextEdit(this);
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_editor);
}

void NameValueItemsWidget::setEnvironmentItems(const EnvironmentItems &items)
{
    EnvironmentItems sortedItems = items;
    EnvironmentItem::sort(&sortedItems);
    const QStringList list = EnvironmentItem::toStringList(sortedItems);
    m_editor->document()->setPlainText(list.join(QLatin1Char('\n')));
}

EnvironmentItems NameValueItemsWidget::environmentItems() const
{
    const QStringList list = m_editor->document()->toPlainText().split(QLatin1String("\n"));
    EnvironmentItems items = EnvironmentItem::fromStringList(list);
    return cleanUp(items);
}

void NameValueItemsWidget::setPlaceholderText(const QString &text)
{
    m_editor->setPlaceholderText(text);
}
} // namespace Internal

NameValuesDialog::NameValuesDialog(const QString &windowTitle, const QString &helpText, QWidget *parent)
    : QDialog(parent)
{
    resize(640, 480);
    m_editor = new Internal::NameValueItemsWidget(this);
    auto box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                    Qt::Horizontal,
                                    this);
    connect(box, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(box, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto helpLabel = new QLabel(this);
    helpLabel->setText(helpText);

    auto layout = new QVBoxLayout(this);
    layout->addWidget(m_editor);
    layout->addWidget(helpLabel);

    layout->addWidget(box);

    setWindowTitle(windowTitle);
}

void NameValuesDialog::setNameValueItems(const EnvironmentItems &items)
{
    m_editor->setEnvironmentItems(items);
}

EnvironmentItems NameValuesDialog::nameValueItems() const
{
    return m_editor->environmentItems();
}

void NameValuesDialog::setPlaceholderText(const QString &text)
{
    m_editor->setPlaceholderText(text);
}

Utils::optional<NameValueItems> NameValuesDialog::getNameValueItems(QWidget *parent,
                                                                    const NameValueItems &initial,
                                                                    const QString &placeholderText,
                                                                    Polisher polisher,
                                                                    const QString &windowTitle,
                                                                    const QString &helpText)
{
    NameValuesDialog dialog(windowTitle, helpText, parent);
    if (polisher)
        polisher(&dialog);
    dialog.setNameValueItems(initial);
    dialog.setPlaceholderText(placeholderText);
    bool result = dialog.exec() == QDialog::Accepted;
    if (result)
        return dialog.nameValueItems();

    return {};
}

} // namespace Utils
