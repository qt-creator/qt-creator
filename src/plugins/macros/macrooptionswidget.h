/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nicolas Arnaud-Cormos.
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef MACROSPLUGIN_MACROOPTIONSWIDGET_H
#define MACROSPLUGIN_MACROOPTIONSWIDGET_H

#include <QWidget>

#include <QStringList>
#include <QMap>

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

    void initialize();

    void apply();

private slots:
    void remove();
    void changeCurrentItem(QTreeWidgetItem *current);

private:
    void createTable();

private slots:
    void changeDescription(const QString &description);

private:
    Ui::MacroOptionsWidget *m_ui;
    QStringList m_macroToRemove;
    bool m_changingCurrent;

    QMap<QString, QString> m_macroToChange;
};

} // namespace Internal
} // namespace Macros

#endif // MACROSPLUGIN_MACROOPTIONSWIDGET_H
