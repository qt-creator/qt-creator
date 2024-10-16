T = require'TextEditor'
editor = T.currentEditor()
cursor = editor:cursor()
cursor:insertText('-- Hello World!')
