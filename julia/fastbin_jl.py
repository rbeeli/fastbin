from itertools import chain
import json
import os
import sys
import shutil
from pathlib import Path
from typing import Dict, List, Optional
from dataclasses import dataclass

prefix = "_"  # prefix for internally generated functions


@dataclass
class TypeDef:
    # e = Enum, s = Struct, c = Container/Vector, p = Primitive, v = Variant
    category: str
    name: str
    lang_type: str
    include_stmt: str
    native_size: int
    aligned_size: int
    variable_length: bool
    use_print: bool
    element_type_def: Optional["TypeDef"] = None
    # elements_type_defs: Optional[List["TypeDef"]] = None  # TODO


@dataclass
class StructMemberDef:
    name: str
    type_def: TypeDef


@dataclass
class StructDef:
    name: str
    type_def: TypeDef
    members: Dict[str, StructMemberDef]
    docstring: str


@dataclass
class EnumMemberDef:
    name: str
    value: int
    # map: List[str]  # TODO
    docstring: List[str]


@dataclass
class EnumDef:
    name: str
    type_def: TypeDef
    storage_type_def: TypeDef
    # generate_parse: bool  # TODO
    members: Dict[str, EnumMemberDef]
    docstring: str


class GenContext:
    namespace: str
    enums: Dict[str, EnumDef]
    structs: Dict[str, StructDef]
    enum_types: Dict[str, TypeDef]
    struct_types: Dict[str, TypeDef]
    built_in_types: Dict[str, TypeDef]

    def __init__(self, schema: dict):
        self.enums = {}
        self.structs = {}
        self.enum_types = {}
        self.struct_types = {}
        self.built_in_types = {
            "int8": TypeDef("p", "int8", "Int8", "", 1, 8, False, True),
            "int16": TypeDef("p", "int16", "Int16", "", 2, 8, False, True),
            "int32": TypeDef("p", "int32", "Int32", "", 4, 8, False, True),
            "int64": TypeDef("p", "int64", "Int64", "", 8, 8, False, True),
            "uint8": TypeDef("p", "uint8", "UInt8", "", 1, 8, False, True),
            "uint16": TypeDef("p", "uint16", "UInt16", "", 2, 8, False, True),
            "uint32": TypeDef("p", "uint32", "UInt32", "", 4, 8, False, True),
            "uint64": TypeDef("p", "uint64", "UInt64", "", 8, 8, False, True),
            "byte": TypeDef("p", "byte", "UInt8", "", 1, 8, False, True),
            "char": TypeDef("p", "char", "UInt8", "", 1, 8, False, True),
            "float32": TypeDef("p", "float32", "Float32", "", 4, 8, False, True),
            "float64": TypeDef("p", "float64", "Float64", "", 8, 8, False, True),
            "bool": TypeDef("p", "bool", "Bool", "", 1, 8, False, True),
        }
        self.built_in_types["string"] = TypeDef(
            "c",
            "string",
            "StringView",
            "using StringViews",
            -1,
            -1,
            True,
            True,
            self.built_in_types["char"],
        )

        # parse namespace
        self.namespace = schema.get("namespace", "")
        if self.namespace == "":
            raise ValueError("No namespace defined in schema")

        # parse enums
        for enum_name, enum_content in schema.get("enums", {}).items():
            if "type" not in enum_content:
                raise ValueError(f"Enum '{enum_name}' does not have 'type' defined")

            storage_type = self.get_type_def(enum_content["type"])
            type_def = TypeDef(
                "e",
                enum_name,
                f"{enum_name}.T",
                "",
                storage_type.native_size,
                storage_type.aligned_size,
                False,
                True,
            )
            self.enum_types[enum_name] = type_def

            # parse docstring
            docstring = parse_docstring(enum_content.get("docstring", None))

            # parse enum members
            members = {}
            for member_name, member_content in enum_content.get("members", {}).items():
                value = member_content["value"]
                # TODO: Support enum map
                # map = member_content.get("map", member_name)
                # if not isinstance(map, list):
                #     map = [map]
                member_docstring = parse_docstring(
                    member_content.get("docstring", None)
                )
                members[member_name] = EnumMemberDef(
                    member_name, value, member_docstring
                )

            # ensure that enum members are unique
            if len(members) != len(set(members.keys())):
                raise ValueError(f"Enum '{enum_name}' has duplicate members")

            # ensure that enum values are unique
            if len(set([m.value for m in members.values()])) != len(members):
                raise ValueError(f"Enum '{enum_name}' has duplicate values")

            # TODO: Support enum maps
            # # ensure that enum maps are unique
            # all_maps = list(chain.from_iterable(m.map for m in members.values()))
            # if len(set(all_maps)) != len(all_maps):
            #     raise ValueError(f"Enum '{enum_name}' has duplicate map entries")

            # generate_parse = enum_content.get("generate_parse", False)  # TODO
            self.enums[enum_name] = EnumDef(
                enum_name, type_def, storage_type, members, docstring
            )

        # parse structs
        for struct_name, struct_content in schema.get("structs", {}).items():
            # parse docstring
            docstring = parse_docstring(struct_content.get("docstring", None))

            # parse struct members
            members: Dict[str, StructMemberDef] = {}
            for member_name, member_type in struct_content.get("members", {}).items():
                type_def = self.get_type_def(member_type)
                members[member_name] = StructMemberDef(member_name, type_def)

            # calculate total size of struct (aligned)
            variable_length = any(m.type_def.variable_length for m in members.values())
            size = -1
            if not variable_length:
                size = sum(x.type_def.aligned_size for x in members.values())
            type_def = TypeDef(
                "s",
                struct_name,
                struct_name,
                "",
                size,
                size,
                variable_length,
                False,
            )
            self.structs[struct_name] = StructDef(
                struct_name, type_def, members, docstring
            )
            self.struct_types[struct_name] = type_def

    def get_struct_def(self, name: str) -> StructDef:
        if name not in self.structs:
            raise ValueError(f"Struct '{name}' not found or used before declaration")
        return self.structs[name]

    def get_enum_def(self, name: str) -> EnumDef:
        if name not in self.enums:
            raise ValueError(f"Enum '{name}' not found")
        return self.enums[name]

    def get_type_def(self, type_name: str) -> TypeDef:
        # primitive types (int8, double, etc.)
        if type_name in self.built_in_types:
            return self.built_in_types[type_name]

        # enum:MyEnum
        if type_name.startswith("enum:"):
            enum_type_name = type_name.split(":")[1]
            if enum_type_name not in self.enum_types:
                raise ValueError(f"Enum '{enum_type_name}' not found")
            return self.enum_types[enum_type_name]

        # struct:MyStruct
        if type_name.startswith("struct:"):
            struct_type_name = type_name.split(":")[1]
            if struct_type_name not in self.struct_types:
                raise ValueError(f"Struct '{struct_type_name}' not found")
            return self.struct_types[struct_type_name]

        # vector<T> -> Vector{T}
        if type_name.startswith("vector<"):
            el_type = type_name.split("<")[1].split(">")[0]
            el_type_def = self.get_type_def(el_type)
            if el_type_def.variable_length:
                # TODO Support variable length vectors?
                raise ValueError(f"Vector element type '{el_type}' must be fixed size")
            return TypeDef(
                "c",
                type_name,
                f"Vector{{{self.get_type_def(el_type).lang_type}}}",
                "",
                -1,
                -1,
                True,
                False,
                el_type_def,
            )

        raise ValueError(f"Unknown type: {type_name}")


def parse_docstring(docstring: str | List[str] | None):
    if docstring is None or docstring == "" or docstring == []:
        return None
    if isinstance(docstring, str):
        return [docstring]
    return docstring


def generate_docstring(docstring: List[str], indent: int):
    indent_str = " " * indent
    code = ""
    code += indent_str + '"""\n'
    for line in docstring:
        code += f"{indent_str}{line}\n"
    code += indent_str + '"""\n'
    return code


def generate_enum(ctx: GenContext, enum_def: EnumDef):
    print(f"Generating enum {ctx.namespace}::{enum_def.name}")

    code_body = ""
    for i, (name, member_def) in enumerate(enum_def.members.items()):
        # docstring
        if member_def.docstring:
            if i > 0:
                code_body += "\n"
            code_body += generate_docstring(member_def.docstring, 4)

        # member
        code_body += f"    {name} = {member_def.value}\n"

    code = "using EnumX\n"
    code += "\n"
    if enum_def.docstring:
        code += generate_docstring(enum_def.docstring, 0)
    code += f"@enumx {enum_def.name}::{enum_def.storage_type_def.lang_type} begin\n"
    code += code_body
    code += "end\n"
    code += "\n"
    code += f"function from_string(::Type{{{enum_def.name}.T}}, str::T) where T <: AbstractString\n"
    for i, (name, member_def) in enumerate(enum_def.members.items()):
        code += f'    str == "{name}" && return {enum_def.name}.{name}\n'
    code += f'    throw(ArgumentError("Invalid string value for enum {ctx.namespace}.{enum_def.name}: $str"))\n'
    code += "end\n"

    return code


def generate_get_member_body(ctx: GenContext, member_def: StructMemberDef):
    type_def = member_def.type_def
    lang_type = type_def.lang_type

    if type_def.category in ["e", "p"]:
        # primitive, enum
        code = f"    return unsafe_load(reinterpret(Ptr{{{lang_type}}}, obj.buffer + {prefix}{member_def.name}_offset(obj)))\n"
    elif type_def.category == "s":
        # struct
        code = f"    ptr::Ptr{{UInt8}} = obj.buffer + {prefix}{member_def.name}_offset(obj)\n"
        code += f"    return {lang_type}(ptr, {prefix}{member_def.name}_size_aligned(obj), false)\n"
    elif type_def.category == "c":
        # container with variable length (string, vector<T>)
        el_type_def = type_def.element_type_def
        el_type_jl = el_type_def.lang_type
        code = f"    ptr::Ptr{{{el_type_jl}}} = reinterpret(Ptr{{{el_type_jl}}}, obj.buffer + {prefix}{member_def.name}_offset(obj))\n"
        code += f"    unaligned_size::UInt64 = {prefix}{member_def.name}_size_unaligned(obj)\n"
        code += "    n_bytes::UInt64 = unaligned_size - 8\n"  # -8 to skip the size
        if el_type_def.native_size == 1:
            code += "    count::UInt64 = n_bytes\n"
        elif el_type_def.native_size == 2:
            # >> 1 is equal to dividing by 2
            code += "    count::UInt64 = n_bytes >> 1\n"
        elif el_type_def.native_size == 4:
            # >> 2 is equal to dividing by 4
            code += "    count::UInt64 = n_bytes >> 2\n"
        elif el_type_def.native_size == 8:
            # >> 3 is equal to dividing by 8
            code += "    count::UInt64 = n_bytes >> 3\n"
        else:
            code += f"    count::UInt64 = n_bytes / {el_type_def.native_size}\n"
        if type_def.name == "string":
            # StringView
            code += "    return StringView(unsafe_wrap(Vector{UInt8}, ptr + 8, count, own=false))\n"  # +8 to skip the size
        else:
            # Vector{T}
            code += f"    return unsafe_wrap(Vector{{{el_type_def.lang_type}}}, ptr + 8, count, own=false)\n"  # +8 to skip the size
    else:
        raise ValueError(f"Unknown type category: {type_def.category}")

    return code


def generate_set_member_body(ctx: GenContext, member_def: StructMemberDef):
    # NOTE: Must match implementation in `generate_calc_size_aligned_member_body`
    type_def = member_def.type_def
    lang_type = type_def.lang_type

    if type_def.category in ["p", "e"]:
        # primitive, enum
        code = f"    unsafe_store!(reinterpret(Ptr{{{lang_type}}}, obj.buffer + {prefix}{member_def.name}_offset(obj)), value)\n"
    elif type_def.category == "c":
        # container with variable length (string, vector<T>) and fixed element size
        el_type_def = member_def.type_def.element_type_def
        el_size_bytes = el_type_def.native_size
        code = f"    offset::UInt64 = {prefix}{member_def.name}_offset(obj)\n"
        code += f"    contents_size::UInt64 = length(value) * {el_size_bytes}\n"
        maybe_unaligned = el_type_def.native_size % 8 != 0
        if maybe_unaligned:
            # string or vector<T> with size of T not divisible of 8
            code += "    unaligned_size::UInt64 = 8 + contents_size\n"
            code += "    aligned_size::UInt64 = (unaligned_size + 7) & ~7\n"
            code += "    aligned_diff::UInt64 = aligned_size - unaligned_size\n"
            # add diff to high-bits of aligned_size
            code += (
                "    aligned_size_high::UInt64 = aligned_size | (aligned_diff << 56)\n"
            )
            code += "    unsafe_store!(reinterpret(Ptr{UInt64}, obj.buffer + offset), aligned_size_high)\n"
        else:
            # element size is divisible by 8, no alignment adjustment needed
            code += "    unsafe_store!(reinterpret(Ptr{UInt64}, obj.buffer + offset), 8 + contents_size)\n"
        code += "    dest_ptr::Ptr{UInt8} = obj.buffer + offset + 8\n"
        code += "    src_ptr::Ptr{UInt8} = reinterpret(Ptr{UInt8}, pointer(value))\n"
        code += "    unsafe_copyto!(dest_ptr, src_ptr, contents_size)\n"
    elif type_def.category == "s":
        # struct
        code = f'    @assert fastbin_binary_size(value) > 0 "Cannot set member `{member_def.name}`, parameter struct of type `{lang_type}` not finalized. Call fastbin_finalize!(obj) on struct after creation."\n'
        code += f"    offset::UInt64 = {prefix}{member_def.name}_offset(obj)\n"
        code += "    size::UInt64 = fastbin_binary_size(value)\n"
        code += "    unsafe_copyto!(obj.buffer + offset, value.buffer, size)\n"
    else:
        raise ValueError(f"Unknown type category: {type_def.category}")

    return code


def generate_size_member_body(
    ctx: GenContext, member_def: StructMemberDef, unaligned_size: bool
):
    type_def = member_def.type_def

    if type_def.category in ["p", "e"]:
        # primitives, enum
        code = f"    return {type_def.aligned_size}\n"
    elif type_def.category == "c":
        # container with variable length (string, vector<T>)
        el_type_def = type_def.element_type_def
        code = f"    stored_size::UInt64 = unsafe_load(reinterpret(Ptr{{UInt64}}, obj.buffer + {prefix}{member_def.name}_offset(obj)))\n"
        maybe_unaligned = el_type_def.native_size % 8 != 0
        if maybe_unaligned:
            # string or vector<T> with size of T not divisible of 8
            if unaligned_size:
                code += "    aligned_diff::UInt64 = stored_size >> 56\n"
                code += "    aligned_size::UInt64 = stored_size & 0x00FFFFFFFFFFFFFF\n"  # remove 8 high-bits
                code += "    return aligned_size - aligned_diff\n"
            else:
                code += "    aligned_size::UInt64 = stored_size & 0x00FFFFFFFFFFFFFF\n"  # remove 8 high-bits
                code += "    return aligned_size\n"
        else:
            # element size is divisible by 8, no alignment adjustment needed
            code += "    return stored_size\n"
    elif type_def.category == "s":
        # structs are always aligned to 8 bytes
        if type_def.variable_length:
            code = f"    return unsafe_load(reinterpret(Ptr{{UInt64}}, obj.buffer + {prefix}{member_def.name}_offset(obj)))\n"
        else:
            code = f"    return {type_def.aligned_size}\n"
    else:
        raise ValueError(f"Unknown type category: {type_def.category}")

    return code


def generate_calc_size_aligned_member_body(
    ctx: GenContext,
    struct_def: StructDef,
    member_def: StructMemberDef,
):
    # NOTE: Must match implementation in `generate_size_member_body`
    type_def = member_def.type_def
    # lang_type = type_def.lang_type

    if type_def.category in ["p", "e"]:
        # primitive, enum
        code = f"    return {type_def.aligned_size}\n"
    elif type_def.category == "c":
        # container with variable length (string, vector<T>) and fixed element size
        el_type_def = member_def.type_def.element_type_def
        el_size_bytes = el_type_def.native_size
        code = f"    contents_size::UInt64 = length(value) * {el_size_bytes}\n"
        maybe_unaligned = el_type_def.native_size % 8 != 0
        if maybe_unaligned:
            # string or vector<T> with size of T not divisible by 8
            code += "    unaligned_size::UInt64 = 8 + contents_size\n"
            code += "    return (unaligned_size + 7) & ~7\n"
        else:
            # element size is divisible by 8, no alignment adjustment needed
            code += "    return 8 + contents_size\n"
    elif type_def.category == "s":
        # struct
        code = "    return fastbin_binary_size(value)\n"
    else:
        raise ValueError(f"Unknown type category: {type_def.category}")

    return code


def generate_offset_member_body(
    ctx: GenContext,
    index: int,
    struct_def: StructDef,
    member_def: StructMemberDef,
):
    member_names = list(struct_def.members.keys())
    if struct_def.type_def.variable_length:
        # first 8 bytes are reserved for the size of the variable-length struct
        offset = 8
    else:
        # fixed-length struct
        offset = 0
    for i, prev_member in enumerate(struct_def.members.values()):
        if i < index:
            if prev_member.type_def.variable_length:
                # cannot precompute fixed offset, variable length member found
                offset = -1
                break
            else:
                # fixed-length members only take up their aligned size space
                offset += prev_member.type_def.aligned_size
    if offset == -1:
        # variable length member found, cannot precompute offset
        code_body = f"    return {prefix}{member_names[index - 1]}_offset(obj) + {prefix}{member_names[index - 1]}_size_aligned(obj)\n"
    else:
        code_body = f"    return {offset}\n"
    return code_body


def generate_struct(ctx: GenContext, struct_def: StructDef):
    print(f"Generating struct {ctx.namespace}::{struct_def.name}", end=" ")
    if struct_def.type_def.variable_length:
        print("[size=variable]")
    else:
        print(f"[size={struct_def.type_def.aligned_size} bytes]")
    for name, member_def in struct_def.members.items():
        print(f"- {name}: {member_def.type_def.lang_type}")

    member_names = list(name for name, _ in struct_def.members.items())

    if len(member_names) == 0:
        raise ValueError(
            f"struct {ctx.namespace}::{struct_def.name} does not have any members"
        )

    # generate member functions (get, set, size, offset)
    code_body = ""
    for i, (name, member_def) in enumerate(struct_def.members.items()):
        code_body += f"\n# Member: {name}::{member_def.type_def.lang_type}\n"
        type_def = member_def.type_def

        # getter
        code_body += (
            f"\n@inline function {name}(obj::{struct_def.name})::{type_def.lang_type}\n"
        )
        code_body += generate_get_member_body(ctx, member_def)
        code_body += "end\n"
        code_body += "\n"

        # setter
        if type_def.lang_type == "StringView":
            code_body += f"@inline function {name}!(obj::{struct_def.name}, value::T) where {{T<:AbstractString}}\n"
        else:
            code_body += f"@inline function {name}!(obj::{struct_def.name}, value::{type_def.lang_type})\n"
        code_body += generate_set_member_body(ctx, member_def)
        code_body += "end\n"
        code_body += "\n"

        # offset
        code_body += (
            f"@inline function {prefix}{name}_offset(obj::{struct_def.name})::UInt64\n"
        )
        code_body += generate_offset_member_body(ctx, i, struct_def, member_def)
        code_body += "end\n"
        code_body += "\n"

        # size aligned
        code_body += f"@inline function {prefix}{name}_size_aligned(obj::{struct_def.name})::UInt64\n"
        code_body += generate_size_member_body(ctx, member_def, False)
        code_body += "end\n"
        code_body += "\n"

        # calc size aligned
        if type_def.lang_type == "StringView":
            code_body += f"@inline function {prefix}{name}_calc_size_aligned(::Type{{{struct_def.name}}}, value::T)::UInt64 where {{T<:AbstractString}}\n"
        else:
            code_body += f"@inline function {prefix}{name}_calc_size_aligned(::Type{{{struct_def.name}}}, value::{type_def.lang_type})::UInt64\n"
        code_body += generate_calc_size_aligned_member_body(ctx, struct_def, member_def)
        code_body += "end\n"
        code_body += "\n"

        # size unaligned
        if type_def.category == "c":
            code_body += f"@inline function {prefix}{name}_size_unaligned(obj::{struct_def.name})::UInt64\n"
            code_body += generate_size_member_body(ctx, member_def, True)
            code_body += "end\n"

    code_body += (
        "\n# --------------------------------------------------------------------\n"
    )

    code = "import Base.show\n"
    code += "import Base.finalizer\n"
    includes = [
        m.type_def.include_stmt
        for m in struct_def.members.values()
        if m.type_def.include_stmt != ""
    ]
    for include in list(dict.fromkeys(includes)):  # remove duplicates
        code += f"{include}\n"
    code += "\n"
    code += '"""\n'
    if struct_def.docstring:
        for line in struct_def.docstring:
            code += f"{line}\n"
        code += "\n"
        code += f"{'-' * 60}\n"
        code += "\n"

    code += "Binary serializable data container generated by `fastbin`.\n"
    code += "\n"
    if struct_def.type_def.variable_length:
        code += "This container has variable size.\n"
        code += "All setter methods starting from the first variable-sized member and afterwards MUST be called in order.\n\n"
    else:
        code += f"This container has fixed size of {struct_def.type_def.aligned_size} bytes.\n"
        code += "\n"

    code += "Members in order\n"
    code += "================\n"
    for i, (name, member_def) in enumerate(struct_def.members.items()):
        name_type = "`" + name + "::" + member_def.type_def.lang_type + "`"
        code += f"- {name_type.ljust(24)} ({'variable' if member_def.type_def.variable_length else 'fixed'})\n"
    code += "\n"
    code += "The `fastbin_finalize!()` method MUST be called after all setter methods have been called.\n"
    code += "\n"
    code += "It is the responsibility of the caller to ensure that the buffer is\n"
    code += "large enough to hold all data.\n"
    code += '"""\n'
    code += f"mutable struct {struct_def.name}\n"
    code += "    buffer::Ptr{UInt8}\n"
    code += "    buffer_size::UInt64\n"
    code += "    owns_buffer::Bool\n"

    # constructors
    code += "\n"
    code += f"    function {struct_def.name}(buffer::Ptr{{UInt8}}, buffer_size::Integer, owns_buffer::Bool)\n"
    code += "        owns_buffer || (@assert iszero(UInt(buffer) & 0x7) \"Buffer not 8 byte aligned\")\n"
    code += "        new(buffer, UInt64(buffer_size), owns_buffer)\n"
    code += "    end\n"
    code += "\n"
    code += f"    function {struct_def.name}(buffer_size::Integer)\n"
    code += "        buffer = reinterpret(Ptr{UInt8}, Base.Libc.malloc(buffer_size))\n"
    code += "        obj = new(buffer, buffer_size, true)\n"
    code += "        finalizer(_finalize!, obj)\n"
    code += "    end\n"

    code += "end\n"

    # finalizer (called by garbage collector)
    code += "\n"
    code += f"_finalize!(obj::{struct_def.name}) = Base.Libc.free(obj.buffer)\n"
    code += "\n"

    # member functions
    code += code_body

    # binary size calculated
    code += "\n"
    code += (
        f"@inline function fastbin_calc_binary_size(obj::{struct_def.name})::UInt64\n"
    )
    if struct_def.type_def.variable_length:
        code += f"    return {prefix}{member_names[-1]}_offset(obj) + {prefix}{member_names[-1]}_size_aligned(obj)\n"
    else:
        code += f"    return {struct_def.type_def.aligned_size}\n"
    code += "end\n"

    # binary size estimation helper
    if not struct_def.type_def.variable_length:
        # fixed size
        code += "\n"
        code += (
            f"@inline function fastbin_calc_binary_size(::Type{{{struct_def.name}}})\n"
        )
        code += f"    {struct_def.type_def.aligned_size}\n"
        code += "end\n"
    else:
        # variable length.
        # calculate size based on the fixed size + the size of all variable-length members
        var_members = [
            (k, v)
            for (k, v) in struct_def.members.items()
            if v.type_def.variable_length
        ]
        fixed_members = [
            (k, v)
            for (k, v) in struct_def.members.items()
            if not v.type_def.variable_length
        ]
        code += "\n"
        code += f"@inline function fastbin_calc_binary_size(::Type{{{struct_def.name}}}"
        for name, member_def in var_members:
            code += f",\n    {member_def.name}::{member_def.type_def.lang_type}"
        code += "\n)\n"
        fixed_size = 8 + sum([v.type_def.aligned_size for (_, v) in fixed_members])
        code += f"    return {fixed_size} +\n"
        code += "        " + " +\n        ".join(
            [
                f"{prefix}{name}_calc_size_aligned({struct_def.name}, {name})"
                for name, _ in var_members
            ]
        )
        code += "\n"
        code += "end\n"

    # binary size
    code += "\n"
    code += '"""\n'
    code += "Returns the stored (aligned) binary size of the object.\n"
    code += "This function should only be called after `fastbin_finalize!(obj)`.\n"
    code += '"""\n'
    code += f"@inline function fastbin_binary_size(obj::{struct_def.name})::UInt64\n"
    if struct_def.type_def.variable_length:
        code += "    return unsafe_load(reinterpret(Ptr{UInt64}, obj.buffer))\n"
    else:
        code += f"    return {struct_def.type_def.aligned_size}\n"
    code += "end\n"

    # finalize (write the struct's binary size to the first 8 bytes)
    code += "\n"
    code += '"""\n'
    code += "Finalizes the object by writing the binary size to the beginning of its buffer.\n"
    code += "After calling this function, the underlying buffer can be used for serialization.\n"
    code += "To get the actual buffer size, call `fastbin_binary_size(obj)`.\n"
    code += '"""\n'
    code += f"@inline function fastbin_finalize!(obj::{struct_def.name})\n"
    if struct_def.type_def.variable_length:
        code += "    unsafe_store!(reinterpret(Ptr{UInt64}, obj.buffer), fastbin_calc_binary_size(obj))\n"
        code += "    nothing\n"
    code += "end\n"

    # override Base.show
    code += "\n"
    code += f"function show(io::IO, obj::{struct_def.name})\n"
    code += f'    print(io, "[{ctx.namespace}::{struct_def.name}]")\n'
    for i, (name, member_def) in enumerate(struct_def.members.items()):
        code += f'    print(io, "\\n    {name}: ")\n'
        if member_def.type_def.use_print:
            code += f"    print(io, {member_def.name}(obj))\n"
        else:
            code += f"    show(io, {member_def.name}(obj))\n"
    code += "    println(io)\n"
    code += "end\n"

    return code


def generate_single_include_file(output_dir: str, ctx: GenContext):
    code = f"module {ctx.namespace}\n\n"

    for enum_name in ctx.enums.keys():
        code += f'include("{enum_name}.jl")\n'

    code += "\n"

    for struct_name in ctx.structs.keys():
        code += f'include("{struct_name}.jl")\n'

    code += "\n"

    # export all
    code += "# export all types and functions\n"
    code += "for n in names(@__MODULE__; all=true)\n"
    code += (
        "    if Base.isidentifier(n) && n ∉ (Symbol(@__MODULE__), :eval, :include) && !startswith(string(n), \"_\")\n"
    )
    code += "        @eval export $n\n"
    code += "    end\n"
    code += "end\n\n"

    code += "end\n"

    with open(output_dir / "models.jl", "w") as file:
        file.write(code)


def copy_template_files(output_dir: Path, ctx: GenContext, script_dir: Path):
    # copy template files from "templates" directory to output directory
    # replace "YOUR_NAMESPACE" with actual namespace
    tpl_path = script_dir / "templates"
    for file in tpl_path.iterdir():
        with (
            open(file, "r") as src,
            open(output_dir / file.name, "w") as dst,
        ):
            dst.write(src.read().replace("YOUR_NAMESPACE", ctx.namespace))


def generate_code(schema_file: Path, output_dir: Path, script_dir: Path):
    with open(schema_file, "r") as file:
        schema = json.load(file)

    ctx = GenContext(schema)

    # ensure target dir exists
    os.makedirs(output_dir, exist_ok=True)

    # ensure target dir is empty to clear stale files
    [shutil.rmtree(p) if p.is_dir() else p.unlink() for p in output_dir.iterdir()]

    for enum_name, enum_def in ctx.enums.items():
        gen_code = generate_enum(ctx, enum_def)

        # write to file
        with open(output_dir / f"{enum_name}.jl", "w") as file:
            file.write(gen_code)

    for struct_name, struct_def in ctx.structs.items():
        gen_code = generate_struct(ctx, struct_def)

        # write to file
        with open(output_dir / f"{struct_name}.jl", "w") as file:
            file.write(gen_code)

    # single include header file
    generate_single_include_file(output_dir, ctx)

    # move template files to output directory
    copy_template_files(output_dir, ctx, script_dir)


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python fastbin_jl.py <schema.json> <output_dir>")
        sys.exit(1)

    script_dir = Path(__file__).resolve().parent
    schema_file = Path(sys.argv[1]).resolve()
    output_dir = Path(sys.argv[2]).resolve()
    generate_code(schema_file, output_dir, script_dir)
