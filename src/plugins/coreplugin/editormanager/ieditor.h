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
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
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
    IEditor(QObject *parent = 0);
    virtual ~IEditor() {}

    bool duplicateSupported() const;
    void setDuplicateSupported(bool duplicateSupported);

    virtual bool open(QString *errorString, const QString &fileName, const QString &realFileName) = 0;
    virtual IDocument *document() = 0;

    virtual IEditor *duplicate() { return 0; }

    virtual QByteArray saveState() const { return QByteArray(); }
    virtual bool restoreState(const QByteArray &/*state*/) { return true; }

    virtual int currentLine() const { return 0; }
    virtual int currentColumn() const { return 0; }
    virtual void gotoLine(int line, int column = 0, bool centerLine = true) { Q_UNUSED(line) Q_UNUSED(column) Q_UNUSED(centerLine) }

    virtual QWidget *toolBar() = 0;

    virtual bool isDesignModePreferred() const { return false; }

private:
    bool m_duplicateSupported;
};

} // namespace Core

Q_DECLARE_METATYPE(Core::IEditor*)

#endif // IEDITOR_H
