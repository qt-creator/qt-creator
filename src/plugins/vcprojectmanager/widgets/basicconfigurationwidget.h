/**************************************************************************
**
** Copyright (c) 2013 Bojan Petrovic
** Copyright (c) 2013 Radovan Zivkovic
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/
#ifndef BASICCONFIGURATIONWIDGET_H
#define BASICCONFIGURATIONWIDGET_H

#include <QWidget>
#include <QHash>

class QTableWidget;
class QLabel;
class QTableWidgetItem;

namespace VcProjectManager {
namespace Internal {

class BasicConfigurationWidget : public QWidget
{
    Q_OBJECT
public:
    explicit BasicConfigurationWidget(QWidget *parent = 0);
    ~BasicConfigurationWidget();

    void insertTableRow(const QString &column0, QWidget *widget, const QString &description);

private slots:
    void onTableCellClicked(int row, int column);

private:
    void setLineText(const QString &text);
    void setLineDescription(const QString &desc);

private:
    QTableWidget *m_tableWidget;
    QLabel *m_purposeLineLabel;
    QLabel *m_purposeDescription;

    QHash<QString, QString> m_labelDescription;
};

} // namespace Internal
} // namespace VcProjectManager

#endif // BASICCONFIGURATIONWIDGET_H
