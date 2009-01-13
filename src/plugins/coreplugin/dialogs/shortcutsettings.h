/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#ifndef SHORTCUTSETTINGS_H
#define SHORTCUTSETTINGS_H

#include <coreplugin/dialogs/ioptionspage.h>

#include <QtCore/QObject>
#include <QtGui/QKeySequence>
#include <QtGui/QTreeWidgetItem>
#include <QtGui/QKeyEvent>

QT_BEGIN_NAMESPACE
class Ui_ShortcutSettings;
QT_END_NAMESPACE

namespace Core {

class ICommand;

namespace Internal {

class ActionManagerPrivate;
class Command;
class MainWindow;

struct ShortcutItem
{
    ICommand *m_cmd;
    QKeySequence m_key;
    QTreeWidgetItem *m_item;
};


class ShortcutSettings : public Core::IOptionsPage
{
    Q_OBJECT

public:
    ShortcutSettings(QObject *parent = 0);
    ~ShortcutSettings();

    // IOptionsPage
    QString name() const;
    QString category() const;
    QString trCategory() const;

    QWidget *createPage(QWidget *parent);
    void apply();
    void finish();

protected:
    bool eventFilter(QObject *o, QEvent *e);

private slots:
    void commandChanged(QTreeWidgetItem *current);
    void filterChanged(const QString &f);
    void keyChanged();
    void resetKeySequence();
    void removeKeySequence();
    void importAction();
    void exportAction();
    void defaultAction();

private:
    void setKeySequence(const QKeySequence &key);
    bool filter(const QString &f, const QTreeWidgetItem *item);
    void initialize();

    void handleKeyEvent(QKeyEvent *e);
    int translateModifiers(Qt::KeyboardModifiers state, const QString &text);

    QList<ShortcutItem *> m_scitems;
    ActionManagerPrivate *m_am;
    int m_key[4], m_keyNum;
    Ui_ShortcutSettings *m_page;
};

} // namespace Internal
} // namespace Core

#endif // SHORTCUTSETTINGS_H
