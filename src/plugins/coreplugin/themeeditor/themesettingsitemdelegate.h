/****************************************************************************
**
** Copyright (C) 2015 Thorben Kroeger <thorbenkroeger@gmail.com>.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef THEMESETTINGSITEMDELEGATE_H
#define THEMESETTINGSITEMDELEGATE_H

#include "themecolors.h"
#include "colorvariable.h"

#include <QStyledItemDelegate>

QT_BEGIN_NAMESPACE
class QComboBox;
QT_END_NAMESPACE

namespace Core {
namespace Internal {
namespace ThemeEditor {

class ThemeSettingsItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT

    enum Action {
        Action_NoAction,
        Action_ChooseNamedColor,
        Action_MakeUnnamed,
        Action_CreateNew
    };

public:
    ThemeSettingsItemDelegate(QObject *parent);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;

    void popupMenu();

private:
    QWidget *createColorEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;

    mutable QMap<int, QPair<Action, ColorVariable::Ptr> > m_actions;
    mutable QComboBox *m_comboBox;
};

} // namespace ThemeEditor
} // namespace Internal
} // namespace Core

#endif // THEMESETTINGSITEMDELEGATE_H
