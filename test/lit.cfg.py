# SPDX-License-Identifier: GPL-3.0-or-later

import os

import lit.formats
from lit.llvm import llvm_config
from lit.llvm.subst import FindTool, ToolSubst

config.name = "loonglint"
config.test_format = lit.formats.ShTest()
config.suffixes = [".s", ".test"]
config.excludes = ["Inputs"]
config.test_source_root = os.path.dirname(__file__)
config.test_exec_root = os.path.join(config.loonglint_obj_root, "test")

llvm_config.use_default_substitutions()
tool_dirs = [config.llvm_tools_dir]
tool_dirs.extend(filter(None, config.environment.get("PATH", "").split(os.pathsep)))
llvm_config.add_tool_substitutions(
    [
        ToolSubst("loonglint", FindTool("loonglint"), unresolved="fatal"),
        ToolSubst("llvm-mc", FindTool("llvm-mc"), unresolved="fatal"),
        ToolSubst("llvm-objcopy", FindTool("llvm-objcopy"), unresolved="fatal"),
        ToolSubst("yaml2obj", FindTool("yaml2obj"), unresolved="fatal"),
        ToolSubst("ld.lld", FindTool("ld.lld"), unresolved="fatal"),
    ],
    tool_dirs,
)
