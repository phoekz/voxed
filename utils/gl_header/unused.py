import argparse
import glob
import os
import re
import wishlist

from os import path
from pprint import pprint

#
# argument parsing
#

parser = argparse.ArgumentParser(description="Finds unused OpenGL functions and definitions")
parser.add_argument("-d", "--directory", help="Source directory", required=True)
args = parser.parse_args()

#
# source files
#

wanted_files = [
    "sdl_gl_platform.cpp",
]
all_files = glob.glob(path.join(args.directory, "**"), recursive=True)
matched_files = filter(lambda x: path.basename(x) in wanted_files, all_files)

#
# find unused definitions
#

matched_enums = {key: 0 for key in wishlist.enums}
matched_funcs = {key: 0 for key in wishlist.functions}

for file_path in matched_files:
    print(file_path)
    with open(file_path, "r") as file:
        for line in file.read().split("\n"):
            for enum in re.findall("\\bGL_[A-Z0-9_]+", line):
                if enum in matched_enums:
                    matched_enums[enum] += 1
            for func in re.findall("\\bgl[A-Za-z0-9]+", line):
                if func in matched_funcs:
                    matched_funcs[func] += 1

pprint(list(filter(lambda x: x[1] == 0, list(matched_enums.items()))))
pprint(list(filter(lambda x: x[1] == 0, list(matched_funcs.items()))))
