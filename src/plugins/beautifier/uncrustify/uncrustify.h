/****************************************************************************
**
** Copyright (C) 2016 Lorenz Haas
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

#include "../beautifierabstracttool.h"

#include "uncrustifyoptionspage.h"
#include "uncrustifysettings.h"

namespace Beautifier {
namespace Internal {

class Uncrustify : public BeautifierAbstractTool
{
    Q_OBJECT

public:
    Uncrustify();

    QString id() const override;
    void updateActions(Core::IEditor *editor) override;
    TextEditor::Command command() const override;
    bool isApplicable(const Core::IDocument *document) const override;

private:
    void formatFile();
    void formatSelectedText();
    QString configurationFile() const;
    TextEditor::Command command(const QString &cfgFile, bool fragment = false) const;

    QAction *m_formatFile = nullptr;
    QAction *m_formatRange = nullptr;
    UncrustifySettings m_settings;
    UncrustifyOptionsPage m_page{&m_settings};
};

} // namespace Internal
} // namespace Beautifier
