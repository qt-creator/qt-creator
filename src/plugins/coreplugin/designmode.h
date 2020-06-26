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

#include <coreplugin/imode.h>

namespace Core {
class IEditor;

/**
  * A global mode for Design pane - used by Bauhaus (QML Designer) and
  * Qt Designer. Other plugins can register themselves by registerDesignWidget()
  * and giving a list of mimetypes that the editor understands, as well as an instance
  * to the main editor widget itself.
  */

class CORE_EXPORT DesignMode final : public IMode
{
    Q_OBJECT

public:
    static DesignMode *instance();

    static void setDesignModeIsRequired();

    static void registerDesignWidget(QWidget *widget,
                                     const QStringList &mimeTypes,
                                     const Context &context);
    static void unregisterDesignWidget(QWidget *widget);

    static void createModeIfRequired();
    static void destroyModeIfRequired();

signals:
    void actionsUpdated(Core::IEditor *editor);

private:
    DesignMode();
    ~DesignMode() final;

    void updateActions();

    void currentEditorChanged(IEditor *editor);
    void updateContext(Utils::Id newMode, Utils::Id oldMode);
    void setActiveContext(const Context &context);
};

} // namespace Core
