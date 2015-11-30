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

#ifndef THEMEEDITORWIDGET_H
#define THEMEEDITORWIDGET_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QSortFilterProxyModel;
QT_END_NAMESPACE

namespace Utils { class Theme; }

namespace Core {
namespace Internal {
namespace ThemeEditor {

namespace Ui { class ThemeEditorWidget; }

class ThemeSettingsTableModel;

class ThemeEditorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ThemeEditorWidget(QWidget *parent = 0);
    ~ThemeEditorWidget();

    void initFrom(Utils::Theme *theme);

    ThemeSettingsTableModel *model();

    void setReadOnly(bool readOnly);

private slots:
    void changeColor(const QModelIndex &index);

private:
    Ui::ThemeEditorWidget *m_ui;
    bool m_readOnly;
    ThemeSettingsTableModel *m_model;
    QSortFilterProxyModel *m_proxyModel;
};

} // namespace ThemeEditor
} // namespace Internal
} // namespace Core

#endif // THEMEEDITORWIDGET_H
