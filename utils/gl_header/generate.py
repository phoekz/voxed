import argparse
import os
import re
import glcorearb
import wishlist

from os import path
from pprint import pprint

#
# argument parsing
#

parser = argparse.ArgumentParser(description="Generates OpenGL Header-file for C")
parser.add_argument("--output", help="Output File", required=True)
args = parser.parse_args()

#
# organize source lines for the lexer
#

gl_version_groups_raw = re.findall("(?s)(?<=#ifndef GL_VERSION_\d_\d\n).*?(?=\n#endif /\* GL_VERSION_\d_\d \*/)", glcorearb.file)
gl_version_groups = {}

for group in gl_version_groups_raw:
    lines = group.split("\n")
    gl_version = tuple(int(x) for x in re.findall("\d_\d", lines[0])[0].split("_"))
    gl_version_groups[gl_version] = lines

gl_version_groups = { k: v for k, v in gl_version_groups.items() if 10 * k[0] + k[1] < 45 }
for k, v in gl_version_groups.items():
    lines = list(filter(lambda x: not re.match("#define GL_VERSION_\d_\d", x), v))
    defines = list(filter(lambda x: re.match("#define \w+[ ]+\w+", x), lines))
    functions = list(filter(lambda x: re.match("GLAPI", x), lines))
    gl_version_groups[k] = {"version": k, "defines":defines, "functions":functions}

#
# lexer
#

type_table = {
    "GLbitfield": "vx::u32",
    "GLboolean": "vx::u8",
    "GLbyte": "char",
    "GLchar": "char",
    "GLdouble": "double",
    "GLenum": "vx::u32",
    "GLfloat": "float",
    "GLint": "vx::i32",
    "GLint64": "vx::i64",
    "GLintptr": "vx::iptr",
    "GLshort": "vx::i64",
    "GLsizei": "vx::i32",
    "GLsizeiptr": "vx::iptr",
    "GLsync": "void",
    "GLubyte": "char",
    "GLuint": "vx::u32",
    "GLuint64": "vx::u64",
    "GLushort": "vx::u16",
    "void": "void",
    "GLDEBUGPROC": "void",
}

def lex_type(line):
    return {"type":type_table[line.split(" ")[0].strip()], "ptr":line.count("*")}
def lex_arg(line):
    lexed = lex_type(line)
    lexed["id"] = line.replace("*", "").split(" ")[-1:][0]
    return lexed
def lex_fn_id(line):
    return re.findall("gl[\w]+", line)[0].strip()
def lex_fn_ret(line):
    return re.findall("(.+)(?=gl)", line)[0].replace("const", "").strip()
def lex_fn_sig(line):
    line = re.findall("\(.+\)", line)[0].lstrip("(").rstrip(")").strip().split(",")
    line = list(map(lambda x: x.replace("const", "").strip(), line))
    return list(map(lex_arg, line))
def lex_fn(line):
    return {"ret":lex_type(lex_fn_ret(line)), "id":lex_fn_id(line), "sig":lex_fn_sig(line)}

for k, v in gl_version_groups.items():
    v["defines"] = [x.split()[1:] for x in v["defines"]]
    v["defines"] = [{"id":x[0], "val":int(x[-1].rstrip("u").strip("ull"), 16)} for x in v["defines"]]
    v["functions"] = [x.replace("GLAPI", "").replace("APIENTRY", "").rstrip(";") for x in v["functions"]]
    v["functions"] = list(map(lex_fn, v["functions"]))

#
# codegen
#

final_defs = []
final_funcs = []
for _, v in gl_version_groups.items():
    for d in v["defines"]:
        if d["id"] in wishlist.enums:
            final_defs.append(d)
    for f in v["functions"]:
        if f["id"] in wishlist.functions:
            final_funcs.append(f)

with open(args.output, "w") as file:
    class pr():
        def __init__(self):
            self.buf = []
        def __call__(self, s = "", w = 0):
            self.buf.append(w * " " + s)
        def str(self):
            return "\n".join(self.buf)
    def strtype(t):
        return t["type"] + t["ptr"] * "*"
    def strsig(s, idents=False):
        fn = (lambda x: "{} {}".format(strtype(x), x["id"])) if idents else lambda x: strtype(x)
        return ", ".join(list(map(fn, s))).replace("void void", "void")
    p = pr()
    p("#pragma once")
    p()
    p("#include \"common/aliases.h\"")
    p()
    p("// clang-format off")
    p()
    list(map(lambda d: p("#define {id} 0x{val:08x}u".format(id=d["id"], val=d["val"])), final_defs))
    p()
    list(map(lambda f: p("extern {ret}(*{id})({sig});".format(ret=strtype(f["ret"]), id=f["id"], sig=strsig(f["sig"], True))), final_funcs))
    p()
    p("extern void vx_gl_init(void *(*addr)(const char *));")
    p()
    p("#ifdef VX_GL_IMPLEMENTATION")
    p()
    list(map(lambda f: p("{ret}(*{id})({sig});".format(ret=strtype(f["ret"]), id=f["id"], sig=strsig(f["sig"], True))), final_funcs))
    p()
    p("void vx_gl_init(void *(*addr)(const char *))")
    p("{")
    list(map(lambda f: p("{id} = ({ret}(*)({sig}))addr(\"{id}\");".format(id=f["id"], ret=strtype(f["ret"]), sig=strsig(f["sig"])), 4), final_funcs))
    p("}")
    p()
    p("#endif // VX_GL_IMPLEMENTATION")
    p()
    p("// clang-format on")
    p()

    file.write(p.str())
