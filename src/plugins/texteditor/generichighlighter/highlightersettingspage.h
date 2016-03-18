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

#include "highlightdefinitionmetadata.h"
#include "../texteditoroptionspage.h"

QT_BEGIN_NAMESPACE
template <class> class QList;
QT_END_NAMESPACE

namespace TextEditor {

class HighlighterSettings;

class HighlighterSettingsPage : public TextEditorOptionsPage
{
    Q_OBJECT

public:
    HighlighterSettingsPage(Core::Id id, QObject *parent);
    ~HighlighterSettingsPage();

    QWidget *widget();
    void apply();
    void finish();

    const HighlighterSettings &highlighterSettings() const;

private:
    void manageDefinitions(const QList<Internal::DefinitionMetaDataPtr> &metaData);
    void showError();
    void ignoreDownloadReply();
    void setFallbackLocationState(bool checked);
    void setDownloadDefinitionsState(bool valid);

    void resetDefinitionsLocation();
    void requestAvailableDefinitionsMetaData();
    void settingsFromUI();
    void settingsToUI();

    bool settingsChanged() const;

    bool m_requestHighlightFileRegistration;

    struct HighlighterSettingsPagePrivate;
    HighlighterSettingsPagePrivate *m_d;
};

} // namespace TextEditor
