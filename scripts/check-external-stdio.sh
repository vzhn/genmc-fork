#!/bin/bash

# Checks that GenMC rejects the stdio functions it cannot run soundly
#
# This program is dual-licensed under the Apache License 2.0 and the MIT License.
# You may choose to use, distribute, or modify this software under either license.
#
# Apache License 2.0:
#     http://www.apache.org/licenses/LICENSE-2.0
#
# MIT License:
#     https://opensource.org/licenses/MIT

# sprintf(), sscanf() and scanf() would write to the program's memory
# behind the interpreter (the address of a local is not a host address):
# sscanf() crashed, and sprintf() into a global left it unwritten, so a
# correct program failed its assertions. GenMC's <stdio.h> does not
# declare them; a program that does is rejected as calling an unknown
# external function (a user error, status 17). printf() still prints its
# formatted arguments. ctest target "external-stdio".
#
#   GenMC=<build>/bin/genmc scripts/check-external-stdio.sh

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
GenMC="${GenMC:-$DIR/../RelWithDebInfo/bin/genmc}"
work=$(mktemp -d)
trap 'rm -rf "${work}"' EXIT

status=0
rejects() { # <function> <declaration> <call>
	printf '#include <stdio.h>\nextern int %s(%s);\nchar buf[8];\nint a;\nint main() { return %s; }\n' \
		"$1" "$2" "$3" > "${work}/$1.c"
	out=$("${GenMC}" -rc11 -disable-estimation -- "${work}/$1.c" < /dev/null 2>&1)
	rc=$?
	if [[ ${rc} -eq 17 ]] && echo "${out}" | grep -q "unknown external function: $1"; then
		echo "ok   $1 rejected"
	else
		echo "FAIL $1: status ${rc}, expected 17 and \"unknown external function: $1\""
		echo "${out}" | tail -3; status=1
	fi
}
rejects sprintf 'char *, const char *, ...' 'sprintf(buf, "%d", 7)'
rejects sscanf 'const char *, const char *, ...' 'sscanf("12", "%d", &a)'
rejects scanf 'const char *, ...' 'scanf("%d", &a)'

printf '#include <stdio.h>\nint main() { printf("%%d-%%s-%%u\\n", 7, "x", 3u); return 0; }\n' > "${work}/printf.c"
out=$("${GenMC}" -rc11 -disable-estimation -- "${work}/printf.c" < /dev/null 2>&1)
rc=$?
if [[ ${rc} -eq 0 ]] && echo "${out}" | grep -qx '7-x-3'; then
	echo "ok   printf prints"
else
	echo "FAIL printf: status ${rc}, expected 0 and the line 7-x-3"; echo "${out}" | tail -3; status=1
fi
exit ${status}
