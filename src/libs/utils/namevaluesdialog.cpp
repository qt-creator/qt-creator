// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "namevaluesdialog.h"

#include "guiutils.h"
#include "hostosinfo.h"
#include "pathchooser.h"
#include "utilstr.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QHBoxLayout>
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
    , m_scriptCheckBox(new QCheckBox(this))
    , m_scriptChooser(new PathChooser(this))
{
    const QString fileHelpText = Tr::tr(
        "The file can be a simple text file containing key/value pairs, or a script\n"
        "to be evaluated by the system shell.");

    const QString helpText = Tr::tr(
        "Enter one environment variable per line.\n"
        "To set or change a variable, use VARIABLE=VALUE.\n"
        "To disable a variable, prefix this line with \"#\".\n"
        "To append to a variable, use VARIABLE+=VALUE.\n"
        "To prepend to a variable, use VARIABLE=+VALUE.\n"
        "Existing variables can be referenced in a VALUE with ${OTHER}.\n"
        "To clear a variable, put its name on a line with nothing else on it.\n"
        "Lines starting with \"##\" will be treated as comments.\n").append(fileHelpText);

    m_scriptCheckBox->setText(Tr::tr("Get variables from text file or shell script"));
    m_scriptCheckBox->setToolTip(fileHelpText);
    connect(m_scriptCheckBox, &QCheckBox::toggled, m_scriptChooser, &PathChooser::setEnabled);

    m_scriptChooser->setExpectedKind(PathChooser::File);
    m_scriptChooser->setEnabled(false);

    const auto scriptLayout = new QHBoxLayout;
    scriptLayout->addWidget(m_scriptCheckBox);
    scriptLayout->addWidget(m_scriptChooser);
    m_editor = new Internal::TextEditHelper(this);
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addLayout(scriptLayout);
    layout->addWidget(m_editor);
    layout->addWidget(new QLabel(helpText, this));

    const auto timer = new QTimer(this);
    timer->setSingleShot(true);
    timer->setInterval(1000);
    connect(m_editor, &QPlainTextEdit::textChanged, timer, qOverload<>(&QTimer::start));
    connect(m_scriptChooser, &PathChooser::rawPathChanged, this, [this] { forceUpdateCheck(); });
    connect(m_scriptCheckBox, &QCheckBox::toggled, this, [this] { forceUpdateCheck(); });
    connect(timer, &QTimer::timeout, this, &NameValueItemsWidget::forceUpdateCheck);
    connect(m_editor, &Internal::TextEditHelper::lostFocus, this, [this, timer] {
        timer->stop();
        forceUpdateCheck();
    });
}

void NameValueItemsWidget::setEnvironmentChanges(const EnvironmentChanges &envFromUser)
{
    m_originalEnvChanges = envFromUser;
    m_scriptChooser->setFilePath(envFromUser.file());
    m_scriptCheckBox->setChecked(!envFromUser.file().isEmpty());
    m_editor->document()->setPlainText(
        EnvironmentItem::toStringList(envFromUser.itemsFromUser()).join(QLatin1Char('\n')));
}

EnvironmentChanges NameValueItemsWidget::envChanges() const
{
    const QStringList list = m_editor->document()->toPlainText().split(QLatin1String("\n"));
    EnvironmentChanges changes;
    changes.setItemsFromUser(Internal::cleanUp(EnvironmentItem::fromStringList(list)));
    if (m_scriptCheckBox->isChecked())
        changes.setFile(m_scriptChooser->filePath());
    return changes;
}

void NameValueItemsWidget::setPlaceholderText(const QString &text)
{
    m_editor->setPlaceholderText(text);
}

void NameValueItemsWidget::setBrowseHint(const FilePath &hint)
{
    m_scriptChooser->setInitialBrowsePathBackup(hint);
}

bool NameValueItemsWidget::editVariable(const QString &name, Selection selection)
{
    QTextDocument * const doc = m_editor->document();
    for (QTextBlock b = doc->lastBlock(); b.isValid(); b = b.previous()) {
        const QString &line = b.text();
        qsizetype offset = 0;
        const auto skipWhiteSpace = [&] {
            for (; offset < line.size(); ++offset) {
                if (!line.at(offset).isSpace())
                    return;
            }
        };
        skipWhiteSpace();
        if (offset < line.size() && line.at(offset) == '#') {
            ++offset;
            skipWhiteSpace();
        }
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
        if (offset < line.size()) {
            QChar nextChar = line.at(offset);
            if (nextChar.isLetterOrNumber() || nextChar == '_')
                continue;
            if (nextChar == '=') {
                if (++offset < line.size() && line.at(offset) == '+')
                    ++offset;
            } else if (nextChar == '+') {
                if (++offset < line.size() && line.at(offset) == '=')
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
    const EnvironmentChanges newChanges = envChanges();
    if (newChanges != m_originalEnvChanges) {
        m_originalEnvChanges = newChanges;
        emit userChangedItems(newChanges);
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

void NameValuesDialog::setEnvChanges(const EnvironmentChanges &items)
{
    m_editor->setEnvironmentChanges(items);
}

EnvironmentChanges NameValuesDialog::envChanges() const
{
    return m_editor->envChanges();
}

void NameValuesDialog::setPlaceholderText(const QString &text)
{
    m_editor->setPlaceholderText(text);
}

std::optional<EnvironmentChanges> NameValuesDialog::getNameValueItems(
    QWidget *parent,
    const EnvironmentChanges &initial,
    const QString &placeholderText,
    Polisher polisher,
    const QString &windowTitle,
    const FilePath &browseHint)
{
    NameValuesDialog dialog(windowTitle, parent);
    if (polisher)
        polisher(&dialog);
    dialog.setEnvChanges(initial);
    dialog.setPlaceholderText(placeholderText);
    dialog.setBrowseHint(browseHint);
    bool result = dialog.exec() == QDialog::Accepted;
    if (result)
        return dialog.envChanges();
    return {};
}

void NameValueItemsWidget::setupDirtyHooks()
{
    QObject::connect(m_editor, &QPlainTextEdit::textChanged, []() { markSettingsDirty(); });
}

} // namespace Utils

#include <namevaluesdialog.moc>
