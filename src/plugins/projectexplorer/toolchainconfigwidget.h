/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef TOOLCHAINCONFIGWIDGET_H
#define TOOLCHAINCONFIGWIDGET_H

#include "projectexplorer_export.h"

#include <QtGui/QWidget>

QT_FORWARD_DECLARE_CLASS(QFormLayout)
QT_FORWARD_DECLARE_CLASS(QGridLayout)

namespace ProjectExplorer {

namespace Internal {
class ToolChainConfigWidgetPrivate;
} // namespace Internal

class ToolChain;

// --------------------------------------------------------------------------
// ToolChainConfigWidget
// --------------------------------------------------------------------------

class PROJECTEXPLORER_EXPORT ToolChainConfigWidget : public QWidget
{
    Q_OBJECT

public:
    ToolChainConfigWidget(ProjectExplorer::ToolChain *);

    void setDisplayName(const QString &);
    virtual void apply() = 0;
    virtual void discard() = 0;

    ProjectExplorer::ToolChain *toolChain() const;

    virtual bool isDirty() const = 0;

signals:
    void dirty(ProjectExplorer::ToolChain *);

protected slots:
    void emitDirty();
    void setErrorMessage(const QString &);
    void clearErrorMessage();

protected:
    void addDebuggerCommandControls(QFormLayout *lt,
                                    const QStringList &versionArguments = QStringList());
    void addDebuggerCommandControls(QGridLayout *lt,
                                    int row = 0, int column = 0,
                                    const QStringList &versionArguments = QStringList());
    void addDebuggerAutoDetection(QObject *receiver, const char *autoDetectSlot);
    void addErrorLabel(QFormLayout *lt);
    void addErrorLabel(QGridLayout *lt, int row = 0, int column = 0, int colSpan = 1);

    QString debuggerCommand() const;
    void setDebuggerCommand(const QString &d);

private:
    void ensureDebuggerPathChooser(const QStringList &versionArguments);

    Internal::ToolChainConfigWidgetPrivate *m_d;
};

} // namespace ProjectExplorer

#endif // TOOLCHAINCONFIGWIDGET_H
