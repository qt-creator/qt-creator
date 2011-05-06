/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef VCSBASE_VCSBASEEDITORPARAMETERWIDGET_H
#define VCSBASE_VCSBASEEDITORPARAMETERWIDGET_H

#include "vcsbase_global.h"

#include <QtGui/QWidget>
#include <QtCore/QStringList>

namespace VCSBase {
class VCSBaseEditorParameterWidgetPrivate;

// Documentation->inside.
class VCSBASE_EXPORT VCSBaseEditorParameterWidget : public QWidget
{
    Q_OBJECT
public:
    explicit VCSBaseEditorParameterWidget(QWidget *parent = 0);
    ~VCSBaseEditorParameterWidget();

    QStringList baseArguments() const;
    void setBaseArguments(const QStringList &);

    void addToggleButton(const QString &option, const QString &label,
                         const QString &tooltip = QString());
    void addIgnoreWhiteSpaceButton(const QString &option);
    void addIgnoreBlankLinesButton(const QString &option);

    // Return the effective arguments according to setting.
    QStringList arguments() const;

    // Standard texts
    static QString msgIgnoreWhiteSpaceLabel();
    static QString msgIgnoreWhiteSpaceToolTip();
    static QString msgIgnoreBlankLinesLabel();
    static QString msgIgnoreBlankLinesToolTip();

signals:
    // Trigger a re-run to show changed output according to new argument list.
    void argumentsChanged();

private:
    QScopedPointer<VCSBaseEditorParameterWidgetPrivate> d;
};

} // namespace VCSBase

#endif // VCSBASE_VCSBASEEDITORPARAMETERWIDGET_H
