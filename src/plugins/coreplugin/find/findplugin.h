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

#include "textfindconstants.h"

#include <QObject>

QT_BEGIN_NAMESPACE
class QStringListModel;
QT_END_NAMESPACE

namespace Core {
class IFindFilter;
namespace Internal { class CorePlugin; }

class CORE_EXPORT Find : public QObject
{
    Q_OBJECT

public:
    static Find *instance();

    enum FindDirection {
        FindForwardDirection,
        FindBackwardDirection
    };

    static FindFlags findFlags();
    static bool hasFindFlag(FindFlag flag);
    static void updateFindCompletion(const QString &text);
    static void updateReplaceCompletion(const QString &text);
    static QStringListModel *findCompletionModel();
    static QStringListModel *replaceCompletionModel();
    static void setUseFakeVim(bool on);
    static void openFindToolBar(FindDirection direction);
    static void openFindDialog(IFindFilter *filter);

    static void setCaseSensitive(bool sensitive);
    static void setWholeWord(bool wholeOnly);
    static void setBackward(bool backward);
    static void setRegularExpression(bool regExp);
    static void setPreserveCase(bool preserveCase);

signals:
    void findFlagsChanged();

private:
    friend class Internal::CorePlugin;
    static void initialize();
    static void extensionsInitialized();
    static void aboutToShutdown();
    static void destroy();
};

} // namespace Core
