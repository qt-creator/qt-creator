# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

ZDOTDIR=$USER_ZDOTDIR
if [[ $options[norcs] = off && -o "login" &&  -f $ZDOTDIR/.zlogin ]]; then
	. $ZDOTDIR/.zlogin
fi
