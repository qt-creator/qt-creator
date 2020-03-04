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

#include "shortcutsettings.h"
#include <coreplugin/coreconstants.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/id.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/actionmanager/command_p.h>
#include <coreplugin/actionmanager/commandsfile.h>

#include <utils/algorithm.h>
#include <utils/fancylineedit.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>
#include <utils/theme/theme.h>

#include <QKeyEvent>
#include <QFileDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QTreeWidgetItem>
#include <QApplication>
#include <QDebug>

Q_DECLARE_METATYPE(Core::Internal::ShortcutItem*)

const char kSeparator[] = " | ";

static int translateModifiers(Qt::KeyboardModifiers state,
                                         const QString &text)
{
    int result = 0;
    // The shift modifier only counts when it is not used to type a symbol
    // that is only reachable using the shift key anyway
    if ((state & Qt::ShiftModifier) && (text.isEmpty()
                                        || !text.at(0).isPrint()
                                        || text.at(0).isLetterOrNumber()
                                        || text.at(0).isSpace()))
        result |= Qt::SHIFT;
    if (state & Qt::ControlModifier)
        result |= Qt::CTRL;
    if (state & Qt::MetaModifier)
        result |= Qt::META;
    if (state & Qt::AltModifier)
        result |= Qt::ALT;
    return result;
}

static QString keySequenceToEditString(const QKeySequence &sequence)
{
    QString text = sequence.toString(QKeySequence::PortableText);
    if (Utils::HostOsInfo::isMacHost()) {
        // adapt the modifier names
        text.replace(QLatin1String("Ctrl"), QLatin1String("Cmd"), Qt::CaseInsensitive);
        text.replace(QLatin1String("Alt"), QLatin1String("Opt"), Qt::CaseInsensitive);
        text.replace(QLatin1String("Meta"), QLatin1String("Ctrl"), Qt::CaseInsensitive);
    }
    return text;
}

static QString keySequencesToEditString(const QList<QKeySequence> &sequence)
{
    return Utils::transform(sequence, keySequenceToEditString).join(kSeparator);
}

static QString keySequencesToNativeString(const QList<QKeySequence> &sequence)
{
    return Utils::transform(sequence,
                            [](const QKeySequence &k) {
                                return k.toString(QKeySequence::NativeText);
                            })
        .join(kSeparator);
}

static QKeySequence keySequenceFromEditString(const QString &editString)
{
    QString text = editString.trimmed();
    if (Utils::HostOsInfo::isMacHost()) {
        // adapt the modifier names
        text.replace(QLatin1String("Opt"), QLatin1String("Alt"), Qt::CaseInsensitive);
        text.replace(QLatin1String("Ctrl"), QLatin1String("Meta"), Qt::CaseInsensitive);
        text.replace(QLatin1String("Cmd"), QLatin1String("Ctrl"), Qt::CaseInsensitive);
    }
    return QKeySequence::fromString(text, QKeySequence::PortableText);
}

struct ParsedKey
{
    QString text;
    QKeySequence key;
};

static QList<ParsedKey> keySequencesFromEditString(const QString &editString)
{
    if (editString.trimmed().isEmpty())
        return {};
    return Utils::transform(editString.split(kSeparator), [](const QString &str) {
        return ParsedKey{str, keySequenceFromEditString(str)};
    });
}

static bool keySequenceIsValid(const QKeySequence &sequence)
{
    if (sequence.isEmpty())
        return false;
    for (int i = 0; i < sequence.count(); ++i) {
        if (sequence[i] == Qt::Key_unknown)
            return false;
    }
    return true;
}

static bool textKeySequence(const QKeySequence &sequence)
{
    if (sequence.isEmpty())
        return false;
    for (int i = 0; i < sequence.count(); ++i) {
        int key = sequence[i];
        key &= ~(Qt::ShiftModifier | Qt::KeypadModifier);
        if (key < Qt::Key_Escape)
            return true;
    }
    return false;
}

namespace Core {
namespace Internal {

ShortcutButton::ShortcutButton(QWidget *parent)
    : QPushButton(parent)
    , m_key({{0, 0, 0, 0}})
{
    // Using ShortcutButton::tr() as workaround for QTBUG-34128
    setToolTip(ShortcutButton::tr("Click and type the new key sequence."));
    setCheckable(true);
    m_checkedText = ShortcutButton::tr("Stop Recording");
    m_uncheckedText = ShortcutButton::tr("Record");
    updateText();
    connect(this, &ShortcutButton::toggled, this, &ShortcutButton::handleToggleChange);
}

QSize ShortcutButton::sizeHint() const
{
    if (m_preferredWidth < 0) { // initialize size hint
        const QString originalText = text();
        auto that = const_cast<ShortcutButton *>(this);
        that->setText(m_checkedText);
        m_preferredWidth = QPushButton::sizeHint().width();
        that->setText(m_uncheckedText);
        int otherWidth = QPushButton::sizeHint().width();
        if (otherWidth > m_preferredWidth)
            m_preferredWidth = otherWidth;
        that->setText(originalText);
    }
    return QSize(m_preferredWidth, QPushButton::sizeHint().height());
}

bool ShortcutButton::eventFilter(QObject *obj, QEvent *evt)
{
    if (evt->type() == QEvent::ShortcutOverride) {
        evt->accept();
        return true;
    }
    if (evt->type() == QEvent::KeyRelease
               || evt->type() == QEvent::Shortcut
               || evt->type() == QEvent::Close/*Escape tries to close dialog*/) {
        return true;
    }
    if (evt->type() == QEvent::MouseButtonPress && isChecked()) {
        setChecked(false);
        return true;
    }
    if (evt->type() == QEvent::KeyPress) {
        auto k = static_cast<QKeyEvent*>(evt);
        int nextKey = k->key();
        if (m_keyNum > 3
                || nextKey == Qt::Key_Control
                || nextKey == Qt::Key_Shift
                || nextKey == Qt::Key_Meta
                || nextKey == Qt::Key_Alt) {
             return false;
        }

        nextKey |= translateModifiers(k->modifiers(), k->text());
        switch (m_keyNum) {
        case 0:
            m_key[0] = nextKey;
            break;
        case 1:
            m_key[1] = nextKey;
            break;
        case 2:
            m_key[2] = nextKey;
            break;
        case 3:
            m_key[3] = nextKey;
            break;
        default:
            break;
        }
        m_keyNum++;
        k->accept();
        emit keySequenceChanged(QKeySequence(m_key[0], m_key[1], m_key[2], m_key[3]));
        if (m_keyNum > 3)
            setChecked(false);
        return true;
    }
    return QPushButton::eventFilter(obj, evt);
}

void ShortcutButton::updateText()
{
    setText(isChecked() ? m_checkedText : m_uncheckedText);
}

void ShortcutButton::handleToggleChange(bool toogleState)
{
    updateText();
    m_keyNum = m_key[0] = m_key[1] = m_key[2] = m_key[3] = 0;
    if (toogleState) {
        if (QApplication::focusWidget())
            QApplication::focusWidget()->clearFocus(); // funny things happen otherwise
        qApp->installEventFilter(this);
    } else {
        qApp->removeEventFilter(this);
    }
}

class ShortcutSettingsWidget final : public CommandMappings
{
    Q_DECLARE_TR_FUNCTIONS(Core::Internal::ShortcutSettings)

public:
    ShortcutSettingsWidget();
    ~ShortcutSettingsWidget() final;

    void apply();

private:
    void importAction() final;
    void exportAction() final;
    void defaultAction() final;
    bool filterColumn(const QString &filterString, QTreeWidgetItem *item, int column) const final;

    void initialize();
    void handleCurrentCommandChanged(QTreeWidgetItem *current);
    void resetToDefault();
    bool validateShortcutEdit() const;
    bool markCollisions(ShortcutItem *);
    void setKeySequences(const QList<QKeySequence> &keys);
    void showConflicts();
    void clear();

    QList<ShortcutItem *> m_scitems;
    QGroupBox *m_shortcutBox;
    Utils::FancyLineEdit *m_shortcutEdit;
    QLabel *m_warningLabel;
};

ShortcutSettingsWidget::ShortcutSettingsWidget()
{
    setPageTitle(tr("Keyboard Shortcuts"));
    setTargetHeader(tr("Shortcut"));

    connect(ActionManager::instance(), &ActionManager::commandListChanged,
            this, &ShortcutSettingsWidget::initialize);
    connect(this, &ShortcutSettingsWidget::currentCommandChanged,
            this, &ShortcutSettingsWidget::handleCurrentCommandChanged);

    m_shortcutBox = new QGroupBox(tr("Shortcut"), this);
    m_shortcutBox->setEnabled(false);
    auto vboxLayout = new QVBoxLayout(m_shortcutBox);
    m_shortcutBox->setLayout(vboxLayout);
    auto hboxLayout = new QHBoxLayout;
    vboxLayout->addLayout(hboxLayout);
    m_shortcutEdit = new Utils::FancyLineEdit(m_shortcutBox);
    m_shortcutEdit->setFiltering(true);
    m_shortcutEdit->setPlaceholderText(tr("Enter key sequence as text"));
    auto shortcutLabel = new QLabel(tr("Key sequence:"));
    shortcutLabel->setToolTip(Utils::HostOsInfo::isMacHost()
           ? QLatin1String("<html><body>")
             + tr("Use \"Cmd\", \"Opt\", \"Ctrl\", and \"Shift\" for modifier keys. "
                  "Use \"Escape\", \"Backspace\", \"Delete\", \"Insert\", \"Home\", and so on, for special keys. "
                  "Combine individual keys with \"+\", "
                  "and combine multiple shortcuts to a shortcut sequence with \",\". "
                  "For example, if the user must hold the Ctrl and Shift modifier keys "
                  "while pressing Escape, and then release and press A, "
                  "enter \"Ctrl+Shift+Escape,A\".")
             + QLatin1String("</body></html>")
           : QLatin1String("<html><body>")
             + tr("Use \"Ctrl\", \"Alt\", \"Meta\", and \"Shift\" for modifier keys. "
                  "Use \"Escape\", \"Backspace\", \"Delete\", \"Insert\", \"Home\", and so on, for special keys. "
                  "Combine individual keys with \"+\", "
                  "and combine multiple shortcuts to a shortcut sequence with \",\". "
                  "For example, if the user must hold the Ctrl and Shift modifier keys "
                  "while pressing Escape, and then release and press A, "
                  "enter \"Ctrl+Shift+Escape,A\".")
             + QLatin1String("</body></html>"));
    auto shortcutButton = new ShortcutButton(m_shortcutBox);
    connect(shortcutButton,
            &ShortcutButton::keySequenceChanged,
            this,
            [this](const QKeySequence &k) { setKeySequences({k}); });
    auto resetButton = new QPushButton(tr("Reset"), m_shortcutBox);
    resetButton->setToolTip(tr("Reset to default."));
    connect(resetButton, &QPushButton::clicked,
            this, &ShortcutSettingsWidget::resetToDefault);
    hboxLayout->addWidget(shortcutLabel);
    hboxLayout->addWidget(m_shortcutEdit);
    hboxLayout->addWidget(shortcutButton);
    hboxLayout->addWidget(resetButton);

    m_warningLabel = new QLabel(m_shortcutBox);
    m_warningLabel->setTextFormat(Qt::RichText);
    QPalette palette = m_warningLabel->palette();
    palette.setColor(QPalette::Active, QPalette::WindowText,
                     Utils::creatorTheme()->color(Utils::Theme::TextColorError));
    m_warningLabel->setPalette(palette);
    connect(m_warningLabel, &QLabel::linkActivated, this, &ShortcutSettingsWidget::showConflicts);
    vboxLayout->addWidget(m_warningLabel);

    layout()->addWidget(m_shortcutBox);

    initialize();

    m_shortcutEdit->setValidationFunction([this](Utils::FancyLineEdit *, QString *) {
        return validateShortcutEdit();
    });
}

ShortcutSettingsWidget::~ShortcutSettingsWidget()
{
    qDeleteAll(m_scitems);
}

ShortcutSettings::ShortcutSettings()
{
    setId(Constants::SETTINGS_ID_SHORTCUTS);
    setDisplayName(ShortcutSettingsWidget::tr("Keyboard"));
    setCategory(Constants::SETTINGS_CATEGORY_CORE);
}

QWidget *ShortcutSettings::widget()
{
    if (!m_widget)
        m_widget = new ShortcutSettingsWidget();
    return m_widget;
}

void ShortcutSettingsWidget::apply()
{
    foreach (ShortcutItem *item, m_scitems)
        item->m_cmd->setKeySequences(item->m_keys);
}

void ShortcutSettings::apply()
{
    QTC_ASSERT(m_widget, return);
    m_widget->apply();
}

void ShortcutSettings::finish()
{
    delete m_widget;
}

ShortcutItem *shortcutItem(QTreeWidgetItem *treeItem)
{
    if (!treeItem)
        return nullptr;
    return treeItem->data(0, Qt::UserRole).value<ShortcutItem *>();
}

void ShortcutSettingsWidget::handleCurrentCommandChanged(QTreeWidgetItem *current)
{
    ShortcutItem *scitem = shortcutItem(current);
    if (!scitem) {
        m_shortcutEdit->clear();
        m_warningLabel->clear();
        m_shortcutBox->setEnabled(false);
    } else {
        setKeySequences(scitem->m_keys);
        markCollisions(scitem);
        m_shortcutBox->setEnabled(true);
    }
}

static bool checkValidity(const QList<ParsedKey> &keys, QString *warningMessage)
{
    QTC_ASSERT(warningMessage, return true);
    for (const ParsedKey &k : keys) {
        if (!keySequenceIsValid(k.key)) {
            *warningMessage = ShortcutSettingsWidget::tr("Invalid key sequence \"%1\".").arg(k.text);
            return false;
        }
    }
    for (const ParsedKey &k : keys) {
        if (textKeySequence(k.key)) {
            *warningMessage = ShortcutSettingsWidget::tr(
                                  "Key sequence \"%1\" will not work in editor.")
                                  .arg(k.text);
            break;
        }
    }
    return true;
}

bool ShortcutSettingsWidget::validateShortcutEdit() const
{
    m_warningLabel->clear();
    QTreeWidgetItem *current = commandList()->currentItem();
    ShortcutItem *item = shortcutItem(current);
    if (!item)
        return true;

    const QString text = m_shortcutEdit->text().trimmed();
    const QList<ParsedKey> currentKeys = keySequencesFromEditString(text);
    QString warningMessage;
    bool isValid = checkValidity(currentKeys, &warningMessage);

    if (isValid) {
        item->m_keys = Utils::transform(currentKeys, &ParsedKey::key);
        auto that = const_cast<ShortcutSettingsWidget *>(this);
        if (item->m_keys != item->m_cmd->defaultKeySequences())
            that->setModified(current, true);
        else
            that->setModified(current, false);
        current->setText(2, keySequencesToNativeString(item->m_keys));
        isValid = !that->markCollisions(item);
        if (!isValid) {
            m_warningLabel->setText(
                tr("Key sequence has potential conflicts. <a href=\"#conflicts\">Show.</a>"));
        }
    }
    if (!warningMessage.isEmpty()) {
        if (m_warningLabel->text().isEmpty())
            m_warningLabel->setText(warningMessage);
        else
            m_warningLabel->setText(m_warningLabel->text() + " " + warningMessage);
    }
    return isValid;
}

bool ShortcutSettingsWidget::filterColumn(const QString &filterString, QTreeWidgetItem *item,
                                          int column) const
{
    const ShortcutItem *scitem = shortcutItem(item);
    if (column == item->columnCount() - 1) { // shortcut
        // filter on the shortcut edit text
        if (!scitem)
            return true;
        const QStringList filters = Utils::transform(filterString.split(kSeparator),
                                                     [](const QString &s) { return s.trimmed(); });
        for (const QKeySequence &k : scitem->m_keys) {
            const QString &keyString = keySequenceToEditString(k);
            const bool found = Utils::anyOf(filters, [keyString](const QString &f) {
                return keyString.contains(f, Qt::CaseInsensitive);
            });
            if (found)
                return false;
        }
        return true;
    }
    QString text;
    if (column == 0 && scitem) { // command id
        text = scitem->m_cmd->id().toString();
    } else {
        text = item->text(column);
    }
    return !text.contains(filterString, Qt::CaseInsensitive);
}

void ShortcutSettingsWidget::setKeySequences(const QList<QKeySequence> &keys)
{
    m_shortcutEdit->setText(keySequencesToEditString(keys));
}

void ShortcutSettingsWidget::showConflicts()
{
    QTreeWidgetItem *current = commandList()->currentItem();
    ShortcutItem *scitem = shortcutItem(current);
    if (scitem)
        setFilterText(keySequencesToEditString(scitem->m_keys));
}

void ShortcutSettingsWidget::resetToDefault()
{
    QTreeWidgetItem *current = commandList()->currentItem();
    ShortcutItem *scitem = shortcutItem(current);
    if (scitem) {
        setKeySequences(scitem->m_cmd->defaultKeySequences());
        foreach (ShortcutItem *item, m_scitems)
            markCollisions(item);
    }
}

void ShortcutSettingsWidget::importAction()
{
    QString fileName = QFileDialog::getOpenFileName(ICore::dialogParent(), tr("Import Keyboard Mapping Scheme"),
        ICore::resourcePath() + QLatin1String("/schemes/"),
        tr("Keyboard Mapping Scheme (*.kms)"));
    if (!fileName.isEmpty()) {

        CommandsFile cf(fileName);
        QMap<QString, QList<QKeySequence>> mapping = cf.importCommands();
        for (ShortcutItem *item : qAsConst(m_scitems)) {
            QString sid = item->m_cmd->id().toString();
            if (mapping.contains(sid)) {
                item->m_keys = mapping.value(sid);
                item->m_item->setText(2, keySequencesToNativeString(item->m_keys));
                if (item->m_item == commandList()->currentItem())
                    emit currentCommandChanged(item->m_item);

                if (item->m_keys != item->m_cmd->defaultKeySequences())
                    setModified(item->m_item, true);
                else
                    setModified(item->m_item, false);
            }
        }

        foreach (ShortcutItem *item, m_scitems)
            markCollisions(item);
    }
}

void ShortcutSettingsWidget::defaultAction()
{
    foreach (ShortcutItem *item, m_scitems) {
        item->m_keys = item->m_cmd->defaultKeySequences();
        item->m_item->setText(2, keySequencesToNativeString(item->m_keys));
        setModified(item->m_item, false);
        if (item->m_item == commandList()->currentItem())
            emit currentCommandChanged(item->m_item);
    }

    foreach (ShortcutItem *item, m_scitems)
        markCollisions(item);
}

void ShortcutSettingsWidget::exportAction()
{
    QString fileName = DocumentManager::getSaveFileNameWithExtension(
        tr("Export Keyboard Mapping Scheme"),
        ICore::resourcePath() + QLatin1String("/schemes/"),
        tr("Keyboard Mapping Scheme (*.kms)"));
    if (!fileName.isEmpty()) {
        CommandsFile cf(fileName);
        cf.exportCommands(m_scitems);
    }
}

void ShortcutSettingsWidget::clear()
{
    QTreeWidget *tree = commandList();
    for (int i = tree->topLevelItemCount()-1; i >= 0 ; --i) {
        delete tree->takeTopLevelItem(i);
    }
    qDeleteAll(m_scitems);
    m_scitems.clear();
}

void ShortcutSettingsWidget::initialize()
{
    clear();
    QMap<QString, QTreeWidgetItem *> sections;

    foreach (Command *c, ActionManager::commands()) {
        if (c->hasAttribute(Command::CA_NonConfigurable))
            continue;
        if (c->action() && c->action()->isSeparator())
            continue;

        QTreeWidgetItem *item = nullptr;
        auto s = new ShortcutItem;
        m_scitems << s;
        item = new QTreeWidgetItem;
        s->m_cmd = c;
        s->m_item = item;

        const QString identifier = c->id().toString();
        int pos = identifier.indexOf(QLatin1Char('.'));
        const QString section = identifier.left(pos);
        const QString subId = identifier.mid(pos + 1);
        if (!sections.contains(section)) {
            QTreeWidgetItem *categoryItem = new QTreeWidgetItem(commandList(), QStringList(section));
            QFont f = categoryItem->font(0);
            f.setBold(true);
            categoryItem->setFont(0, f);
            sections.insert(section, categoryItem);
            commandList()->expandItem(categoryItem);
        }
        sections[section]->addChild(item);

        s->m_keys = c->keySequences();
        item->setText(0, subId);
        item->setText(1, c->description());
        item->setText(2, keySequencesToNativeString(s->m_keys));
        if (s->m_keys != s->m_cmd->defaultKeySequences())
            setModified(item, true);

        item->setData(0, Qt::UserRole, QVariant::fromValue(s));

        markCollisions(s);
    }
    filterChanged(filterText());
}

bool ShortcutSettingsWidget::markCollisions(ShortcutItem *item)
{
    bool hasCollision = false;
    if (!item->m_keys.isEmpty()) {
        Id globalId(Constants::C_GLOBAL);
        const Context itemContext = item->m_cmd->context();
        const bool itemHasGlobalContext = itemContext.contains(globalId);
        for (ShortcutItem *currentItem : qAsConst(m_scitems)) {
            if (item == currentItem)
                continue;
            const bool containsSameShortcut = Utils::anyOf(currentItem->m_keys,
                                                           [item](const QKeySequence &k) {
                                                               return item->m_keys.contains(k);
                                                           });
            if (!containsSameShortcut)
                continue;
            // check if contexts might conflict
            const Context currentContext = currentItem->m_cmd->context();
            bool currentIsConflicting = (itemHasGlobalContext && !currentContext.isEmpty());
            if (!currentIsConflicting) {
                for (const Id &id : currentContext) {
                    if ((id == globalId && !itemContext.isEmpty()) || itemContext.contains(id)) {
                        currentIsConflicting = true;
                        break;
                    }
                }
            }
            if (currentIsConflicting) {
                currentItem->m_item->setForeground(2,
                                                   Utils::creatorTheme()->color(
                                                       Utils::Theme::TextColorError));
                hasCollision = true;
            }
        }
    }
    item->m_item->setForeground(2,
                                hasCollision
                                    ? Utils::creatorTheme()->color(Utils::Theme::TextColorError)
                                    : commandList()->palette().windowText());
    return hasCollision;
}

} // namespace Internal
} // namespace Core
