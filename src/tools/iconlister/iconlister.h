/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include <QString>
#include <QIcon>

struct IconInfo
{
    QIcon icon;
    QString id;
    QString idContext;
    QString description;
};

class IconLister
{
public:
    IconLister() = default;

    enum ThemeKind {
        ThemeKindDark,
        ThemeKindLight
    };

    static void setCreatorTheme(ThemeKind themeKind);
    static void generateJson();
    static void generateIcons(ThemeKind theme, int dpr);

private:
    void addAutoTestIcons();
    void addCoreIcons();
    void addDiffEditorIcons();
    void addHelpIcons();
    void addUtilsIcons();
    void addProjectExplorerIcons();
    void addDebuggerIcons();
    void addCPlusPlusIcons();
    void addQmlDesignerIcons();
    void addProfilerTimelineIcons();
    void addWizardIcons();
    void addWelcomeIcons();

    void addAndroidIcons();
    void addIosIcons();
    void addBareMetalIcons();
    void addQnxIcons();
    void addWinRTIcons();
    void addBoot2QtIcons();
    void addVxWorksIcons();

    void addAllIcons();

    void saveJson() const;
    void saveIcons(ThemeKind theme, int dpr) const;

    QList<IconInfo> m_icons;
};
