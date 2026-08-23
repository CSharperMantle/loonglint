# SPDX-License-Identifier: GPL-3.0-or-later

import os

import lit.formats
from lit.llvm import llvm_config
from lit.llvm.subst import FindTool, ToolSubst

config.name = "loonglint"
config.test_format = lit.formats.ShTest()
config.suffixes = [".test"]
config.excludes = ["Inputs"]
config.test_source_root = os.path.dirname(__file__)
config.test_exec_root = os.path.join(config.loonglint_obj_root, "test")

llvm_config.use_default_substitutions()
llvm_config.add_tool_substitutions(
    [ToolSubst("%loonglint", FindTool("loonglint"), unresolved="fatal")],
    [config.llvm_tools_dir],
)
