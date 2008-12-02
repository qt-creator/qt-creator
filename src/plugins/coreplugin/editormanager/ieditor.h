/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** 
** Non-Open Source Usage  
** 
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.  
** 
** GNU General Public License Usage 
** 
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception version
** 1.2, included in the file GPL_EXCEPTION.txt in this package.  
** 
***************************************************************************/

#ifndef IEDITOR_H
#define IEDITOR_H

#include <coreplugin/core_global.h>
#include <coreplugin/icontext.h>
#include <coreplugin/ifile.h>

QT_BEGIN_NAMESPACE
class QToolBar;
QT_END_NAMESPACE

namespace Core {

/*!
  \class Core::IEditor
  \brief The IEditor is an interface for providing different editors for different file types.

  Classes that implement this interface are for example the editors for
  C++ files, ui-files and resource files.

  Whenever a user wants to edit or create a file, the EditorManager scans all
  EditorFactoryInterfaces for suitable editors. The selected EditorFactory
  is then asked to create an editor, which must implement this interface.

  Guidelines for implementing:
  \list
  \o displayName() is used as a user visible description of the document (usually filename w/o path).
  \o kind() must be the same value as the kind() of the corresponding EditorFactory.
  \o The changed() signal should be emitted when the modified state of the document changes
     (so /bold{not} every time the document changes, but /bold{only once}).
  \o If duplication is supported, you need to ensure that all duplicates
        return the same file().
  \endlist

  \sa Core::EditorFactoryInterface Core::IContext

*/

class CORE_EXPORT IEditor : public IContext
{
    Q_OBJECT
public:
    IEditor(QObject *parent = 0) : IContext(parent) {}
    virtual ~IEditor() {}

    virtual bool createNew(const QString &contents = QString()) = 0;
    virtual bool open(const QString &fileName = QString()) = 0;
    virtual IFile *file() = 0;
    virtual const char *kind() const = 0;
    virtual QString displayName() const = 0;
    virtual void setDisplayName(const QString &title) = 0;

    virtual bool duplicateSupported() const = 0;
    virtual IEditor *duplicate(QWidget *parent) = 0;

    virtual QByteArray saveState() const = 0;
    virtual bool restoreState(const QByteArray &state) = 0;

    virtual int currentLine() const { return 0; };
    virtual int currentColumn() const { return 0; };

    virtual QToolBar *toolBar() = 0;

signals:
    void changed();
};

} // namespace Core

#endif //IEDITOR_H
