#!/bin/sh
#
# Utility helper tools for Github Actions by AntPickax
#
# Copyright 2018 Yahoo Japan Corporation.
#
# AntPickax provides utility tools for supporting autotools
# builds.
#
# These tools retrieve the necessary information from the
# repository and appropriately set the setting values of
# configure, Makefile, spec,etc file and so on.
# These tools were recreated to reduce the number of fixes and
# reduce the workload of developers when there is a change in
# the project configuration.
# 
# For the full copyright and license information, please view
# the license file that was distributed with this source code.
#
# AUTHOR:   Takeshi Nakatani
# CREATE:   Fri, Apr 13 2018
# REVISION:
#

#==============================================================
# Autotools
#==============================================================
#
# Instead of pipefail(for shells not support "set -o pipefail")
#
PIPEFAILURE_FILE="/tmp/.pipefailure.$(od -An -tu4 -N4 /dev/random | tr -d ' \n')"

#
# Common variables
#
AUTOGEN_NAME=$(basename "${0}")
AUTOGEN_DIR=$(dirname "${0}")
SRCTOP=$(cd "${AUTOGEN_DIR}" || exit 1; pwd)

#
# Start to run
#
echo "[RUN] autogen.sh"
echo ""

#
# Parse parameters
#
UPDATE_VERSION_FILE=1
FORCEPARAM="--force"
PARAMETERS=""
while [ $# -ne 0 ]; do
	if [ -z "$1" ]; then
		echo "[ERROR] Parameters are something wrong."
		exit 1

	elif echo "$1" | grep -q -i -e "^-h$" -e "^--help$"; then
		echo "Usage: ${AUTOGEN_NAME} [--help(-h)] [--no_update_version_file(-nu)] [--no_aclocal_force(-na)] [--no_check_ver_diff(-nc)]"
		exit 0

	elif echo "$1" | grep -q -i -e "^-nu$" -e "^--no_update_version_file$"; then
		UPDATE_VERSION_FILE=0
		FORCEPARAM=""			# do not need force

	elif echo "$1" | grep -q -i -e "^-na$" -e "^--no_aclocal_force$"; then
		FORCEPARAM=""

	elif echo "$1" | grep -q -i -e "^-nc$" -e "^--no_check_ver_diff$"; then
		PARAMETERS="$1"

	else
		echo "[ERROR] Unknown option $1"
		echo "Usage: ${AUTOGEN_NAME} [--help(-h)] [--no_update_version_file(-nu)] [--no_aclocal_force(-na)] [--no_check_ver_diff(-nc)]"
		exit 1
	fi
	shift
done

#
# update RELEASE_VERSION file
#
if [ "${UPDATE_VERSION_FILE}" -eq 1 ] && [ -f "${SRCTOP}/buildutils/make_release_version_file.sh" ]; then
	echo "    [INFO] run make_release_version_file.sh"
	if ({ /bin/sh -c "${SRCTOP}/buildutils/make_release_version_file.sh" "${PARAMETERS}" 2>&1 || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's|^|        |g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
		echo "[ERROR] update RELEASE_VERSION file"
		exit 1
	fi
fi

#
# Check files
#
if [ ! -f "${SRCTOP}/NEWS" ]; then
	touch "${SRCTOP}/NEWS"
fi
if [ ! -f "${SRCTOP}/README" ]; then
	touch "${SRCTOP}/README"
fi
if [ ! -f "${SRCTOP}/AUTHORS" ]; then
	touch "${SRCTOP}/AUTHORS"
fi
if [ ! -f "${SRCTOP}/ChangeLog" ]; then
	touch "${SRCTOP}/ChangeLog"
fi

#
# Auto scan
#
if [ ! -f configure.scan ] || [ -n "${FORCEPARAM}" ]; then
	echo "    [INFO] run autoscan"
	if ({ autoscan 2>&1 || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's|^|        |g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
		echo "[ERROR] something error occurred in autoscan"
		exit 1
	fi
fi

#
# Copy libtools
#
if grep -q 'LT_INIT' configure.ac configure.scan; then
	if ({ libtoolize --force --copy 2>&1 || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's|^|        |g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
		echo "[ERROR] something error occurred in libtoolize"
		exit 1
	fi
fi

#
# Build configure and Makefile
#
echo "    [INFO] run aclocal ${FORCEPARAM}"
if ({ /bin/sh -c "aclocal ${FORCEPARAM}" 2>&1 || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's|^|        |g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "[ERROR] something error occurred in aclocal ${FORCEPARAM}"
	exit 1
fi

if grep -q 'AC_CONFIG_HEADERS' configure.ac configure.scan; then
	echo "    [INFO] run autoheader"
	if ({ autoheader 2>&1 || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's|^|        |g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
		echo "[ERROR] something error occurred in autoheader"
		exit 1
	fi
fi

echo "    [INFO] run automake -c --add-missing"
if ({ automake -c --add-missing 2>&1 || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's|^|        |g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "[ERROR] something error occurred in automake -c --add-missing"
	exit 1
fi

echo "    [INFO] run autoconf"
if ({ autoconf 2>&1 || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's|^|        |g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "[ERROR] something error occurred in autoconf"
	exit 1
fi

#
# Finish
#
echo ""
echo "[SUCCEED] autogen.sh"
exit 0

#
# Local variables:
# tab-width: 4
# c-basic-offset: 4
# End:
# vim600: noexpandtab sw=4 ts=4 fdm=marker
# vim<600: noexpandtab sw=4 ts=4
#
