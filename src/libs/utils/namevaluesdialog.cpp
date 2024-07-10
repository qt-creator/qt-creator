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
#include <QTextBlock>
#include <QTimer>
#include <QVBoxLayout>

namespace Utils {
namespace Internal {

static EnvironmentItems cleanUp(const EnvironmentItems &items)
{
    EnvironmentItems cleanedItems;
    for (int i = items.count() - 1; i >= 0; i--) {
        EnvironmentItem item = items.at(i);
        if (HostOsInfo::isWindowsHost())
            item.name = item.name.toUpper();
        const QString &itemName = item.name;
        QString emptyName = itemName;
        emptyName.remove(QLatin1Char(' '));
        if (!emptyName.isEmpty())
            cleanedItems.prepend(item);
    }
    return cleanedItems;
}

class TextEditHelper : public QPlainTextEdit
{
    Q_OBJECT
public:
    using QPlainTextEdit::QPlainTextEdit;

signals:
    void lostFocus();

private:
    void focusOutEvent(QFocusEvent *e) override
    {
        QPlainTextEdit::focusOutEvent(e);
        emit lostFocus();
    }
};

} // namespace Internal

NameValueItemsWidget::NameValueItemsWidget(QWidget *parent)
    : QWidget(parent)
{
    const QString helpText = Tr::tr(
        "Enter one environment variable per line.\n"
        "To set or change a variable, use VARIABLE=VALUE.\n"
        "To disable a variable, prefix this line with \"#\".\n"
        "To append to a variable, use VARIABLE+=VALUE.\n"
        "To prepend to a variable, use VARIABLE=+VALUE.\n"
        "Existing variables can be referenced in a VALUE with ${OTHER}.\n"
        "To clear a variable, put its name on a line with nothing else on it.\n"
        "Lines starting with \"##\" will be treated as comments.");

    m_editor = new Internal::TextEditHelper(this);
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_editor);
    layout->addWidget(new QLabel(helpText, this));

    const auto timer = new QTimer(this);
    timer->setSingleShot(true);
    timer->setInterval(1000);
    connect(m_editor, &QPlainTextEdit::textChanged, timer, qOverload<>(&QTimer::start));
    connect(timer, &QTimer::timeout, this, &NameValueItemsWidget::forceUpdateCheck);
    connect(m_editor, &Internal::TextEditHelper::lostFocus, this, [this, timer] {
        timer->stop();
        forceUpdateCheck();
    });
}

void NameValueItemsWidget::setEnvironmentItems(const EnvironmentItems &items)
{
    m_originalItems = items;
    m_editor->document()->setPlainText(EnvironmentItem::toStringList(items)
                                           .join(QLatin1Char('\n')));
}

EnvironmentItems NameValueItemsWidget::environmentItems() const
{
    const QStringList list = m_editor->document()->toPlainText().split(QLatin1String("\n"));
    return Internal::cleanUp(EnvironmentItem::fromStringList(list));
}

void NameValueItemsWidget::setPlaceholderText(const QString &text)
{
    m_editor->setPlaceholderText(text);
}

bool NameValueItemsWidget::editVariable(const QString &name, Selection selection)
{
    QTextDocument * const doc = m_editor->document();
    for (QTextBlock b = doc->lastBlock(); b.isValid(); b = b.previous()) {
        const QString &line = b.text();
        qsizetype offset = 0;
        const auto skipWhiteSpace = [&] {
            for (; offset < line.length(); ++offset) {
                if (!line.at(offset).isSpace())
                    return;
            }
        };
        skipWhiteSpace();
        if (line.mid(offset, name.size()) != name)
            continue;
        offset += name.size();

        const auto updateCursor = [&](int anchor, int pos) {
            QTextCursor newCursor(doc);
            newCursor.setPosition(anchor);
            newCursor.setPosition(pos, QTextCursor::KeepAnchor);
            m_editor->setTextCursor(newCursor);
        };

        if (selection == Selection::Name) {
            m_editor->setFocus();
            updateCursor(b.position() + offset, b.position());
            return true;
        }

        skipWhiteSpace();
        if (offset < line.length()) {
            QChar nextChar = line.at(offset);
            if (nextChar.isLetterOrNumber() || nextChar == '_')
                continue;
            if (nextChar == '=') {
                if (++offset < line.length() && line.at(offset) == '+')
                    ++offset;
            } else if (nextChar == '+') {
                if (++offset < line.length() && line.at(offset) == '=')
                    ++offset;
            }
        }
        m_editor->setFocus();
        updateCursor(b.position() + b.length() - 1, b.position() + offset);
        return true;
    }
    return false;
}

void NameValueItemsWidget::forceUpdateCheck()
{
    const EnvironmentItems newItems = environmentItems();
    if (newItems != m_originalItems) {
        m_originalItems = newItems;
        emit userChangedItems(newItems);
    }
}

NameValuesDialog::NameValuesDialog(const QString &windowTitle, QWidget *parent)
    : QDialog(parent)
{
    resize(640, 480);
    m_editor = new NameValueItemsWidget(this);
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

std::optional<EnvironmentItems> NameValuesDialog::getNameValueItems(QWidget *parent,
                                                                    const EnvironmentItems &initial,
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

#include <namevaluesdialog.moc>
