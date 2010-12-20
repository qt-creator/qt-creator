/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nicolas Arnaud-Cormos.
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef MACROSPLUGIN_MACROOPTIONSWIDGET_H
#define MACROSPLUGIN_MACROOPTIONSWIDGET_H

#include <QWidget>
#include <QPixmap>
#include <QMap>
#include <QStringList>

class QTreeWidgetItem;


namespace Macros {
namespace Internal {

class MacroSettings;

namespace Ui {
    class MacroOptionsWidget;
}

class MacroOptionsWidget : public QWidget {
    Q_OBJECT
public:
    explicit MacroOptionsWidget(QWidget *parent = 0);
    ~MacroOptionsWidget();

    void setSettings(const MacroSettings &s);

    void apply();

private slots:
    void addDirectoy();
    void remove();
    void setDefault();
    void changeCurrentItem(QTreeWidgetItem *current);

private:
    void appendDirectory(const QString &directory, bool isDefault = false);
    void changeData(QTreeWidgetItem *current, int column, QVariant value);

private slots:
    void changeDescription(const QString &description);
    void changeShortcut(bool shortcut);

private:
    Ui::MacroOptionsWidget *ui;
    QStringList m_macroToRemove;
    QStringList m_directories;
    bool changingCurrent;

    struct ChangeSet {
        QString description;
        bool shortcut;
    };
    QMap<QString, ChangeSet> m_macroToChange;
};

} // namespace Internal
} // namespace Macros

#endif // MACROSPLUGIN_MACROOPTIONSWIDGET_H
