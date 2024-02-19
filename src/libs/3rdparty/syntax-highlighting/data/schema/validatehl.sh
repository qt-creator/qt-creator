#!/bin/sh
schemadir=$(dirname "$0")
xmllint --noout --schema "$schemadir"/language.xsd "$@"
