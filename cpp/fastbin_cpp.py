import json
import os
import sys
from typing import Dict, List, Optional


class TypeDef:
    category: str  # e = Enum, s = Struct, c = Container/Vector, p = Primitive
    name: str
    cpp_type: str
    include_stmt: str
    native_size: int
    aligned_size: int
    variable_length: bool
    use_print: bool
    element_type_def: Optional["TypeDef"]

    def __init__(
        self,
        category: str,
        name: str,
        cpp_type: str,
        include_stmt: str,
        native_size: int,
        aligned_size: int,
        variable_length: bool,
        use_print: bool,
        element_type_def: Optional["TypeDef"] = None,
    ):
        self.category = category
        self.name = name
        self.cpp_type = cpp_type
        self.include_stmt = include_stmt
        self.native_size = native_size
        self.aligned_size = aligned_size
        self.variable_length = variable_length
        self.use_print = use_print
        self.element_type_def = element_type_def


class StructMemberDef:
    name: str
    type_def: TypeDef

    def __init__(
        self,
        name: str,
        type_def: TypeDef,
    ):
        self.name = name
        self.type_def = type_def


class StructDef:
    name: str
    type_def: TypeDef
    members: Dict[str, StructMemberDef]
    docstring: str

    def __init__(
        self,
        name: str,
        type_def: TypeDef,
        members: Dict[str, StructMemberDef],
        docstring: str,
    ):
        self.name = name
        self.type_def = type_def
        self.members = members
        self.docstring = docstring


class EnumMemberDef:
    name: str
    value: int
    docstring: List[str]

    def __init__(self, name: str, value: int, docstring: str):
        self.name = name
        self.value = value
        self.docstring = docstring


class EnumDef:
    name: str
    type_def: TypeDef
    storage_type_def: TypeDef
    members: Dict[str, EnumMemberDef]
    docstring: str

    def __init__(
        self,
        name: str,
        type_def: TypeDef,
        storage_type_def: TypeDef,
        members: Dict[str, EnumMemberDef],
        docstring: str,
    ):
        self.name = name
        self.type_def = type_def
        self.storage_type_def = storage_type_def
        self.members = members
        self.docstring = docstring


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
            "char": TypeDef(
                "p", "char", "char", "", 1, 8, False, True
            ),
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
            if not "type" in enum_content:
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
                member_docstring = parse_docstring(
                    member_content.get("docstring", None)
                )
                members[member_name] = EnumMemberDef(
                    member_name, value, member_docstring
                )

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
        if not name in self.structs:
            raise ValueError(f"Struct '{name}' not found or used before declaration")
        return self.structs[name]

    def get_enum_def(self, name: str) -> EnumDef:
        if not name in self.enums:
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
                # TODO for variable size?
                raise ValueError(f"Vector element type '{el_type}' must be fixed size")
            return TypeDef(
                "c",
                type_name,
                f"std::span<{self.get_type_def(el_type).cpp_type}>",
                "#include <span>",
                -1,
                -1,
                True,
                False,
                el_type_def,
            )

        raise ValueError(f"Unknown type: {type_name}")


def parse_docstring(docstring: str | List[str] | None):
    if docstring == None or docstring == "" or docstring == []:
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
    code += f"#include <string>\n"
    code += f"#include <ostream>\n"
    if enum_def.storage_type_def.include_stmt:
        code += f"{enum_def.storage_type_def.include_stmt}\n"
    code += "\n"
    code += f"namespace {ctx.namespace}\n"
    code += "{\n"
    if enum_def.docstring:
        code += generate_docstring(enum_def.docstring, 0)
    code += f"enum class {enum_def.name} : {enum_def.storage_type_def.cpp_type}\n"
    code += "{\n"
    code += code_body
    code += "};\n"

    code += f"}}; // namespace {ctx.namespace}\n"

    # to_string
    code += "\n"
    code += f"inline std::string to_string({ctx.namespace}::{enum_def.name} value)\n"
    code += "{\n"
    code += "    switch (value)\n"
    code += "    {\n"
    for name in enum_def.members.keys():
        code += f"        case {ctx.namespace}::{enum_def.name}::{name}:\n"
        code += f'            return "{name}";\n'
    code += "        default:\n"
    code += '            return "Unknown";\n'
    code += "    }\n"
    code += "}\n"

    # ostream << operator
    code += "\n"
    code += f"inline std::ostream& operator<<(std::ostream& os, const {ctx.namespace}::{enum_def.name}& obj)\n"
    code += "{\n"
    code += "    os << to_string(obj);\n"
    code += "    return os;\n"
    code += "}\n"

    return code


def generate_get_member_body(ctx: GenContext, member_def: StructMemberDef):
    type_def = member_def.type_def
    cpp_type = type_def.cpp_type

    if type_def.category == "e":
        # enum
        code = f"        return static_cast<{cpp_type}>(*reinterpret_cast<const {cpp_type}*>(buffer + fastbin_{member_def.name}_offset()));\n"
    elif type_def.category == "p":
        # primitive
        code = f"        return *reinterpret_cast<const {cpp_type}*>(buffer + fastbin_{member_def.name}_offset());\n"
    elif type_def.category == "s":
        # struct
        code = f"        auto ptr = buffer + fastbin_{member_def.name}_offset();\n"
        code += f"        return {cpp_type}(ptr, fastbin_{member_def.name}_size(), false);\n"
    elif type_def.category == "c":
        # container with variable length (string, vector<T>)
        el_type_def = type_def.element_type_def
        el_type_cpp = el_type_def.cpp_type
        code = f"        size_t n_bytes = fastbin_{member_def.name}_size_unaligned() - 8;\n"  # -8 to skip the size
        if el_type_def.native_size == 1:
            code += f"        size_t count = n_bytes;\n"
        elif el_type_def.native_size == 2:
            # >> 1 is equal to dividing by 2
            code += f"        size_t count = n_bytes >> 1;\n"
        elif el_type_def.native_size == 4:
            # >> 2 is equal to dividing by 4
            code += f"        size_t count = n_bytes >> 2;\n"
        elif el_type_def.native_size == 8:
            # >> 3 is equal to dividing by 8
            code += f"        size_t count = n_bytes >> 3;\n"
        else:
            code += f"        size_t count = n_bytes / {el_type_def.native_size};\n"
        if type_def.name == "string":
            # std::string_view
            code += f"        auto ptr = reinterpret_cast<const char*>(buffer + fastbin_{member_def.name}_offset() + 8);\n"  # +8 to skip the size
            code += f"        return std::string_view(ptr, count);\n"
        else:
            # std::span<T>
            code += f"        auto ptr = reinterpret_cast<{el_type_cpp}*>(buffer + fastbin_{member_def.name}_offset() + 8);\n"  # +8 to skip the size
            code += f"        return std::span<{el_type_cpp}>(ptr, count);\n"
    else:
        raise ValueError(f"Unknown type category: {type_def.category}")

    return code


def generate_set_member_body(ctx: GenContext, member_def: StructMemberDef):
    type_def = member_def.type_def
    cpp_type = type_def.cpp_type

    if type_def.category == "e":
        # enum
        code = f"        *reinterpret_cast<{cpp_type}*>(buffer + fastbin_{member_def.name}_offset()) = static_cast<{cpp_type}>(value);\n"
    elif type_def.category == "p":
        # primitive
        code = f"        *reinterpret_cast<{cpp_type}*>(buffer + fastbin_{member_def.name}_offset()) = value;\n"
    elif type_def.category == "c":
        # container with variable length (string, vector<T>) and fixed element size
        el_type_def = member_def.type_def.element_type_def
        el_size_bytes = el_type_def.native_size
        code = f"        size_t offset = fastbin_{member_def.name}_offset();\n"
        code += f"        size_t elements_size = value.size() * {el_size_bytes};\n"
        maybe_unaligned = el_type_def.native_size % 8 != 0
        if maybe_unaligned:
            # string or vector<T> with size of T not divisible of 8
            code += f"        size_t unaligned_size = 8 + elements_size;\n"
            code += f"        size_t aligned_size = (unaligned_size + 7) & ~7;\n"
            code += f"        size_t aligned_diff = aligned_size - unaligned_size;\n"
            # add diff to high-bits of aligned_size
            code += f"        size_t aligned_size_high = aligned_size | (aligned_diff << 56);\n"
            code += f"        *reinterpret_cast<size_t*>(buffer + offset) = aligned_size_high;\n"
        else:
            # element size is divisible by 8, no alignment adjustment needed
            code += f"        *reinterpret_cast<size_t*>(buffer + offset) = 8 + elements_size;\n"
        code += f"        auto dest_ptr = reinterpret_cast<std::byte*>(buffer + offset + 8);\n"
        code += f"        auto src_ptr = reinterpret_cast<const std::byte*>(value.data());\n"
        code += f"        std::copy(src_ptr, src_ptr + elements_size, dest_ptr);\n"
    elif type_def.category == "s":
        # struct
        code = f'        assert(value.fastbin_binary_size() > 0 && "Cannot set struct value, struct {cpp_type} not finalized, call fastbin_finalize() on struct after creation.");\n'
        code += f"        size_t offset = fastbin_{member_def.name}_offset();\n"
        code += f"        size_t size = value.fastbin_binary_size();\n"
        code += (
            f"        std::copy(value.buffer, value.buffer + size, buffer + offset);\n"
        )
    else:
        raise ValueError(f"Unknown type category: {type_def.category}")

    return code


def generate_size_member_body(
    ctx: GenContext, member_def: StructMemberDef, unaligned_size: bool
):
    type_def = member_def.type_def

    if type_def.category in ["p", "e"]:
        # primitives, enum
        code = f"        return {type_def.aligned_size};\n"
    elif type_def.category == "c":
        # container with variable length (string, vector<T>)
        el_type_def = type_def.element_type_def
        code = f"        size_t stored_size = *reinterpret_cast<size_t*>(buffer + fastbin_{member_def.name}_offset());\n"
        maybe_unaligned = el_type_def.native_size % 8 != 0
        if maybe_unaligned:
            # string or vector<T> with size of T not divisible of 8
            if unaligned_size:
                code += f"        size_t aligned_diff = stored_size >> 56;\n"
                code += f"        size_t aligned_size = stored_size & 0x00FFFFFFFFFFFFFF;\n"  # remove 8 high-bits
                code += f"        return aligned_size - aligned_diff;\n"
            else:
                code += f"        size_t aligned_size = stored_size & 0x00FFFFFFFFFFFFFF;\n"  # remove 8 high-bits
                code += f"        return aligned_size;\n"
        else:
            # element size is divisible by 8, no alignment adjustment needed
            code += f"        return stored_size;\n"
    elif type_def.category == "s":
        # structs are always aligned to 8 bytes
        if type_def.variable_length:
            code = f"        return *reinterpret_cast<size_t*>(buffer + fastbin_{member_def.name}_offset());\n"
        else:
            code = f"        return {type_def.aligned_size};\n"
    else:
        raise ValueError(f"Unknown type category: {type_def.category}")

    return code


def generate_offset_member_body(
    ctx: GenContext, index: int, struct_def: StructDef, member_def: StructMemberDef
):
    member_names = list(struct_def.members.keys())
    if struct_def.type_def.variable_length:
        # first 8 bytes are reserved for the size of the variable-length struct
        offset = 8
    else:
        # fixed-length struct
        offset = 0
    for (i, prev_member) in enumerate(struct_def.members.values()):
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
        code_body = f"        return fastbin_{member_names[index-1]}_offset() + fastbin_{member_names[index-1]}_size();\n"
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

    return f"obj.{member_def.name}()"


def generate_struct(ctx: GenContext, struct_def: StructDef):
    print(f"Generating struct {ctx.namespace}::{struct_def.name}", end=" ")
    if struct_def.type_def.variable_length:
        print(f"[size=variable]")
    else:
        print(f"[size={struct_def.type_def.aligned_size} bytes]")
    for name, member_def in struct_def.members.items():
        print(f"- {name}: {member_def.type_def.cpp_type}")

    includes = [
        "#include <cstddef>",
        "#include <cassert>",
        "#include <ostream>",
        "#include <span>",
    ]

    member_names = list(name for name, _ in struct_def.members.items())

    if len(member_names) == 0:
        raise ValueError(
            f"struct {ctx.namespace}::{struct_def.name} does not have any members"
        )

    # genereate member functions (get, set, size, offset)
    code_body = ""
    for i, (name, member_def) in enumerate(struct_def.members.items()):
        type_def = member_def.type_def
        code_body += f"\n    inline {type_def.cpp_type} {name}() const noexcept\n"
        code_body += "    {\n"
        code_body += generate_get_member_body(ctx, member_def)
        code_body += "    }\n"
        code_body += "\n"
        if type_def.category == "s":
            # struct
            code_body += f"    inline void {name}(const {type_def.cpp_type}& value) noexcept\n"
        else:
            code_body += f"    inline void {name}(const {type_def.cpp_type} value) noexcept\n"
        code_body += "    {\n"
        code_body += generate_set_member_body(ctx, member_def)
        code_body += f"    }}\n"
        code_body += "\n"
        code_body += f"    constexpr inline size_t fastbin_{name}_offset() const noexcept\n"
        code_body += "    {\n"
        code_body += generate_offset_member_body(ctx, i, struct_def, member_def)
        code_body += f"    }}\n"
        code_body += "\n"
        code_body += f"    constexpr inline size_t fastbin_{name}_size() const noexcept\n"
        code_body += "    {\n"
        code_body += generate_size_member_body(ctx, member_def, False)
        code_body += f"    }}\n"
        if type_def.category == "c":
            code_body += "\n"
            code_body += (
                f"    constexpr inline size_t fastbin_{name}_size_unaligned() const noexcept\n"
            )
            code_body += "    {\n"
            code_body += generate_size_member_body(ctx, member_def, True)
            code_body += f"    }}\n"

    # add includes based on member types
    includes.extend([
        m.type_def.include_stmt
        for m in struct_def.members.values()
        if m.type_def.include_stmt != ""
    ])
    
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
        code += f" *\n"
        code += f" * {'-'*60}\n"
        code += f" *\n"

    code += " * Binary serializable data container.\n"
    code += " * \n"
    if struct_def.type_def.variable_length:
        code += " * This container has variable size.\n"
        code += " * \n"
        code += " * Setter methods from the first variable-sized field onwards\n"
        code += " * MUST be called in order.\n"
        code += " * \n"
    else:
        code += f" * This container has fixed size of {struct_def.type_def.aligned_size} bytes.\n"
        code += " * \n"
    code += " * The `finalize()` method MUST be called after all setter methods have been called.\n"
    code += " * \n"
    code += " * It is the responsibility of the caller to ensure that the buffer is\n"
    code += " * large enough to hold the serialized data.\n"
    code += " */\n"
    code += f"struct {struct_def.name}\n"
    code += "{\n"
    code += "    std::byte* buffer{nullptr};\n"
    code += "    size_t buffer_size{0};\n"
    code += "    bool owns_buffer{false};\n"

    # constructor
    code += "\n"
    code += f"    explicit {struct_def.name}(std::byte* buffer, size_t binary_size, bool owns_buffer) noexcept\n"
    code += (
        "        : buffer(buffer), buffer_size(binary_size), owns_buffer(owns_buffer)\n"
    )
    code += "    {\n"
    code += "    }\n"

    # destructor
    code += "\n"
    code += f"    ~{struct_def.name}() noexcept\n"
    code += "    {\n"
    code += "        if (owns_buffer && buffer != nullptr)\n"
    code += "        {\n"
    code += "            delete[] buffer;\n"
    code += "            buffer = nullptr;\n"
    code += "        }\n"
    code += "    }\n"

    # delete copy constructor and assignment operator
    code += "\n"
    code += "    // disable copy\n"
    code += f"    {struct_def.name}(const {struct_def.name}&) = delete;\n"
    code += f"    {struct_def.name}& operator=(const {struct_def.name}&) = delete;\n"

    # implement move constructor and assignment operator
    code += "\n"
    code += "    // enable move\n"
    code += f"    {struct_def.name}({struct_def.name}&& other) noexcept\n"
    code += "        : buffer(other.buffer), buffer_size(other.buffer_size), owns_buffer(other.owns_buffer)\n"
    code += "    {\n"
    code += "        other.buffer = nullptr;\n"
    code += "        other.buffer_size = 0;\n    }\n"
    code += f"    {struct_def.name}& operator=({struct_def.name}&& other) noexcept\n"
    code += "    {\n"
    code += "        if (this != &other)\n"
    code += "        {\n"
    code += "            delete[] buffer;\n"
    code += "            buffer = other.buffer;\n"
    code += "            buffer_size = other.buffer_size;\n"
    code += "            owns_buffer = other.owns_buffer;\n"
    code += "            other.buffer = nullptr;\n"
    code += "            other.buffer_size = 0;\n        }\n"
    code += "        return *this;\n    }\n"

    # binary size
    code += "\n"
    code += "    constexpr inline size_t fastbin_binary_size() const noexcept\n"
    code += "    {\n"
    if struct_def.type_def.variable_length:
        code += f"        return *reinterpret_cast<size_t*>(buffer);\n"
    else:
        code += f"        return {struct_def.type_def.aligned_size};\n"
    code += "    }\n"

    # member functions
    code += code_body

    # binary size computed
    code += "\n"
    code += "    constexpr inline size_t fastbin_compute_binary_size() const noexcept\n"
    code += "    {\n"
    if struct_def.type_def.variable_length:
        code += (
            f"        return fastbin_{member_names[-1]}_offset() + fastbin_{member_names[-1]}_size();\n"
        )
    else:
        code += f"        return {struct_def.type_def.aligned_size};\n"
    code += "    }\n"

    # finalize (write the struct's binary size to the first 8 bytes)
    code += "\n"
    code += "    inline void fastbin_finalize() const noexcept\n"
    code += "    {\n"
    if struct_def.type_def.variable_length:
        code += "        *reinterpret_cast<size_t*>(buffer) = fastbin_compute_binary_size();\n"
    code += "    }\n"

    code += "};\n"
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


def generate_single_include_header_file(output_dir: str, ctx: GenContext):
    code = "#pragma once\n"
    code += "\n"

    for enum_name in ctx.enums.keys():
        code += f'#include "{enum_name}.hpp"\n'

    code += "\n"

    for struct_name in ctx.structs.keys():
        code += f'#include "{struct_name}.hpp"\n'

    with open(f"{output_dir}/models.hpp", "w") as file:
        file.write(code)


def generate_cpp_code(schema_file, output_dir: str):
    with open(schema_file, "r") as file:
        schema = json.load(file)

    ctx = GenContext(schema)

    os.makedirs(output_dir, exist_ok=True)

    for enum_name, enum_def in ctx.enums.items():
        cpp_code = generate_enum(ctx, enum_def)

        # write to file
        with open(f"{output_dir}/{enum_name}.hpp", "w") as file:
            file.write(cpp_code)

    for struct_name, struct_def in ctx.structs.items():
        cpp_code = generate_struct(ctx, struct_def)

        # write to file
        with open(f"{output_dir}/{struct_name}.hpp", "w") as file:
            file.write(cpp_code)

    # single include header file
    generate_single_include_header_file(output_dir, ctx)


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python fastbin_cpp.py <schema.json> <output_dir>")
        sys.exit(1)

    schema_file = sys.argv[1]
    output_dir = sys.argv[2]
    generate_cpp_code(schema_file, output_dir)
