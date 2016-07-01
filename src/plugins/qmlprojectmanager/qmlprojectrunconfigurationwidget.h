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

#include <QWidget>

QT_FORWARD_DECLARE_CLASS(QComboBox)
QT_FORWARD_DECLARE_CLASS(QStandardItemModel)

namespace QmlProjectManager {

class QmlProjectRunConfiguration;

namespace Internal {

const char CURRENT_FILE[]  = QT_TRANSLATE_NOOP("QmlManager", "<Current File>");

class QmlProjectRunConfigurationWidget : public QWidget
{
    Q_OBJECT
public:
    explicit QmlProjectRunConfigurationWidget(QmlProjectRunConfiguration *rc);

private:
    void updateFileComboBox();
    void setMainScript(int index);
    void onViewerArgsChanged();

    QmlProjectRunConfiguration *m_runConfiguration;

    QComboBox *m_fileListCombo;
    QStandardItemModel *m_fileListModel;
};

} // namespace Internal
} // namespace QmlProjectManager
