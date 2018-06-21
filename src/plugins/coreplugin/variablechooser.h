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

#include "core_global.h"

#include <QWidget>

#include <utils/macroexpander.h>
#include <functional>

namespace Core {

namespace Internal { class VariableChooserPrivate; }

class CORE_EXPORT VariableChooser : public QWidget
{
    Q_OBJECT

public:
    explicit VariableChooser(QWidget *parent = nullptr);
    ~VariableChooser() override;

    void addMacroExpanderProvider(const Utils::MacroExpanderProvider &provider);
    void addSupportedWidget(QWidget *textcontrol, const QByteArray &ownName = QByteArray());

    static void addSupportForChildWidgets(QWidget *parent, Utils::MacroExpander *expander);

protected:
    bool event(QEvent *ev) override;
    bool eventFilter(QObject *, QEvent *event) override;

private:
    Internal::VariableChooserPrivate *d;
};

} // namespace Core
