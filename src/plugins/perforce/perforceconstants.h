/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef PERFORCE_CONSTANTS_H
#define PERFORCE_CONSTANTS_H

#include <QtCore/QtGlobal>

namespace Perforce {
namespace Constants {

const char * const PERFORCEEDITOR_CONTEXT  = "Perforce Editor";

const char * const PERFORCE_SUBMIT_EDITOR_ID  = "Perforce.SubmitEditor";
const char * const PERFORCE_SUBMIT_EDITOR_DISPLAY_NAME = QT_TRANSLATE_NOOP("VCS", "Perforce.SubmitEditor");
const char * const PERFORCESUBMITEDITOR_CONTEXT = "Perforce Submit Editor";

const char * const PERFORCE_COMMANDLOG_EDITOR_ID = "Perfoirce.CommandLogEditor";
const char * const PERFORCE_COMMANDLOG_EDITOR_DISPLAY_NAME = QT_TRANSLATE_NOOP("VCS", "Perforce CommandLog Editor");
const char * const PERFORCE_COMMANDLOG_EDITOR_CONTEXT = "Perforce Command Log Editor";

const char * const PERFORCE_LOG_EDITOR_ID = "Perforce.LogEditor";
const char * const PERFORCE_LOG_EDITOR_DISPLAY_NAME = QT_TRANSLATE_NOOP("VCS", "Perforce Log Editor");
const char * const PERFORCE_LOG_EDITOR_CONTEXT = "Perforce Log Editor";

const char * const PERFORCE_DIFF_EDITOR_ID = "Perforce.DiffEditor";
const char * const PERFORCE_DIFF_EDITOR_DISPLAY_NAME = QT_TRANSLATE_NOOP("VCS", "Perforce Diff Editor");
const char * const PERFORCE_DIFF_EDITOR_CONTEXT = "Perforce Diff Editor";

const char * const PERFORCE_ANNOTATION_EDITOR_ID = "Perforce.AnnotationEditor";
const char * const PERFORCE_ANNOTATION_EDITOR_DISPLAY_NAME = QT_TRANSLATE_NOOP("VCS", "Perforce Annotation Editor");
const char * const PERFORCE_ANNOTATION_EDITOR_CONTEXT = "Perforce Annotation Editor";

const char * const SUBMIT_CURRENT = "Perforce.SubmitCurrentLog";
const char * const DIFF_SELECTED = "Perforce.DiffSelectedFilesInLog";
const char * const SUBMIT_MIMETYPE = "application/vnd.nokia.text.p4.submit";

enum { debug = 0 };

} // Internal
} // Perforce

#endif // PERFORCE_CONSTANTS_H
