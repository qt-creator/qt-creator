/**************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
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

#ifndef QMLCONSOLEITEMMODEL_H
#define QMLCONSOLEITEMMODEL_H

#include <qmljs/consoleitem.h>
#include <utils/treemodel.h>

#include <QItemSelectionModel>

QT_FORWARD_DECLARE_CLASS(QFont)

namespace QmlJSTools {
namespace Internal {

class QmlConsoleItemModel : public Utils::TreeModel
{
    Q_OBJECT
public:

    explicit QmlConsoleItemModel(QObject *parent = 0);

    void shiftEditableRow();

    void appendItem(QmlJS::ConsoleItem *item, int position = -1);
    void appendMessage(QmlJS::ConsoleItem::ItemType itemType, const QString &message,
                       int position = -1);

    int sizeOfFile(const QFont &font);
    int sizeOfLineNumber(const QFont &font);

public slots:
    void clear();

signals:
    void selectEditableRow(const QModelIndex &index, QItemSelectionModel::SelectionFlags flags);

private:
    int m_maxSizeOfFileName;
};

} // Internal
} // QmlJSTools

#endif // QMLCONSOLEITEMMODEL_H
