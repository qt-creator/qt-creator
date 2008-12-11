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
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#ifndef GDBOPTIONPAGE_H
#define GDBOPTIONPAGE_H

#include "ui_gdboptionpage.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <QtGui/QWidget>

namespace ExtensionSystem { class PluginManager; }

namespace Debugger {
namespace Internal {

class GdbSettings;

class GdbOptionPage : public Core::IOptionsPage
{
    Q_OBJECT

public:
    GdbOptionPage(GdbSettings *settings);

    QString name() const;
    QString category() const;
    QString trCategory() const;

    QWidget *createPage(QWidget *parent);
    void finished(bool accepted);

public slots:
    void onGdbLocationChanged();
    void onScriptFileChanged();

private:
    ExtensionSystem::PluginManager *m_pm;
    Ui::GdbOptionPage m_ui;

    GdbSettings *m_settings;
};

#if 0
class TypeMacroPage : public Core::IOptionsPage
{
    Q_OBJECT

public:
    TypeMacroPage(GdbSettings *settings);

    QString name() const;
    QString category() const;
    QString trCategory() const;

    QWidget *createPage(QWidget *parent);
    void finished(bool accepted);

private slots:
    void onAddButton();
    void onDelButton();
    void currentItemChanged(QTreeWidgetItem *item);
    void updateButtonState();

private:
    ExtensionSystem::PluginManager *m_pm;
    Ui::TypeMacroPage m_ui;

    GdbSettings *m_settings;
    QWidget *m_widget;
};
#endif

} // namespace Internal
} // namespace Debugger

#endif // GDBOPTIONPAGE_H
