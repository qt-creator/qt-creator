/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "environmentdialog.h"

#include <utils/environment.h>
#include <utils/hostosinfo.h>

#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QPlainTextEdit>
#include <QLabel>

namespace Utils {

namespace Internal {

static QList<EnvironmentItem> cleanUp(
        const QList<EnvironmentItem> &items)
{
    QList<EnvironmentItem> uniqueItems;
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

class EnvironmentItemsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit EnvironmentItemsWidget(QWidget *parent = nullptr);

    void setEnvironmentItems(const QList<EnvironmentItem> &items);
    QList<EnvironmentItem> environmentItems() const;

    void setPlaceholderText(const QString &text);

private:
    QPlainTextEdit *m_editor;
};

EnvironmentItemsWidget::EnvironmentItemsWidget(QWidget *parent) :
    QWidget(parent)
{
    m_editor = new QPlainTextEdit(this);
    auto layout = new QVBoxLayout(this);
    layout->setMargin(0);
    layout->addWidget(m_editor);
}

void EnvironmentItemsWidget::setEnvironmentItems(const QList<EnvironmentItem> &items)
{
    QList<EnvironmentItem> sortedItems = items;
    EnvironmentItem::sort(&sortedItems);
    const QStringList list = EnvironmentItem::toStringList(sortedItems);
    m_editor->document()->setPlainText(list.join(QLatin1Char('\n')));
}

QList<EnvironmentItem> EnvironmentItemsWidget::environmentItems() const
{
    const QStringList list = m_editor->document()->toPlainText().split(QLatin1String("\n"));
    QList<EnvironmentItem> items = EnvironmentItem::fromStringList(list);
    return cleanUp(items);
}

void EnvironmentItemsWidget::setPlaceholderText(const QString &text)
{
    m_editor->setPlaceholderText(text);
}

class EnvironmentDialogPrivate
{
public:
    EnvironmentItemsWidget *m_editor;
};

} // namespace Internal

EnvironmentDialog::EnvironmentDialog(QWidget *parent) :
    QDialog(parent), d(new Internal::EnvironmentDialogPrivate)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    resize(640, 480);
    d->m_editor = new Internal::EnvironmentItemsWidget(this);
    d->m_editor->setToolTip(tr("Enter one variable per line with the variable name "
                               "separated from the variable value by \"=\".<br>"
                               "Environment variables can be referenced with ${OTHER}."));
    auto box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
    connect(box, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(box, &QDialogButtonBox::rejected, this, &QDialog::reject);
    auto layout = new QVBoxLayout(this);
    QLabel *label = new QLabel(this);
    label->setText(tr("Change environment by assigning one environment variable per line:"));
    layout->addWidget(label);
    layout->addWidget(d->m_editor);
    layout->addWidget(box);
    setWindowTitle(tr("Edit Environment"));
}

EnvironmentDialog::~EnvironmentDialog()
{
    delete d;
}

void EnvironmentDialog::setEnvironmentItems(const QList<EnvironmentItem> &items)
{
    d->m_editor->setEnvironmentItems(items);
}

QList<EnvironmentItem> EnvironmentDialog::environmentItems() const
{
    return d->m_editor->environmentItems();
}

void EnvironmentDialog::setPlaceholderText(const QString &text)
{
    d->m_editor->setPlaceholderText(text);
}

QList<EnvironmentItem> EnvironmentDialog::getEnvironmentItems(bool *ok,
                QWidget *parent,
                const QList<EnvironmentItem> &initial,
                const QString &placeholderText)
{
    EnvironmentDialog dlg(parent);
    dlg.setEnvironmentItems(initial);
    dlg.setPlaceholderText(placeholderText);
    bool result = dlg.exec() == QDialog::Accepted;
    if (ok)
        *ok = result;
    if (result)
        return dlg.environmentItems();
    return QList<EnvironmentItem>();
}

} // namespace Utils

#include "environmentdialog.moc"
