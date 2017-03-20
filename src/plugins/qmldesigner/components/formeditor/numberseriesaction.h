/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/
#pragma once

#include <QWidgetAction>
#include <QPointer>

QT_BEGIN_NAMESPACE
class QStandardItemModel;
QT_END_NAMESPACE

namespace QmlDesigner {

class NumberSeriesAction : public QWidgetAction
{
Q_OBJECT
public:
    explicit NumberSeriesAction(QObject *parent = 0);

    void addEntry(const QString &text, const QVariant &value);
    void setCurrentEntryIndex(int index);

    QVariant currentValue() const;

protected:
    QWidget *createWidget(QWidget *parent);

signals:
    void valueChanged(const QVariant &value);

private:
    void emitValueChanged(int index);

private:
    QPointer<QStandardItemModel> m_comboBoxModel;
    int m_comboBoxModelIndex;
};

} // namespace QmlDesigner
