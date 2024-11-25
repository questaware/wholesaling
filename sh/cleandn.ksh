#!/bin/ksh
					# take out the typedefs
sed -e 's,typedef.* \([A-Z][A-Za-z0-9_]*;\),/typedef.*[* ]\1$/d,p;d' ../../h/base.h > /tmp/pjspp$$
sed -f /tmp/pjspp$$
#rm -f /tmp/pjspp$$
