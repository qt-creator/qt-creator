let $prefix := string("QT_TRANSLATE_NOOP(&quot;MimeType&quot;, &quot;")
let $suffix := concat("&quot;)", codepoints-to-string(10))
for $file in tokenize($files, string("\|"))
    for $comment in doc($file)/*:mime-info/*:mime-type/*:comment
        return fn:concat($prefix, data($comment), $suffix)
