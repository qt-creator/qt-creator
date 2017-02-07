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

static int translateModifiers(Qt::KeyboardModifiers state,
                                         const QString &text)
{
    int result = 0;
    // The shift modifier only counts when it is not used to type a symbol
    // that is only reachable using the shift key anyway
    if ((state & Qt::ShiftModifier) && (text.size() == 0
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

static QKeySequence keySequenceFromEditString(const QString &editString)
{
    QString text = editString;
    if (Utils::HostOsInfo::isMacHost()) {
        // adapt the modifier names
        text.replace(QLatin1String("Opt"), QLatin1String("Alt"), Qt::CaseInsensitive);
        text.replace(QLatin1String("Ctrl"), QLatin1String("Meta"), Qt::CaseInsensitive);
        text.replace(QLatin1String("Cmd"), QLatin1String("Ctrl"), Qt::CaseInsensitive);
    }
    return QKeySequence::fromString(text, QKeySequence::PortableText);
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

namespace Core {
namespace Internal {

ShortcutButton::ShortcutButton(QWidget *parent)
    : QPushButton(parent)
    , m_key({{ 0, 0, 0, 0 }})
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
        ShortcutButton *that = const_cast<ShortcutButton *>(this);
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
        QKeyEvent *k = static_cast<QKeyEvent*>(evt);
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
        if (qApp->focusWidget())
            qApp->focusWidget()->clearFocus(); // funny things happen otherwise
        qApp->installEventFilter(this);
    } else {
        qApp->removeEventFilter(this);
    }
}

ShortcutSettingsWidget::ShortcutSettingsWidget(QWidget *parent)
    : CommandMappings(parent)
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
    connect(shortcutButton, &ShortcutButton::keySequenceChanged,
            this, &ShortcutSettingsWidget::setKeySequence);
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

ShortcutSettings::ShortcutSettings(QObject *parent)
    : IOptionsPage(parent)
{
    setId(Constants::SETTINGS_ID_SHORTCUTS);
    setDisplayName(tr("Keyboard"));
    setCategory(Constants::SETTINGS_CATEGORY_CORE);
    setDisplayCategory(QCoreApplication::translate("Core", Constants::SETTINGS_TR_CATEGORY_CORE));
    setCategoryIcon(Utils::Icon(Constants::SETTINGS_CATEGORY_CORE_ICON));
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
        item->m_cmd->setKeySequence(item->m_key);
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

void ShortcutSettingsWidget::handleCurrentCommandChanged(QTreeWidgetItem *current)
{
    if (!current || !current->data(0, Qt::UserRole).isValid()) {
        m_shortcutEdit->clear();
        m_warningLabel->clear();
        m_shortcutBox->setEnabled(false);
        return;
    } else {
        ShortcutItem *scitem = qvariant_cast<ShortcutItem *>(current->data(0, Qt::UserRole));
        setKeySequence(scitem->m_key);
        markCollisions(scitem);
        m_shortcutBox->setEnabled(true);
    }
}

bool ShortcutSettingsWidget::validateShortcutEdit() const
{
    m_warningLabel->clear();
    QTreeWidgetItem *current = commandList()->currentItem();
    if (!current || !current->data(0, Qt::UserRole).isValid())
        return true;

    ShortcutItem *item = qvariant_cast<ShortcutItem *>(current->data(0, Qt::UserRole));
    QTC_ASSERT(item, return true);

    bool valid = false;

    const QString text = m_shortcutEdit->text().trimmed();
    const QKeySequence currentKey = keySequenceFromEditString(text);

    if (keySequenceIsValid(currentKey) || text.isEmpty()) {
        item->m_key = currentKey;
        auto that = const_cast<ShortcutSettingsWidget *>(this);
        if (item->m_cmd->defaultKeySequence() != item->m_key)
            that->setModified(current, true);
        else
            that->setModified(current, false);
        current->setText(2, item->m_key.toString(QKeySequence::NativeText));
        valid = !that->markCollisions(item);
        if (!valid) {
            m_warningLabel->setText(
                        tr("Key sequence has potential conflicts. <a href=\"#conflicts\">Show.</a>"));
        }
    } else {
        m_warningLabel->setText(tr("Invalid key sequence."));
    }
    return valid;
}

bool ShortcutSettingsWidget::filterColumn(const QString &filterString, QTreeWidgetItem *item,
                                          int column) const
{
    QString text;
    if (column == item->columnCount() - 1) { // shortcut
        // filter on the shortcut edit text
        if (!item->data(0, Qt::UserRole).isValid())
            return true;
        ShortcutItem *scitem = qvariant_cast<ShortcutItem *>(item->data(0, Qt::UserRole));
        text = keySequenceToEditString(scitem->m_key);
    } else if (column == 0 && item->data(0, Qt::UserRole).isValid()) { // command id
        ShortcutItem *scitem = qvariant_cast<ShortcutItem *>(item->data(0, Qt::UserRole));
        text = scitem->m_cmd->id().toString();
    } else {
        text = item->text(column);
    }
    return !text.contains(filterString, Qt::CaseInsensitive);
}

void ShortcutSettingsWidget::setKeySequence(const QKeySequence &key)
{
    m_shortcutEdit->setText(keySequenceToEditString(key));
}

void ShortcutSettingsWidget::showConflicts()
{
    QTreeWidgetItem *current = commandList()->currentItem();
    if (current && current->data(0, Qt::UserRole).isValid()) {
        ShortcutItem *scitem = qvariant_cast<ShortcutItem *>(current->data(0, Qt::UserRole));
        setFilterText(keySequenceToEditString(scitem->m_key));
    }
}

void ShortcutSettingsWidget::resetToDefault()
{
    QTreeWidgetItem *current = commandList()->currentItem();
    if (current && current->data(0, Qt::UserRole).isValid()) {
        ShortcutItem *scitem = qvariant_cast<ShortcutItem *>(current->data(0, Qt::UserRole));
        setKeySequence(scitem->m_cmd->defaultKeySequence());
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
        QMap<QString, QKeySequence> mapping = cf.importCommands();

        foreach (ShortcutItem *item, m_scitems) {
            QString sid = item->m_cmd->id().toString();
            if (mapping.contains(sid)) {
                item->m_key = mapping.value(sid);
                item->m_item->setText(2, item->m_key.toString(QKeySequence::NativeText));
                if (item->m_item == commandList()->currentItem())
                    currentCommandChanged(item->m_item);

                if (item->m_cmd->defaultKeySequence() != item->m_key)
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
        item->m_key = item->m_cmd->defaultKeySequence();
        item->m_item->setText(2, item->m_key.toString(QKeySequence::NativeText));
        setModified(item->m_item, false);
        if (item->m_item == commandList()->currentItem())
            currentCommandChanged(item->m_item);
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

        QTreeWidgetItem *item = 0;
        ShortcutItem *s = new ShortcutItem;
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

        s->m_key = c->keySequence();
        item->setText(0, subId);
        item->setText(1, c->description());
        item->setText(2, s->m_key.toString(QKeySequence::NativeText));
        if (s->m_cmd->defaultKeySequence() != s->m_key)
            setModified(item, true);

        item->setData(0, Qt::UserRole, qVariantFromValue(s));

        markCollisions(s);
    }
    filterChanged(filterText());
}

bool ShortcutSettingsWidget::markCollisions(ShortcutItem *item)
{
    bool hasCollision = false;
    if (!item->m_key.isEmpty()) {
        Id globalId(Constants::C_GLOBAL);
        const Context itemContext = item->m_cmd->context();
        const bool itemHasGlobalContext = itemContext.contains(globalId);
        foreach (ShortcutItem *currentItem, m_scitems) {
            if (currentItem->m_key.isEmpty() || item == currentItem
                    || item->m_key != currentItem->m_key)
                continue;
            const Context currentContext = currentItem->m_cmd->context();
            bool currentIsConflicting = (itemHasGlobalContext && !currentContext.isEmpty());
            if (!currentIsConflicting) {
                foreach (const Id &id, currentContext) {
                    if ((id == globalId && !itemContext.isEmpty())
                            || itemContext.contains(id)) {
                        currentIsConflicting = true;
                        break;
                    }
                }
            }
            if (currentIsConflicting) {
                currentItem->m_item->setForeground(
                            2, Utils::creatorTheme()->color(Utils::Theme::TextColorError));
                hasCollision = true;
            }
        }
    }
    item->m_item->setForeground(2, hasCollision
                                ? Utils::creatorTheme()->color(Utils::Theme::TextColorError)
                                : commandList()->palette().foreground());
    return hasCollision;
}

} // namespace Internal
} // namespace Core
