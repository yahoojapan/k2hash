#
# K2HASH
#
# Copyright 2013 Yahoo Japan Corporation.
#
# K2HASH is key-valuew store base libraries.
# K2HASH is made for the purpose of the construction of
# original KVS system and the offer of the library.
# The characteristic is this KVS library which Key can
# layer. And can support multi-processing and multi-thread,
# and is provided safely as available KVS.
#
# For the full copyright and license information, please view
# the license file that was distributed with this source code.
#
# AUTHOR:   Takeshi Nakatani
# CREATE:   Fri Dec 2 2013
# REVISION:
#

SUBDIRS=lib docs tests buildutils

EXTRA_DIST=RELEASE_VERSION @CONFIGURECUSTOM@

.PHONY: cppcheck shellcheck

#
# CppCheck
#
# [NOTE]
# Versions of cppcheck below 1.8 perform poorly and the test is
# skipped if those versions are installed.
#
CPPCHECK_NG_VERSION		= -e \^0\\. -e \^1\\.[0-7]
CPPCHECK_CMD			= cppcheck
CPPCHECK_TARGET			= lib tests
CPPCHECK_BUILD_DIR		= /tmp/cppcheck
CPPCHECK_BASE_OPT		= --quiet \
						  --error-exitcode=1 \
						  --inline-suppr \
						  -j 4 \
						  --std=c++03 \
						  --xml
CPPCHECK_ENABLE_OPT		= --enable=warning,style,information,missingInclude
CPPCHECK_IGNORE_OPT		= --suppress=unmatchedSuppression

cppcheck:
	@if command -v $(CPPCHECK_CMD) >/dev/null 2>&1; then \
		if ($(CPPCHECK_CMD) --version | sed -e 's|Cppcheck[[:space:]]*||gi' | grep -q $(CPPCHECK_NG_VERSION)); then \
			echo "*** [INFO] cppcheck version below 1.8, so skip to run cppcheck."; \
		else \
			echo "*** Check all files with CppCheck"; \
			if [ -d $(CPPCHECK_BUILD_DIR) ]; then \
				rm -rf $(CPPCHECK_BUILD_DIR); \
			fi; \
			mkdir -p $(CPPCHECK_BUILD_DIR); \
			$(CPPCHECK_CMD) $(CPPCHECK_BASE_OPT) $(CPPCHECK_ENABLE_OPT) $(CPPCHECK_IGNORE_OPT) --cppcheck-build-dir=$(CPPCHECK_BUILD_DIR) $(CPPCHECK_TARGET); \
			rm -rf $(CPPCHECK_BUILD_DIR); \
		fi; \
	else \
		echo "*** [WARNING] cppcheck is not installed, so skip to run cppcheck."; \
	fi

#
# ShellCheck
#
# This make target executes shellchek on text files with the
# extension '*.sh' under the current directory(other than the
# '.git' directory) and executable files(text files other than
# the extension '*.sh').
#
SHELLCHECK_CMD				= shellcheck
SHELLCHECK_BASE_OPT			= --shell=sh
SHELLCHECK_IGN_OPT			= --exclude=SC1117,SC1090,SC1091
SHELLCHECK_INCLUDE_IGN_OPT	= --exclude=SC1117,SC1090,SC1091,SC2034,SC2148
SHELLCHECK_FILES_NO_SH		= `grep -ril '^\#!/bin/sh' . | grep -v '\.sh' | grep -v '\.log' | grep -v '/\.git/' | grep -v '/rpmbuild/' | grep -v '/debian_build/' | grep -v '/autom4te.cache/' | grep -v '/m4/' | grep -v '/install-sh' | grep -v '/ltmain.sh' | tr '\n' ' '`
SHELLCHECK_FILES_SH			= `grep -ril '^\#!/bin/sh' . | grep '\.sh'    | grep -v '\.log' | grep -v '/\.git/' | grep -v '/rpmbuild/' | grep -v '/debian_build/' | grep -v '/autom4te.cache/' | grep -v '/m4/' | grep -v '/install-sh' | grep -v '/ltmain.sh' | tr '\n' ' '`
SHELLCHECK_FILES_INCLUDE_SH	= `grep -Lir '^\#!/bin/sh' . | grep '\.sh'    | grep -v '\.log' | grep -v '/\.git/' | grep -v '/rpmbuild/' | grep -v '/debian_build/' | grep -v '/autom4te.cache/' | grep -v '/m4/' | grep -v '/install-sh' | grep -v '/ltmain.sh' | tr '\n' ' '`

shellcheck:
	@if type shellcheck > /dev/null 2>&1; then \
		echo "*** Check all files with ShellCheck"; \
		if [ -n "$(SHELLCHECK_FILES_NO_SH)" ]; then \
			LC_ALL=C.UTF-8 $(SHELLCHECK_CMD) $(SHELLCHECK_BASE_OPT) $(SHELLCHECK_IGN_OPT) $(SHELLCHECK_FILES_NO_SH) || exit 1; \
		fi; \
		if [ -n "$(SHELLCHECK_FILES_SH)" ]; then \
			LC_ALL=C.UTF-8 $(SHELLCHECK_CMD) $(SHELLCHECK_BASE_OPT) $(SHELLCHECK_IGN_OPT) $(SHELLCHECK_FILES_SH) || exit 1; \
		fi; \
		if [ -n "$(SHELLCHECK_FILES_INCLUDE_SH)" ]; then \
			LC_ALL=C.UTF-8 $(SHELLCHECK_CMD) $(SHELLCHECK_BASE_OPT) $(SHELLCHECK_INCLUDE_IGN_OPT) $(SHELLCHECK_FILES_INCLUDE_SH) || exit 1; \
		fi; \
		echo "   -> No error was detected."; \
		echo ""; \
	else \
		echo "ShellCheck is not installed, so skip check by it."; \
		echo ""; \
	fi

#
# Local variables:
# tab-width: 4
# c-basic-offset: 4
# End:
# vim600: noexpandtab sw=4 ts=4 fdm=marker
# vim<600: noexpandtab sw=4 ts=4
#
