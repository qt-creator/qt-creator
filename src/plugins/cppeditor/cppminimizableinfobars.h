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

#include <utils/id.h>

#include <QAction>
#include <QHash>
#include <QObject>

#include <functional>

namespace Utils {
class InfoBar;
}

namespace CppEditor {
namespace Internal {

class MinimizableInfoBars : public QObject
{
    Q_OBJECT

public:
    using DiagnosticWidgetCreator = std::function<QWidget *()>;
    using ActionCreator = std::function<QAction *(QWidget *widget)>;
    using Actions = QHash<Utils::Id, QAction *>;

    static Actions createShowInfoBarActions(const ActionCreator &actionCreator);

public:
    explicit MinimizableInfoBars(Utils::InfoBar &infoBar, QObject *parent = nullptr);

    // Expected call order: processHasProjectPart(), processHeaderDiagnostics()
    void processHasProjectPart(bool hasProjectPart);
    void processHeaderDiagnostics(const DiagnosticWidgetCreator &diagnosticWidgetCreator);

signals:
    void showAction(const Utils::Id &id, bool show);

private:
    void updateNoProjectConfiguration();
    void updateHeaderErrors();

    void addNoProjectConfigurationEntry(const Utils::Id &id);
    void addHeaderErrorEntry(const Utils::Id &id,
                             const DiagnosticWidgetCreator &diagnosticWidgetCreator);

private:
    Utils::InfoBar &m_infoBar;

    bool m_hasProjectPart = true;
    DiagnosticWidgetCreator m_diagnosticWidgetCreator;
};

} // namespace Internal
} // namespace CppEditor
