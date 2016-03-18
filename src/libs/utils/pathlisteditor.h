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

#include "utils_global.h"

#include <QWidget>

#include <functional>

QT_BEGIN_NAMESPACE
class QPushButton;
QT_END_NAMESPACE

namespace Utils {

struct PathListEditorPrivate;

class QTCREATOR_UTILS_EXPORT PathListEditor : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QStringList pathList READ pathList WRITE setPathList DESIGNABLE true)
    Q_PROPERTY(QString fileDialogTitle READ fileDialogTitle WRITE setFileDialogTitle DESIGNABLE true)

public:
    explicit PathListEditor(QWidget *parent = 0);
    virtual ~PathListEditor();

    QString pathListString() const;
    QStringList pathList() const;
    QString fileDialogTitle() const;

    void clear();
    void setPathList(const QStringList &l);
    void setPathList(const QString &pathString);
    void setFileDialogTitle(const QString &l);

protected:
    // Index after which to insert further "Add" buttons
    static const int lastInsertButtonIndex;

    QPushButton *addButton(const QString &text, QObject *parent, std::function<void()> slotFunc);
    QPushButton *insertButton(int index /* -1 */, const QString &text, QObject *parent,
                              std::function<void()> slotFunc);

    QString text() const;
    void setText(const QString &);

    void insertPathAtCursor(const QString &);
    void deletePathAtCursor();

private:
    PathListEditorPrivate *d;
};

} // namespace Utils
