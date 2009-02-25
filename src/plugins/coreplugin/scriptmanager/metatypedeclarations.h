/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef METATYPEDECLARATIONS_H
#define METATYPEDECLARATIONS_H

#include <coreplugin/messagemanager.h>
#include <coreplugin/filemanager.h>
#include <coreplugin/ifile.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/editormanager/editorgroup.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>

#include <QtCore/QList>
#include <QtCore/QMetaType>

QT_BEGIN_NAMESPACE
class QMainWindow;
class QStatusBar;
class QToolBar;
class QSettings;
QT_END_NAMESPACE

Q_DECLARE_METATYPE(Core::MessageManager*)
Q_DECLARE_METATYPE(Core::FileManager*)
Q_DECLARE_METATYPE(Core::IFile*)
Q_DECLARE_METATYPE(QList<Core::IFile*>)
Q_DECLARE_METATYPE(Core::IEditor*)
Q_DECLARE_METATYPE(QList<Core::IEditor*>)
Q_DECLARE_METATYPE(Core::EditorGroup*)
Q_DECLARE_METATYPE(QList<Core::EditorGroup*>)
Q_DECLARE_METATYPE(Core::EditorManager*)
Q_DECLARE_METATYPE(Core::ICore*)

Q_DECLARE_METATYPE(QMainWindow*)
Q_DECLARE_METATYPE(QStatusBar*)
Q_DECLARE_METATYPE(QToolBar*)
Q_DECLARE_METATYPE(QSettings*)

#endif // METATYPEDECLARATIONS_H
