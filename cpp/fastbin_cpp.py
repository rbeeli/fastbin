from itertools import chain
import json
import os
import sys
import math
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
    elements_type_defs: Optional[List["TypeDef"]] = None


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
    map: List[str]
    docstring: List[str]


@dataclass
class EnumDef:
    name: str
    type_def: TypeDef
    storage_type_def: TypeDef
    generate_parse: bool
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
            "int8": TypeDef(
                "p", "int8", "std::int8_t", "#include <cstdint>", 1, 8, False, True
            ),
            "int16": TypeDef(
                "p", "int16", "std::int16_t", "#include <cstdint>", 2, 8, False, True
            ),
            "int32": TypeDef(
                "p", "int32", "std::int32_t", "#include <cstdint>", 4, 8, False, True
            ),
            "int64": TypeDef(
                "p", "int64", "std::int64_t", "#include <cstdint>", 8, 8, False, True
            ),
            "uint8": TypeDef(
                "p", "uint8", "std::uint8_t", "#include <cstdint>", 1, 8, False, True
            ),
            "uint16": TypeDef(
                "p", "uint16", "std::uint16_t", "#include <cstdint>", 2, 8, False, True
            ),
            "uint32": TypeDef(
                "p", "uint32", "std::uint32_t", "#include <cstdint>", 4, 8, False, True
            ),
            "uint64": TypeDef(
                "p", "uint64", "std::uint64_t", "#include <cstdint>", 8, 8, False, True
            ),
            "byte": TypeDef(
                "p", "byte", "std::byte", "#include <cstddef>", 1, 8, False, True
            ),
            "char": TypeDef("p", "char", "char", "", 1, 8, False, True),
            "float32": TypeDef(
                "p", "float32", "float", "#include <cstddef>", 8, 8, False, True
            ),
            "float64": TypeDef(
                "p", "float64", "double", "#include <cstddef>", 8, 8, False, True
            ),
            "bool": TypeDef("p", "bool", "bool", "", 1, 8, False, True),
        }
        self.built_in_types["string"] = TypeDef(
            "c",
            "string",
            "std::string_view",
            "#include <string_view>",
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
                enum_name,
                f'#include "{enum_name}.hpp"',
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
                map = member_content.get("map", member_name)
                if not isinstance(map, list):
                    map = [map]
                member_docstring = parse_docstring(
                    member_content.get("docstring", None)
                )
                members[member_name] = EnumMemberDef(
                    member_name, value, map, member_docstring
                )

            # ensure that enum members are unique
            if len(members) != len(set(members.keys())):
                raise ValueError(f"Enum '{enum_name}' has duplicate members")

            # ensure that enum values are unique
            if len(set([m.value for m in members.values()])) != len(members):
                raise ValueError(f"Enum '{enum_name}' has duplicate values")

            # ensure that enum maps are unique
            all_maps = list(chain.from_iterable(m.map for m in members.values()))
            if len(set(all_maps)) != len(all_maps):
                raise ValueError(f"Enum '{enum_name}' has duplicate map entries")

            generate_parse = enum_content.get("generate_parse", False)
            self.enums[enum_name] = EnumDef(
                enum_name, type_def, storage_type, generate_parse, members, docstring
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
                f'#include "{struct_name}.hpp"',
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

        # vector<T> -> std::span<T> or StructArray<T>
        if type_name.startswith("vector<"):
            el_type = type_name.split("<")[1].split(">")[0]
            el_type_def = self.get_type_def(el_type)
            if el_type_def.category == "p":
                # vector of primitives
                return TypeDef(
                    "c",
                    type_name,
                    f"std::span<{self.get_type_def(el_type).lang_type}>",
                    "#include <span>",
                    -1,
                    -1,
                    True,
                    False,
                    el_type_def,
                )
            elif el_type_def.category == "s":
                # vector of fixed or variable-sized structs
                return TypeDef(
                    "c",
                    type_name,
                    f"StructArray<{self.get_type_def(el_type).lang_type}>",
                    '#include "StructArray.hpp"',
                    -1,
                    -1,
                    True,
                    False,
                    el_type_def,
                )
            else:
                raise ValueError(
                    f"Variable length container of type '{el_type}' not supported"
                )

        # Variant<T1, T2, ...> -> Variant<T1, T2, ...>
        if type_name.startswith("Variant<"):
            el_types = [
                x.strip() for x in type_name.split("<")[1].split(">")[0].split(",")
            ]
            elements_type_defs = [self.get_type_def(x) for x in el_types]
            return TypeDef(
                "v",
                type_name,
                f"Variant<{', '.join(x.lang_type for x in elements_type_defs)}>",
                '#include "Variant.hpp"',
                -1,
                -1,
                True,
                False,
                None,
                elements_type_defs,
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
    code += f"{indent_str}/**\n"
    for line in docstring:
        code += f"{indent_str} * {line}\n"
    code += f"{indent_str} */\n"
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
        code_body += f"    {name} = {member_def.value},\n"

    code = "#pragma once\n"
    code += "\n"
    code += "#include <string>\n"
    code += "#include <ostream>\n"
    code += "#include <utility>\n"
    if enum_def.generate_parse:
        code += "#include <string_view>\n"
        code += "#include <expected>\n"
    if enum_def.storage_type_def.include_stmt:
        code += f"{enum_def.storage_type_def.include_stmt}\n"
    code += "\n"
    code += f"namespace {ctx.namespace}\n"
    code += "{\n"
    if enum_def.docstring:
        code += generate_docstring(enum_def.docstring, 0)
    code += f"enum class {enum_def.name} : {enum_def.storage_type_def.lang_type}\n"
    code += "{\n"
    code += code_body
    code += "};\n"

    # parse
    if enum_def.generate_parse:
        code += "\n"
        code += f"[[nodiscard]] std::expected<{ctx.namespace}::{enum_def.name}, std::string> parse_{enum_def.name}(std::string_view str)\n"
        code += "{\n"
        for mem in enum_def.members.values():
            code += f"    if ({' || '.join(f'str == "{m}"' for m in mem.map)})\n"
            code += f"        return {ctx.namespace}::{enum_def.name}::{mem.name};\n"
        code += f'    return std::unexpected("Unknown {enum_def.name} enum string value: " + std::string(str));\n'
        code += "}\n"

    code += f"}}; // namespace {ctx.namespace}\n"

    # to_string
    code += "\n"
    code += f"[[nodiscard]] constexpr std::string_view to_string({ctx.namespace}::{enum_def.name} value) noexcept\n"
    code += "{\n"
    code += "    switch (value)\n"
    code += "    {\n"
    for name in enum_def.members.keys():
        code += f"        case {ctx.namespace}::{enum_def.name}::{name}:\n"
        code += f'            return "{name}";\n'
    code += "    }\n"
    code += "    std::unreachable();\n"
    code += "}\n"

    # ostream << operator
    code += "\n"
    code += f"std::ostream& operator<<(std::ostream& os, const {ctx.namespace}::{enum_def.name}& obj)\n"
    code += "{\n"
    code += "    os << to_string(obj);\n"
    code += "    return os;\n"
    code += "}\n"

    return code


def generate_get_member_body(ctx: GenContext, member_def: StructMemberDef):
    type_def = member_def.type_def
    lang_type = type_def.lang_type

    if type_def.category == "e":
        # enum
        code = f"        return static_cast<{lang_type}>(*reinterpret_cast<const {lang_type}*>(buffer + {prefix}{member_def.name}_offset()));\n"
    elif type_def.category == "p":
        # primitive
        code = f"        return *reinterpret_cast<const {lang_type}*>(buffer + {prefix}{member_def.name}_offset());\n"
    elif type_def.category == "s":
        # struct
        code = f"        auto ptr = buffer + {prefix}{member_def.name}_offset();\n"
        code += f"        return {lang_type}::open(ptr, {prefix}{member_def.name}_size_aligned(), false);\n"
    elif type_def.category == "c":
        # container with variable length (string, vector<T>)
        el_type_def = type_def.element_type_def
        el_type_cpp = el_type_def.lang_type
        if el_type_def.category == "p":
            # for primitive elements
            if type_def.name == "string":
                # std::string_view
                code = f"        size_t n_bytes = {prefix}{member_def.name}_size_unaligned() - 8;\n"  # -8 to skip the size
                code += f"        auto ptr = reinterpret_cast<const char*>(buffer + {prefix}{member_def.name}_offset() + 8);\n"  # +8 to skip the size
                code += "        return std::string_view(ptr, n_bytes);\n"
            elif type_def.name.startswith("vector<") and el_type_def.variable_length:
                # StructArray<T>
                # Note: size is part of StructArray<T>, that's why no + 8 here!
                code = f"        return StructArray<{el_type_cpp}>(buffer + {prefix}{member_def.name}_offset());\n"
            else:
                # std::span<T>
                code = f"        size_t n_bytes = {prefix}{member_def.name}_size_unaligned() - 8;\n"  # -8 to skip the size
                code += f"        size_t count = n_bytes >> {int(math.log2(el_type_def.native_size))};\n"
                code += f"        auto ptr = reinterpret_cast<{el_type_cpp}*>(buffer + {prefix}{member_def.name}_offset() + 8);\n"  # +8 to skip the size
                code += f"        return std::span<{el_type_cpp}>(ptr, count);\n"
        elif el_type_def.category == "s":
            # StructArray<T> where T are structs
            code = f"        auto ptr = buffer + {prefix}{member_def.name}_offset();\n"
            code += f"        return StructArray<{el_type_cpp}>::open(ptr, {prefix}{member_def.name}_size_unaligned(), false);\n"
        else:
            raise ValueError(
                f"Unsupported container element type: {el_type_def.category}"
            )
    elif type_def.category == "v":
        # variant
        code = f"        size_t offset = {prefix}{member_def.name}_offset();\n"
        code += "        size_t aligned_size_high = *reinterpret_cast<size_t*>(buffer + offset);\n"
        code += "        size_t aligned_size = aligned_size_high & 0x00FFFFFFFFFFFFFF;\n"  # remove 8 high-bits
        code += "        size_t aligned_diff = aligned_size_high >> 56;\n"
        code += "        auto ptr = buffer + offset + 8;\n"
        code += f"        return Variant<{', '.join(x.lang_type for x in type_def.elements_type_defs)}>::open(ptr, aligned_size - aligned_diff, false);\n"
    else:
        raise ValueError(f"Unknown type category: {type_def.category}")

    return code


def generate_set_member_body(ctx: GenContext, member_def: StructMemberDef):
    type_def = member_def.type_def
    lang_type = type_def.lang_type

    if type_def.category == "e":
        # enum
        code = f"        *reinterpret_cast<{lang_type}*>(buffer + {prefix}{member_def.name}_offset()) = static_cast<{lang_type}>(value);\n"
    elif type_def.category == "p":
        # primitive
        code = f"        *reinterpret_cast<{lang_type}*>(buffer + {prefix}{member_def.name}_offset()) = value;\n"
    elif type_def.category == "s":
        # struct
        code = f'        assert(value.fastbin_binary_size() > 0 && "Cannot set member `{member_def.name}`, parameter of struct type `{lang_type}` not finalized. Call fastbin_finalize() on struct after creation.");\n'
        code += (
            f"        auto dest_ptr = buffer + {prefix}{member_def.name}_offset();\n"
        )
        code += "        size_t size = value.fastbin_binary_size();\n"
        code += "        std::copy(value.buffer, value.buffer + size, dest_ptr);\n"
    elif type_def.category == "v":
        # variant
        code = f'        assert(value.fastbin_binary_size() > 0 && "Cannot set member `{member_def.name}`, parameter of Variant type `{lang_type}` not initialized.");\n'
        code += f"        size_t offset = {prefix}{member_def.name}_offset();\n"
        code += "        size_t contents_size = value.fastbin_binary_size();\n"
        code += "        size_t unaligned_size = 8 + contents_size;\n"
        code += "        size_t aligned_size = (unaligned_size + 7) & ~7;\n"
        code += "        size_t aligned_diff = aligned_size - unaligned_size;\n"
        # add diff to high-bits of aligned_size
        code += (
            "        size_t aligned_size_high = aligned_size | (aligned_diff << 56);\n"
        )
        code += (
            "        *reinterpret_cast<size_t*>(buffer + offset) = aligned_size_high;\n"
        )
        code += "        auto dest_ptr = buffer + offset + 8;\n"
        code += (
            "        std::copy(value.buffer, value.buffer + contents_size, dest_ptr);\n"
        )
    elif type_def.category == "c":
        # container with variable length (string, vector<T>)
        el_type_def = member_def.type_def.element_type_def
        if el_type_def.category == "p":
            # for primitive elements
            el_size_bytes = el_type_def.native_size
            code = f"        size_t offset = {prefix}{member_def.name}_offset();\n"
            code += f"        size_t contents_size = value.size() * {el_size_bytes};\n"
            maybe_unaligned = el_type_def.native_size % 8 != 0
            if maybe_unaligned:
                # string or vector<T> with size of T not divisible by 8
                code += "        size_t unaligned_size = 8 + contents_size;\n"
                code += "        size_t aligned_size = (unaligned_size + 7) & ~7;\n"
                code += "        size_t aligned_diff = aligned_size - unaligned_size;\n"
                # add diff to high-bits of aligned_size
                code += "        size_t aligned_size_high = aligned_size | (aligned_diff << 56);\n"
                code += "        *reinterpret_cast<size_t*>(buffer + offset) = aligned_size_high;\n"
            else:
                # element size is divisible by 8, no alignment adjustment needed
                code += "        *reinterpret_cast<size_t*>(buffer + offset) = 8 + contents_size;\n"
            code += "        auto dest_ptr = reinterpret_cast<std::byte*>(buffer + offset + 8);\n"
            code += "        auto src_ptr = reinterpret_cast<const std::byte*>(value.data());\n"
            code += "        std::copy(src_ptr, src_ptr + contents_size, dest_ptr);\n"
        elif el_type_def.category == "s":
            # this is the same code as for struct (always aligned to 8 bytes)
            code = f'        assert(value.fastbin_binary_size() > 0 && "Cannot set member `{member_def.name}`. Parameter of type `{lang_type}` not initialized.");\n'
            code += f"        auto dest_ptr = buffer + {prefix}{member_def.name}_offset();\n"
            code += "        size_t size = value.fastbin_binary_size();\n"
            code += "        std::copy(value.buffer, value.buffer + size, dest_ptr);\n"
        else:
            raise ValueError(
                f"Unsupported container element type: {el_type_def.category}"
            )
    else:
        raise ValueError(f"Unknown type category: {type_def.category}")

    return code


def generate_size_member_body(
    ctx: GenContext,
    member_def: StructMemberDef,
    unaligned_size: bool,
):
    # NOTE: Must match implementation in `generate_calc_size_aligned_member_body`
    type_def = member_def.type_def

    if type_def.category in ["p", "e"]:
        # primitives, enum
        code = f"        return {type_def.aligned_size};\n"
    elif type_def.category == "c":
        # container with variable length (string, vector<T>)
        el_type_def = type_def.element_type_def
        code = f"        size_t stored_size = *reinterpret_cast<size_t*>(buffer + {prefix}{member_def.name}_offset());\n"
        maybe_unaligned = el_type_def.native_size % 8 != 0
        if maybe_unaligned:
            # string or vector<T> with size of T not divisible by 8
            code += "        size_t aligned_size = stored_size & 0x00FFFFFFFFFFFFFF;\n"  # remove 8 high-bits
            if unaligned_size:
                code += "        size_t aligned_diff = stored_size >> 56;\n"
                code += "        return aligned_size - aligned_diff;\n"
            else:
                code += "        return aligned_size;\n"
        else:
            # element size is divisible by 8, no alignment adjustment needed
            code += "        return stored_size;\n"
    elif type_def.category == "v":
        # variant
        code = f"        size_t stored_size = *reinterpret_cast<size_t*>(buffer + {prefix}{member_def.name}_offset());\n"
        code += "        size_t aligned_size = stored_size & 0x00FFFFFFFFFFFFFF;\n"  # remove 8 high-bits
        if unaligned_size:
            code += "        size_t aligned_diff = stored_size >> 56;\n"
            code += "        return aligned_size - aligned_diff;\n"
        else:
            code += "        return aligned_size;\n"
    elif type_def.category == "s":
        # structs are always aligned to 8 bytes
        if type_def.variable_length:
            code = f"        return *reinterpret_cast<size_t*>(buffer + {prefix}{member_def.name}_offset());\n"
        else:
            code = f"        return {type_def.aligned_size};\n"
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
        code = f"        return {type_def.aligned_size};\n"
    elif type_def.category == "c":
        if type_def.lang_type.startswith("StructArray"):
            # container with fixed- or variable size structs (always aligned to 8 bytes)
            code = "        return value.fastbin_binary_size();\n"
        else:
            # container with variable size (string, vector<T>) and fixed element size
            el_type_def = member_def.type_def.element_type_def
            el_size_bytes = el_type_def.native_size
            code = f"        size_t contents_size = value.size() * {el_size_bytes};\n"
            maybe_unaligned = el_type_def.native_size % 8 != 0
            if maybe_unaligned:
                # string or vector<T> with size of T not divisible by 8
                code += "        size_t unaligned_size = 8 + contents_size;\n"
                code += "        return (unaligned_size + 7) & ~7;\n"
            else:
                # element size is divisible by 8, no alignment adjustment needed
                code += "        return 8 + contents_size;\n"
    elif type_def.category == "v":
        # variant
        code = "        size_t unaligned_size = 8 + value.fastbin_binary_size();\n"
        code += "        size_t aligned_size = (unaligned_size + 7) & ~7;\n"
        code += "        return aligned_size;\n"
    elif type_def.category == "s":
        # struct
        code = "        return value.fastbin_binary_size();\n"
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
        code_body = f"        return {prefix}{member_names[index - 1]}_offset() + {prefix}{member_names[index - 1]}_size_aligned();\n"
    else:
        code_body = f"        return {offset};\n"
    return code_body


def ostream_member_output(ctx: GenContext, member_def: StructMemberDef):
    # string (string_view)
    if member_def.type_def.name == "string":
        return f"std::string(obj.{member_def.name}())"

    # bool
    if member_def.type_def.name == "bool":
        return f'(obj.{member_def.name}() ? "true" : "false")'

    # containers (string, vector<T>)
    if member_def.type_def.category == "c":
        return f'"[{member_def.type_def.name} count=" << obj.{member_def.name}().size() << "]"'

    # Variant<T1, T2, ...>
    if member_def.type_def.category == "v":
        return f'"[{member_def.type_def.name} index=" << obj.{member_def.name}().index() << "]"'

    return f"obj.{member_def.name}()"


def generate_struct(ctx: GenContext, struct_def: StructDef):
    print(f"Generating struct {ctx.namespace}::{struct_def.name}", end=" ")
    if struct_def.type_def.variable_length:
        print("[size=variable]")
    else:
        print(f"[size={struct_def.type_def.aligned_size} bytes]")
    for name, member_def in struct_def.members.items():
        print(f"- {name}: {member_def.type_def.lang_type}")

    includes = [
        "#include <cstddef>",
        "#include <cstring>",
        "#include <cassert>",
        "#include <ostream>",
        "#include <span>",
        '#include "_traits.hpp"',
        '#include "_BufferStored.hpp"',
    ]

    member_names = list(name for name, _ in struct_def.members.items())

    if len(member_names) == 0:
        raise ValueError(
            f"struct {ctx.namespace}::{struct_def.name} does not have any members"
        )

    # generate member functions (get, set, size, offset)
    code_body = ""
    for i, (name, member_def) in enumerate(struct_def.members.items()):
        code_body += f"\n    // Member: {name} [{member_def.type_def.lang_type}]\n"
        type_def = member_def.type_def

        # getter
        code_body += f"\n    inline {type_def.lang_type} {name}() const noexcept\n"
        code_body += "    {\n"
        code_body += generate_get_member_body(ctx, member_def)
        code_body += "    }\n"
        code_body += "\n"

        # setter
        if type_def.category == "s" or type_def.lang_type.startswith("StructArray"):
            # struct or StructArray<T>
            code_body += (
                f"    inline void {name}(const {type_def.lang_type}& value) noexcept\n"
            )
        elif type_def.category == "v":
            # Variant<T1, T2, ...>
            code_body += (
                f"    inline void {name}(const {type_def.lang_type}& value) noexcept\n"
            )
        else:
            code_body += (
                f"    inline void {name}(const {type_def.lang_type} value) noexcept\n"
            )
        code_body += "    {\n"
        code_body += generate_set_member_body(ctx, member_def)
        code_body += "    }\n"
        code_body += "\n"

        # offset
        code_body += (
            f"    constexpr inline size_t {prefix}{name}_offset() const noexcept\n"
        )
        code_body += "    {\n"
        code_body += generate_offset_member_body(ctx, i, struct_def, member_def)
        code_body += "    }\n"
        code_body += "\n"

        # size aligned
        code_body += f"    constexpr inline size_t {prefix}{name}_size_aligned() const noexcept\n"
        code_body += "    {\n"
        code_body += generate_size_member_body(ctx, member_def, False)
        code_body += "    }\n"
        code_body += "\n"

        # calc size aligned
        if type_def.lang_type == "StringView":
            code_body += f"    static size_t {prefix}{name}_calc_size_aligned(const {type_def.lang_type}& value) noexcept\n"
        else:
            code_body += f"    static size_t {prefix}{name}_calc_size_aligned(const {type_def.lang_type}& value) noexcept\n"
        code_body += "    {\n"
        code_body += generate_calc_size_aligned_member_body(ctx, struct_def, member_def)
        code_body += "    }\n"
        code_body += "\n"

        # size unaligned
        if type_def.category == "c":
            code_body += f"    constexpr inline size_t {prefix}{name}_size_unaligned() const noexcept\n"
            code_body += "    {\n"
            code_body += generate_size_member_body(ctx, member_def, True)
            code_body += "    }\n"

    code_body += "\n    // --------------------------------------------------------------------------------\n"

    # add includes based on member types
    includes.extend(
        [
            m.type_def.include_stmt
            for m in struct_def.members.values()
            if m.type_def.include_stmt != ""
        ]
    )

    # sort includes so that *.hpp are included last
    includes.sort(key=lambda x: ".hpp" in x)

    code = "#pragma once\n"
    code += "\n"
    for include in list(dict.fromkeys(includes)):  # remove duplicates
        code += f"{include}\n"
    code += "\n"
    code += f"namespace {ctx.namespace}\n"
    code += "{\n"
    code += "/**\n"
    if struct_def.docstring:
        for line in struct_def.docstring:
            code += f" * {line}\n"
        code += " *\n"
        code += f" * {'-' * 60}\n"
        code += " *\n"
    code += " * Binary serializable data container generated by `fastbin`.\n"
    code += " * \n"
    if struct_def.type_def.variable_length:
        code += " * This container has variable size.\n"
        code += " * All setter methods starting from the first variable-sized member and afterwards MUST be called in order.\n"
        code += " *\n"
    else:
        code += f" * This container has fixed size of {struct_def.type_def.aligned_size} bytes.\n"
        code += " *\n"
    code += " * Members in order\n"
    code += " * ================\n"
    for i, (name, member_def) in enumerate(struct_def.members.items()):
        name_type = "`" + name + "` [`" + member_def.type_def.lang_type + "`]"
        code += f" * - {name_type.ljust(24)} ({'variable' if member_def.type_def.variable_length else 'fixed'})\n"
    code += " *\n"
    code += " * The `finalize()` method MUST be called after all setter methods have been called.\n"
    code += " * \n"
    code += " * It is the responsibility of the caller to ensure that the buffer is\n"
    code += " * large enough to hold all data.\n"
    code += " */\n"
    code += f"class {struct_def.name} final : public BufferStored<{struct_def.name}>\n"
    code += "{\n"
    code += "public:\n"
    code += f"    using BufferStored<{struct_def.name}>::buffer;\n"
    code += f"    using BufferStored<{struct_def.name}>::buffer_size;\n"
    code += f"    using BufferStored<{struct_def.name}>::owns_buffer;\n"
    code += "\n"

    code += "protected:\n"
    code += f"    friend class BufferStored<{struct_def.name}>;\n"
    code += "\n"
    # constructor
    code += f"    {struct_def.name}(\n"
    code += "        std::byte* buffer, size_t buffer_size, bool owns_buffer\n"
    code += "    ) noexcept\n"
    code += (
        f"        : BufferStored<{struct_def.name}>(buffer, buffer_size, owns_buffer)\n"
    )
    code += "    {\n"
    code += "    }\n"
    code += "\n"

    code += "public:\n"
    code += f"    static {struct_def.name} create(std::byte* buffer, size_t buffer_size, bool owns_buffer) noexcept\n"
    code += "    {\n"
    code += "        std::memset(buffer, 0, buffer_size);\n"
    code += "        return {buffer, buffer_size, owns_buffer};\n"
    code += "    }\n"
    code += "    \n"

    # delete copy constructor and assignment operator
    code += "    // disable copy\n"
    code += f"    {struct_def.name}(const {struct_def.name}&) = delete;\n"
    code += f"    {struct_def.name}& operator=(const {struct_def.name}&) = delete;\n"
    code += "\n"
    code += "    // inherit move\n"
    code += f"    {struct_def.name}({struct_def.name}&&) noexcept = default;\n"
    code += (
        f"    {struct_def.name}& operator=({struct_def.name}&&) noexcept = default;\n"
    )
    code += "\n"

    # member functions
    code += code_body

    # binary size calculated
    code += "\n"
    code += "    constexpr inline size_t fastbin_calc_binary_size() const noexcept\n"
    code += "    {\n"
    if struct_def.type_def.variable_length:
        code += f"        return {prefix}{member_names[-1]}_offset() + {prefix}{member_names[-1]}_size_aligned();\n"
    else:
        code += f"        return {struct_def.type_def.aligned_size};\n"
    code += "    }\n"

    # binary size estimation helper
    if not struct_def.type_def.variable_length:
        # fixed size
        code += "\n"
        code += "    constexpr size_t fastbin_calc_binary_size() noexcept\n"
        code += "    {\n"
        code += f"        return {struct_def.type_def.aligned_size};\n"
        code += "    }\n"
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
        code += "    static size_t fastbin_calc_binary_size(\n"
        params = [
            f"const {member_def.type_def.lang_type}& {member_def.name}"
            for name, member_def in var_members
        ]
        code += "        " + (",\n        ".join(params))
        code += "\n"
        code += "    ) noexcept\n"
        code += "    {\n"
        fixed_size = 8 + sum([v.type_def.aligned_size for (_, v) in fixed_members])
        code += f"        return {fixed_size} + "
        code += " +\n            ".join(
            [f"{prefix}{name}_calc_size_aligned({name})" for name, _ in var_members]
        )
        code += ";\n"
        code += "    }\n"

    # binary size
    code += "\n"
    code += "    /**\n"
    code += "     * Returns the stored (aligned) binary size of the object.\n"
    code += "     * This function should only be called after `fastbin_finalize()`.\n"
    code += "     */\n"
    code += "    constexpr inline size_t fastbin_binary_size() const noexcept\n"
    code += "    {\n"
    if struct_def.type_def.variable_length:
        code += "        return *reinterpret_cast<size_t*>(buffer);\n"
    else:
        code += f"        return {struct_def.type_def.aligned_size};\n"
    code += "    }\n"

    if not type_def.variable_length:
        # also provide static constexpr for fixed-size structs
        code += "\n"
        code += "    static constexpr size_t fastbin_fixed_size() noexcept\n"
        code += "    {\n"
        code += f"        return {struct_def.type_def.aligned_size};\n"
        code += "    }\n"

    # finalize (write the struct's binary size to the first 8 bytes)
    code += "\n"
    code += "    /**\n"
    code += "     * Finalizes the object by writing the binary size to the beginning of its buffer.\n"
    code += "     * After calling this function, the underlying buffer can be used for serialization.\n"
    code += "     * To get the actual buffer size, call `fastbin_binary_size()`.\n"
    code += "     */\n"
    code += "    inline void fastbin_finalize() noexcept\n"
    code += "    {\n"
    if struct_def.type_def.variable_length:
        code += (
            "        *reinterpret_cast<size_t*>(buffer) = fastbin_calc_binary_size();\n"
        )
    code += "    }\n"

    code += "};\n"

    # Type traits
    code += "\n"
    code += "// Type traits\n"
    code += "template <>\n"
    code += f"struct is_variable_size<{struct_def.name}> : std::{'true' if type_def.variable_length else 'false'}_type {{}};\n"
    code += "\n"
    code += "template <>\n"
    code += f"struct is_buffer_stored<{struct_def.name}> : std::true_type {{}};\n"
    code += f"}}; // namespace {ctx.namespace}\n"

    # ostream << operator
    code += "\n"
    code += f"inline std::ostream& operator<<(std::ostream& os, const {ctx.namespace}::{struct_def.name}& obj)\n"
    code += "{\n"
    code += f'    os << "[{ctx.namespace}::{struct_def.name} size=" << obj.fastbin_binary_size() << " bytes]\\n";\n'
    for i, (name, member_def) in enumerate(struct_def.members.items()):
        code += f'    os << "    {name}: " << {ostream_member_output(ctx, member_def)} << "\\n";\n'
    code += "    return os;\n"
    code += "}\n"

    return code


def generate_single_include_file(output_dir: Path, ctx: GenContext):
    code = "#pragma once\n"
    code += "\n"
    code += "#include <bit>\n"
    code += "\n"
    code += "static_assert(std::endian::native == std::endian::little);\n"
    code += "static_assert(sizeof(size_t) == 8, \"fastbin requires 64-bit size_t\");\n"
    code += "\n"

    for enum_name in ctx.enums.keys():
        code += f'#include "{enum_name}.hpp"\n'

    code += "\n"

    for struct_name in ctx.structs.keys():
        code += f'#include "{struct_name}.hpp"\n'

    with open(output_dir / "models.hpp", "w") as file:
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
        with open(output_dir / f"{enum_name}.hpp", "w") as file:
            file.write(gen_code)

    for struct_name, struct_def in ctx.structs.items():
        gen_code = generate_struct(ctx, struct_def)

        # write to file
        with open(output_dir / f"{struct_name}.hpp", "w") as file:
            file.write(gen_code)

    # single include header file
    generate_single_include_file(output_dir, ctx)

    # move template files to output directory
    copy_template_files(output_dir, ctx, script_dir)


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python fastbin_cpp.py <schema.json> <output_dir>")
        sys.exit(1)

    script_dir = Path(__file__).resolve().parent
    schema_file = Path(sys.argv[1]).resolve()
    output_dir = Path(sys.argv[2]).resolve()
    generate_code(schema_file, output_dir, script_dir)
