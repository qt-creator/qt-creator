s/#include "qscriptcontext_p.h"//g
s/#include "qscriptcontext.h"//g
s/#include "qscriptengine.h"//g
s/#include "qscriptmember_p.h"//g
s/#include "qscriptobject_p.h"//g
s/#include "qscriptvalueimpl_p.h"//g

s/#ifndef QT_NO_SCRIPT//g
s,#endif // QT_NO_SCRIPT,,g

s/QScript/JavaScript/g
s/QSCRIPT/JAVASCRIPT/g
s/qscript/javascript/g
s/Q_SCRIPT/J_SCRIPT/g

s/qsreal/qjsreal/g