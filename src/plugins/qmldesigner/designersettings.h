/****************************************************************************
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
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
****************************************************************************/


#ifndef DESIGNERSETTINGS_H
#define DESIGNERSETTINGS_H

#include <QtGlobal>
#include <QString>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace QmlDesigner {

class DesignerSettings {
public:
    DesignerSettings();

    void fromSettings(QSettings *);
    void toSettings(QSettings *) const;

    bool equals(const DesignerSettings &other) const;
    int itemSpacing;
    int containerPadding;
    int canvasWidth;
    int canvasHeight;
    bool warningsInDesigner;
    bool designerWarningsInEditor;
    bool showDebugView;
    bool enableDebugView;
    bool alwaysSaveInCrumbleBar;
    bool useOnlyFallbackPuppet;
    QString puppetFallbackDirectory;
    QString puppetToplevelBuildDirectory;
    QString controlsStyle;
};

inline bool operator==(const DesignerSettings &s1, const DesignerSettings &s2)
{ return s1.equals(s2); }
inline bool operator!=(const DesignerSettings &s1, const DesignerSettings &s2)
{ return !s1.equals(s2); }

} // namespace QmlDesigner

#endif // DESIGNERSETTINGS_H
