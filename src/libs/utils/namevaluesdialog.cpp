// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "namevaluesdialog.h"

#include "algorithm.h"
#include "hostosinfo.h"
#include "utilstr.h"

#include <QDialogButtonBox>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
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
        if (!emptyName.isEmpty() && Utils::insert(uniqueSet, itemName))
            uniqueItems.prepend(item);
    }
    return uniqueItems;
}

class NameValueItemsWidget : public QWidget
{
public:
    explicit NameValueItemsWidget(QWidget *parent = nullptr);

    void setEnvironmentItems(const EnvironmentItems &items);
    EnvironmentItems environmentItems() const;

    void setPlaceholderText(const QString &text);

private:
    QPlainTextEdit *m_editor;
};

NameValueItemsWidget::NameValueItemsWidget(QWidget *parent)
    : QWidget(parent)
{
    const QString helpText = Tr::tr(
        "Enter one environment variable per line.\n"
        "To set or change a variable, use VARIABLE=VALUE.\n"
        "To append to a variable, use VARIABLE+=VALUE.\n"
        "To prepend to a variable, use VARIABLE=+VALUE.\n"
        "Existing variables can be referenced in a VALUE with ${OTHER}.\n"
        "To clear a variable, put its name on a line with nothing else on it.\n"
        "To disable a variable, prefix the line with \"#\".");

    m_editor = new QPlainTextEdit(this);
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_editor);
    layout->addWidget(new QLabel(helpText, this));
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

NameValuesDialog::NameValuesDialog(const QString &windowTitle, QWidget *parent)
    : QDialog(parent)
{
    resize(640, 480);
    m_editor = new Internal::NameValueItemsWidget(this);
    auto box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                    Qt::Horizontal,
                                    this);
    box->button(QDialogButtonBox::Ok)->setText(Tr::tr("&OK"));
    box->button(QDialogButtonBox::Cancel)->setText(Tr::tr("&Cancel"));
    connect(box, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(box, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto layout = new QVBoxLayout(this);
    layout->addWidget(m_editor);
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

std::optional<NameValueItems> NameValuesDialog::getNameValueItems(QWidget *parent,
                                                                    const NameValueItems &initial,
                                                                    const QString &placeholderText,
                                                                    Polisher polisher,
                                                                    const QString &windowTitle)
{
    NameValuesDialog dialog(windowTitle, parent);
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
