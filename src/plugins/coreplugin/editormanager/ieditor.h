/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef IEDITOR_H
#define IEDITOR_H

#include <coreplugin/core_global.h>
#include <coreplugin/icontext.h>

#include <QMetaType>

namespace Core {

class IDocument;

class CORE_EXPORT IEditor : public IContext
{
    Q_OBJECT
public:

    IEditor(QObject *parent = 0) : IContext(parent) {}
    virtual ~IEditor() {}

    virtual bool createNew(const QString &contents = QString()) = 0;
    virtual bool open(QString *errorString, const QString &fileName, const QString &realFileName) = 0;
    virtual IDocument *document() = 0;
    virtual Core::Id id() const = 0;
    virtual QString displayName() const = 0;
    virtual void setDisplayName(const QString &title) = 0;

    virtual bool duplicateSupported() const = 0;
    virtual IEditor *duplicate(QWidget *parent) = 0;

    virtual QByteArray saveState() const = 0;
    virtual bool restoreState(const QByteArray &state) = 0;

    virtual int currentLine() const { return 0; }
    virtual int currentColumn() const { return 0; }
    virtual void gotoLine(int line, int column = 0) { Q_UNUSED(line) Q_UNUSED(column) }

    virtual bool isTemporary() const = 0;

    virtual QWidget *toolBar() = 0;

    virtual Id preferredModeType() const { return Id(); }

signals:
    void changed();
};

} // namespace Core

Q_DECLARE_METATYPE(Core::IEditor*)

#endif // IEDITOR_H
