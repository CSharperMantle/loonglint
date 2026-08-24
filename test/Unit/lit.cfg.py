# SPDX-License-Identifier: GPL-3.0-or-later

import os

import lit.formats

config.name = "loonglint-Unit"
config.suffixes = []
config.test_source_root = os.path.join(config.loonglint_obj_root, "unittests")
config.test_exec_root = config.test_source_root
config.test_format = lit.formats.GoogleTest(config.llvm_build_mode, "Tests")
