/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nicolas Arnaud-Cormos.
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef MACROSPLUGIN_MACROOPTIONSWIDGET_H
#define MACROSPLUGIN_MACROOPTIONSWIDGET_H

#include <QtGui/QWidget>

#include <QtCore/QStringList>
#include <QtCore/QMap>

QT_BEGIN_NAMESPACE
class QTreeWidgetItem;
QT_END_NAMESPACE


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
