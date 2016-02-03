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

#include "vcsbase_global.h"

#include  <QWidget>

QT_BEGIN_NAMESPACE
class QCompleter;
QT_END_NAMESPACE

namespace VcsBase {

struct SubmitFieldWidgetPrivate;

class VCSBASE_EXPORT SubmitFieldWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QStringList fields READ fields WRITE setFields DESIGNABLE true)
    Q_PROPERTY(bool hasBrowseButton READ hasBrowseButton WRITE setHasBrowseButton DESIGNABLE true)
    Q_PROPERTY(bool allowDuplicateFields READ allowDuplicateFields WRITE setAllowDuplicateFields DESIGNABLE true)

public:
    explicit SubmitFieldWidget(QWidget *parent = 0);
    ~SubmitFieldWidget() override;

    QStringList fields() const;
    void setFields(const QStringList&);

    bool hasBrowseButton() const;
    void setHasBrowseButton(bool d);

    // Allow several entries for fields ("reviewed-by: a", "reviewed-by: b")
    bool allowDuplicateFields() const;
    void setAllowDuplicateFields(bool);

    QCompleter *completer() const;
    void setCompleter(QCompleter *c);

    QString fieldValue(int pos) const;
    void setFieldValue(int pos, const QString &value);

    QString fieldValues() const;

signals:
    void browseButtonClicked(int pos, const QString &field);

private:
    void slotRemove();
    void slotComboIndexChanged(int);
    void slotBrowseButtonClicked();

    void removeField(int index);
    bool comboIndexChange(int fieldNumber, int index);
    void createField(const QString &f);

    SubmitFieldWidgetPrivate *d;
};

} // namespace VcsBase
