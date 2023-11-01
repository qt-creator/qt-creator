// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../core_global.h"

#include <utils/filesearch.h>

#include <QObject>

QT_BEGIN_NAMESPACE
class QAbstractListModel;
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

    enum { CompletionModelFindFlagsRole = Qt::UserRole + 1 };

    static Utils::FindFlags findFlags();
    static bool hasFindFlag(Utils::FindFlag flag);
    static void updateFindCompletion(const QString &text, Utils::FindFlags flags = {});
    static void updateReplaceCompletion(const QString &text);
    static QAbstractListModel *findCompletionModel();
    static QStringListModel *replaceCompletionModel();
    static void setUseFakeVim(bool on);
    static void openFindToolBar(FindDirection direction);
    static void openFindDialog(IFindFilter *filter, const QString &findString = {});

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
