/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "shortcutsettings.h"
#include "ui_shortcutsettings.h"
#include "actionmanager_p.h"
#include "actionmanager/command.h"
#include "command_p.h"
#include "commandsfile.h"
#include "coreconstants.h"
#include "filemanager.h"
#include "icore.h"
#include "uniqueidmanager.h"

#include <QtGui/QKeyEvent>
#include <QtGui/QShortcut>
#include <QtGui/QHeaderView>
#include <QtGui/QFileDialog>
#include <QtDebug>

Q_DECLARE_METATYPE(Core::Internal::ShortcutItem*);

using namespace Core;
using namespace Core::Internal;

ShortcutSettings::ShortcutSettings(QObject *parent)
    : IOptionsPage(parent)
{
}

ShortcutSettings::~ShortcutSettings()
{
}

// IOptionsPage
QString ShortcutSettings::name() const
{
    return tr("Keyboard");
}

QString ShortcutSettings::category() const
{
    return QLatin1String("Environment");
}

QString ShortcutSettings::trCategory() const
{
    return tr("Environment");
}

QWidget *ShortcutSettings::createPage(QWidget *parent)
{
    m_keyNum = m_key[0] = m_key[1] = m_key[2] = m_key[3] = 0;

    m_page = new Ui_ShortcutSettings();
    QWidget *w = new QWidget(parent);
    m_page->setupUi(w);

    m_page->resetButton->setIcon(QIcon(Constants::ICON_RESET));
    m_page->shortcutEdit->installEventFilter(this);

    connect(m_page->resetButton, SIGNAL(clicked()),
        this, SLOT(resetKeySequence()));
    connect(m_page->removeButton, SIGNAL(clicked()),
        this, SLOT(removeKeySequence()));
    connect(m_page->exportButton, SIGNAL(clicked()),
        this, SLOT(exportAction()));
    connect(m_page->importButton, SIGNAL(clicked()),
        this, SLOT(importAction()));
    connect(m_page->defaultButton, SIGNAL(clicked()),
        this, SLOT(defaultAction()));

    initialize();

    m_page->commandList->sortByColumn(0, Qt::AscendingOrder);

    connect(m_page->filterEdit, SIGNAL(textChanged(QString)), this, SLOT(filterChanged(QString)));
    connect(m_page->commandList, SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)),
        this, SLOT(commandChanged(QTreeWidgetItem *)));
    connect(m_page->shortcutEdit, SIGNAL(textChanged(QString)), this, SLOT(keyChanged()));

    QHeaderView *hv = m_page->commandList->header();
    hv->resizeSection(0, 210);
    hv->resizeSection(1, 110);
    hv->setStretchLastSection(true);

    commandChanged(0);

    return w;
}

void ShortcutSettings::apply()
{
    foreach (ShortcutItem *item, m_scitems)
        item->m_cmd->setKeySequence(item->m_key);
}

void ShortcutSettings::finish()
{
    qDeleteAll(m_scitems);
    m_scitems.clear();

    delete m_page;
}

bool ShortcutSettings::eventFilter(QObject *o, QEvent *e)
{
    Q_UNUSED(o);

    if ( e->type() == QEvent::KeyPress ) {
        QKeyEvent *k = static_cast<QKeyEvent*>(e);
        handleKeyEvent(k);
        return true;
    }

    if ( e->type() == QEvent::Shortcut ||
         e->type() == QEvent::ShortcutOverride  ||
         e->type() == QEvent::KeyRelease )
        return true;

    return false;
}

void ShortcutSettings::commandChanged(QTreeWidgetItem *current)
{
    if (!current || !current->data(0, Qt::UserRole).isValid()) {
        m_page->shortcutEdit->setText("");
        m_page->seqGrp->setEnabled(false);
        return;
    }
    m_page->seqGrp->setEnabled(true);
    ShortcutItem *scitem = qVariantValue<ShortcutItem *>(current->data(0, Qt::UserRole));
    setKeySequence(scitem->m_key);
}

void ShortcutSettings::filterChanged(const QString &f)
{
    for (int i=0; i<m_page->commandList->topLevelItemCount(); ++i) {
        QTreeWidgetItem *item = m_page->commandList->topLevelItem(i);
        item->setHidden(filter(f, item));
    }
}

void ShortcutSettings::keyChanged()
{
    QTreeWidgetItem *current = m_page->commandList->currentItem();
    if (current && current->data(0, Qt::UserRole).isValid()) {
        ShortcutItem *scitem = qVariantValue<ShortcutItem *>(current->data(0, Qt::UserRole));
        scitem->m_key = QKeySequence(m_key[0], m_key[1], m_key[2], m_key[3]);
        current->setText(2, scitem->m_key);
    }
}

void ShortcutSettings::setKeySequence(const QKeySequence &key)
{
    m_keyNum = m_key[0] = m_key[1] = m_key[2] = m_key[3] = 0;
    m_keyNum = key.count();
    for (int i = 0; i < m_keyNum; ++i) {
        m_key[i] = key[i];
    }
    m_page->shortcutEdit->setText(key);
}

bool ShortcutSettings::filter(const QString &f, const QTreeWidgetItem *item)
{
    if (item->childCount() == 0) {
        if (f.isEmpty())
            return false;
        for (int i = 0; i < item->columnCount(); ++i) {
            if (item->text(i).contains(f, Qt::CaseInsensitive))
                return false;
        }
        return true;
    }

    bool found = false;
    for (int i = 0; i < item->childCount(); ++i) {
        QTreeWidgetItem *citem = item->child(i);
        if (filter(f, citem)) {
            citem->setHidden(true);
        } else {
            citem->setHidden(false);
            found = true;
        }
    }
    return !found;
}

void ShortcutSettings::resetKeySequence()
{
    QTreeWidgetItem *current = m_page->commandList->currentItem();
    if (current && current->data(0, Qt::UserRole).isValid()) {
        ShortcutItem *scitem = qVariantValue<ShortcutItem *>(current->data(0, Qt::UserRole));
        setKeySequence(scitem->m_cmd->defaultKeySequence());
    }
}

void ShortcutSettings::removeKeySequence()
{
    m_keyNum = m_key[0] = m_key[1] = m_key[2] = m_key[3] = 0;
    m_page->shortcutEdit->clear();
}

void ShortcutSettings::importAction()
{
    UniqueIDManager *uidm = UniqueIDManager::instance();

    QString fileName = QFileDialog::getOpenFileName(0, tr("Import Keyboard Mapping Scheme"),
        ICore::instance()->resourcePath() + "/schemes/",
        tr("Keyboard Mapping Scheme (*.kms)"));
    if (!fileName.isEmpty()) {
        CommandsFile cf(fileName);
        QMap<QString, QKeySequence> mapping = cf.importCommands();

        foreach (ShortcutItem *item, m_scitems) {
            QString sid = uidm->stringForUniqueIdentifier(item->m_cmd->id());
            if (mapping.contains(sid)) {
                item->m_key = mapping.value(sid);
                item->m_item->setText(2, item->m_key);
                if (item->m_item == m_page->commandList->currentItem())
                    commandChanged(item->m_item);
            }
        }
    }
}

void ShortcutSettings::defaultAction()
{
    foreach (ShortcutItem *item, m_scitems) {
        item->m_key = item->m_cmd->defaultKeySequence();
        item->m_item->setText(2, item->m_key);
        if (item->m_item == m_page->commandList->currentItem())
            commandChanged(item->m_item);
    }
}

void ShortcutSettings::exportAction()
{
    QString fileName = ICore::instance()->fileManager()->getSaveFileNameWithExtension(
        tr("Export Keyboard Mapping Scheme"),
        ICore::instance()->resourcePath() + "/schemes/",
        tr("Keyboard Mapping Scheme (*.kms)"),
        ".kms");
    if (!fileName.isEmpty()) {
        CommandsFile cf(fileName);
        cf.exportCommands(m_scitems);
    }
}

void ShortcutSettings::initialize()
{
    m_am = ActionManagerPrivate::instance();
    UniqueIDManager *uidm = UniqueIDManager::instance();

    foreach (Command *c, m_am->commands()) {
        if (c->hasAttribute(Command::CA_NonConfigureable))
            continue;
        if (c->action() && c->action()->isSeparator())
            continue;

        QTreeWidgetItem *item = 0;
        ShortcutItem *s = new ShortcutItem;
        m_scitems << s;
        item = new QTreeWidgetItem(m_page->commandList);
        s->m_cmd = c;
        s->m_item = item;

        item->setText(0, uidm->stringForUniqueIdentifier(c->id()));

        if (c->action()) {
            QString text = c->hasAttribute(Command::CA_UpdateText) && !c->defaultText().isNull() ? c->defaultText() : c->action()->text();
            text.remove(QRegExp("&(?!&)"));
            s->m_key = c->action()->shortcut();
            item->setText(1, text);
        } else {
            s->m_key = c->shortcut()->key();
            item->setText(1, c->shortcut()->whatsThis());
        }

        item->setText(2, s->m_key);
        item->setData(0, Qt::UserRole, qVariantFromValue(s));
    }
}

void ShortcutSettings::handleKeyEvent(QKeyEvent *e)
{
    int nextKey = e->key();
    if ( m_keyNum > 3 ||
         nextKey == Qt::Key_Control ||
         nextKey == Qt::Key_Shift ||
         nextKey == Qt::Key_Meta ||
         nextKey == Qt::Key_Alt )
         return;

    nextKey |= translateModifiers(e->modifiers(), e->text());
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
    QKeySequence ks(m_key[0], m_key[1], m_key[2], m_key[3]);
    m_page->shortcutEdit->setText(ks);
    e->accept();
}

int ShortcutSettings::translateModifiers(Qt::KeyboardModifiers state,
                                         const QString &text)
{
    int result = 0;
    // The shift modifier only counts when it is not used to type a symbol
    // that is only reachable using the shift key anyway
    if ((state & Qt::ShiftModifier) && (text.size() == 0
                                        || !text.at(0).isPrint()
                                        || text.at(0).isLetter()
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
