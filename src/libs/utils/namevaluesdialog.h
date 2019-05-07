/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "environmentfwd.h"
#include "optional.h"
#include "utils_global.h"

#include <QDialog>

#include <functional>
#include <memory>

QT_BEGIN_NAMESPACE
class QPlainTextEdit;
QT_END_NAMESPACE

namespace Utils {

namespace Internal {
class NameValueItemsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit NameValueItemsWidget(QWidget *parent = nullptr);

    void setEnvironmentItems(const EnvironmentItems &items);
    EnvironmentItems environmentItems() const;

    void setPlaceholderText(const QString &text);

private:
    QPlainTextEdit *m_editor;
};
} // namespace Internal

class QTCREATOR_UTILS_EXPORT NameValuesDialog : public QDialog
{
    Q_OBJECT
public:
    void setNameValueItems(const NameValueItems &items);
    NameValueItems nameValueItems() const;

    void setPlaceholderText(const QString &text);

    using Polisher = std::function<void(QWidget *)>;
    static Utils::optional<NameValueItems> getNameValueItems(QWidget *parent = nullptr,
                                                             const NameValueItems &initial = {},
                                                             const QString &placeholderText = {},
                                                             Polisher polish = {},
                                                             const QString &windowTitle = {},
                                                             const QString &helpText = {});

protected:
    explicit NameValuesDialog(const QString &windowTitle,
                              const QString &helpText,
                              QWidget *parent = {});

private:
    Internal::NameValueItemsWidget *m_editor;
};

} // namespace Utils
