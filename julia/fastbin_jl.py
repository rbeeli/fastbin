import json
import os
import sys
from typing import Dict, List, Optional

prefix = "_"  # prefix for internally generated functions


class TypeDef:
    category: str  # e = Enum, s = Struct, c = Container/Vector, p = Primitive
    name: str
    lang_type: str
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
        lang_type: str,
        include_stmt: str,
        native_size: int,
        aligned_size: int,
        variable_length: bool,
        use_print: bool,
        element_type_def: Optional["TypeDef"] = None,
    ):
        self.category = category
        self.name = name
        self.lang_type = lang_type
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
            "float32": TypeDef("p", "float32", "Float32", "", 8, 8, False, True),
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
            if not "type" in enum_content:
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
    if docstring == None or docstring == "" or docstring == []:
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
        code += f"    str == \"{name}\" && return {enum_def.name}.{name}\n"
    code += f"    throw(ArgumentError(\"Invalid string value for enum {ctx.namespace}.{enum_def.name}: $str\"))\n"
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
        code += (
            f"    return {lang_type}(ptr, {prefix}{member_def.name}_size_aligned(obj), false)\n"
        )
    elif type_def.category == "c":
        # container with variable length (string, vector<T>)
        el_type_def = type_def.element_type_def
        el_type_jl = el_type_def.lang_type
        code = f"    ptr::Ptr{{{el_type_jl}}} = reinterpret(Ptr{{{el_type_jl}}}, obj.buffer + {prefix}{member_def.name}_offset(obj))\n"
        code += f"    unaligned_size::UInt64 = {prefix}{member_def.name}_size_unaligned(obj)\n"
        code += f"    n_bytes::UInt64 = unaligned_size - 8\n"  # -8 to skip the size
        if el_type_def.native_size == 1:
            code += f"    count::UInt64 = n_bytes\n"
        elif el_type_def.native_size == 2:
            # >> 1 is equal to dividing by 2
            code += f"    count::UInt64 = n_bytes >> 1\n"
        elif el_type_def.native_size == 4:
            # >> 2 is equal to dividing by 4
            code += f"    count::UInt64 = n_bytes >> 2\n"
        elif el_type_def.native_size == 8:
            # >> 3 is equal to dividing by 8
            code += f"    count::UInt64 = n_bytes >> 3\n"
        else:
            code += f"    count::UInt64 = n_bytes / {el_type_def.native_size}\n"
        if type_def.name == "string":
            # StringView
            code += f"    return StringView(unsafe_wrap(Vector{{UInt8}}, ptr + 8, count, own=false))\n"  # +8 to skip the size
        else:
            # Vector{T}
            code += f"    return unsafe_wrap(Vector{{{el_type_def.lang_type}}}, ptr + 8, count, own=false)\n"  # +8 to skip the size
    else:
        raise ValueError(f"Unknown type category: {type_def.category}")

    return code


def generate_calc_size_aligned_member_body(ctx: GenContext, struct_def: StructDef, member_def: StructMemberDef):
    # NOTE: Must match implementation in `generate_size_member_body`
    type_def = member_def.type_def
    lang_type = type_def.lang_type

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
            # string or vector<T> with size of T not divisible of 8
            code += f"    unaligned_size::UInt64 = 8 + contents_size\n"
            code += f"    return (unaligned_size + 7) & ~7\n"
        else:
            # element size is divisible by 8, no alignment adjustment needed
            code += f"    return 8 + contents_size\n"
    elif type_def.category == "s":
        # struct
        code = f'    return binary_size(value)\n'
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
            code += f"    unaligned_size::UInt64 = 8 + contents_size\n"
            code += f"    aligned_size::UInt64 = (unaligned_size + 7) & ~7\n"
            code += f"    aligned_diff::UInt64 = aligned_size - unaligned_size\n"
            # add diff to high-bits of aligned_size
            code += (
                f"    aligned_size_high::UInt64 = aligned_size | (aligned_diff << 56)\n"
            )
            code += f"    unsafe_store!(reinterpret(Ptr{{UInt64}}, obj.buffer + offset), aligned_size_high)\n"
        else:
            # element size is divisible by 8, no alignment adjustment needed
            code += f"    unsafe_store!(reinterpret(Ptr{{UInt64}}, obj.buffer + offset), 8 + contents_size)\n"
        code += f"    dest_ptr::Ptr{{UInt8}} = obj.buffer + offset + 8\n"
        code += f"    src_ptr::Ptr{{UInt8}} = reinterpret(Ptr{{UInt8}}, pointer(value))\n"
        code += f"    unsafe_copyto!(dest_ptr, src_ptr, contents_size)\n"
    elif type_def.category == "s":
        # struct
        code = f'    @assert binary_size(value) > 0 "Cannot set member `{member_def.name}`, parameter struct of type `{lang_type}` not finalized. Call fastbin_finalize!(obj) on struct after creation."\n'
        code += f"    offset::UInt64 = {prefix}{member_def.name}_offset(obj)\n"
        code += f"    size::UInt64 = binary_size(value)\n"
        code += f"    unsafe_copyto!(obj.buffer + offset, value.buffer, size)\n"
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
                code += f"    aligned_diff::UInt64 = stored_size >> 56\n"
                code += f"    aligned_size::UInt64 = stored_size & 0x00FFFFFFFFFFFFFF\n"  # remove 8 high-bits
                code += f"    return aligned_size - aligned_diff\n"
            else:
                code += f"    aligned_size::UInt64 = stored_size & 0x00FFFFFFFFFFFFFF\n"  # remove 8 high-bits
                code += f"    return aligned_size\n"
        else:
            # element size is divisible by 8, no alignment adjustment needed
            code += f"    return stored_size\n"
    elif type_def.category == "s":
        # structs are always aligned to 8 bytes
        if type_def.variable_length:
            code = f"    return unsafe_load(reinterpret(Ptr{{UInt64}}, obj.buffer + {prefix}{member_def.name}_offset(obj)))\n"
        else:
            code = f"    return {type_def.aligned_size}\n"
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
        code_body = f"    return {prefix}{member_names[index-1]}_offset(obj) + {prefix}{member_names[index-1]}_size_aligned(obj)\n"
    else:
        code_body = f"    return {offset}\n"
    return code_body


def generate_struct(ctx: GenContext, struct_def: StructDef):
    print(f"Generating struct {ctx.namespace}::{struct_def.name}", end=" ")
    if struct_def.type_def.variable_length:
        print(f"[size=variable]")
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
        code_body += (
            f"@inline function {prefix}{name}_size_aligned(obj::{struct_def.name})::UInt64\n"
        )
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
        
    code_body += "\n# --------------------------------------------------------------------\n"
    
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
        code += f"\n"
        code += f"{'-'*60}\n"
        code += f"\n"

    code += "Binary serializable data container generated by `fastbin`.\n"
    code += "\n"
    if struct_def.type_def.variable_length:
        code += "This container has variable size.\n"
        code += "All setter methods starting from the first variable-sized member and afterwards MUST be called in order.\n\n"
        code += "Members in order\n"
        code += "================\n"
        for i, (name, member_def) in enumerate(struct_def.members.items()):
            name_type = '`' + name + "::" + member_def.type_def.lang_type + '`'
            code += f"- {name_type.ljust(24)} ({'variable' if member_def.type_def.variable_length else 'fixed'})\n"
        code += "\n"
    else:
        code += f"This container has fixed size of {struct_def.type_def.aligned_size} bytes.\n"
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
    code += f"    function {struct_def.name}(buffer::Ptr{{UInt8}}, buffer_size::UInt64, owns_buffer::Bool)\n"
    code += "        new(buffer, buffer_size, owns_buffer)\n"
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
    code += f"@inline function fastbin_calc_binary_size(obj::{struct_def.name})::UInt64\n"
    if struct_def.type_def.variable_length:
        code += f"    return {prefix}{member_names[-1]}_offset(obj) + {prefix}{member_names[-1]}_size_aligned(obj)\n"
    else:
        code += f"    return {struct_def.type_def.aligned_size}\n"
    code += "end\n"

    # binary size estimation helper
    if not struct_def.type_def.variable_length:
        # fixed size
        code += "\n"
        code += f"@inline function fastbin_calc_binary_size(::Type{{{struct_def.name}}})\n"
        code += f"    {struct_def.type_def.aligned_size}\n"
        code += "end\n"
    else:
        # variable length.
        # calculate size based on the fixed size + the size of all variable-length members
        var_members = [(k,v) for (k,v) in struct_def.members.items() if v.type_def.variable_length]
        fixed_members = [(k,v) for (k,v) in struct_def.members.items() if not v.type_def.variable_length]
        code += "\n"
        code += f"@inline function fastbin_calc_binary_size(::Type{{{struct_def.name}}}"
        fixed_size = 8 + sum([v.type_def.aligned_size for (_,v) in fixed_members])
        for (name, member_def) in var_members:
            code += f",\n    {member_def.name}::{member_def.type_def.lang_type}"
        code += "\n)\n"
        code += f"    return {fixed_size} +\n"
        code += "        " + ' +\n        '.join([f'{prefix}{name}_calc_size_aligned({struct_def.name}, {name})' for name, _ in var_members])
        code += "\n"
        code += "end\n"

    # binary size
    code += "\n"
    code += '"""\n'
    code += 'Returns the stored (aligned) binary size of the object.\n'
    code += 'This function should only be called after `fastbin_finalize!(obj)`.\n'
    code += '"""\n'
    code += f"@inline function fastbin_binary_size(obj::{struct_def.name})::UInt64\n"
    if struct_def.type_def.variable_length:
        code += f"    return unsafe_load(reinterpret(Ptr{{UInt64}}, obj.buffer))\n"
    else:
        code += f"    return {struct_def.type_def.aligned_size}\n"
    code += "end\n"

    # finalize (write the struct's binary size to the first 8 bytes)
    code += "\n"
    code += '"""\n'
    code += 'Finalizes the object by writing the binary size to the beginning of its buffer.\n'
    code += 'After calling this function, the underlying buffer can be used for serialization.\n'
    code += 'To get the actual buffer size, call `fastbin_binary_size(obj)`.\n'
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
    code += f"    println(io)\n"
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
    code += f"# export all types and functions\n"
    code += f"for n in names(@__MODULE__; all=true)\n"
    code += (
        f"    if Base.isidentifier(n) && n ∉ (Symbol(@__MODULE__), :eval, :include)\n"
    )
    # code += f'        println("Exporting: $n")\n'
    code += f"        @eval export $n\n"
    code += f"    end\n"
    code += f"end\n\n"

    # # export all enums
    # if len(ctx.enums) > 0:
    #     code += f'export {", ".join(ctx.enums.keys())}\n\n'

    # # export all structs
    # if len(ctx.structs) > 0:
    #     code += f'export {", ".join(ctx.structs.keys())}\n\n'

    code += "end\n"

    with open(f"{output_dir}/models.jl", "w") as file:
        file.write(code)


def generate_jl_code(schema_file, output_dir: str):
    with open(schema_file, "r") as file:
        schema = json.load(file)

    ctx = GenContext(schema)

    os.makedirs(output_dir, exist_ok=True)

    for enum_name, enum_def in ctx.enums.items():
        jl_code = generate_enum(ctx, enum_def)

        # write to file
        with open(f"{output_dir}/{enum_name}.jl", "w") as file:
            file.write(jl_code)

    for struct_name, struct_def in ctx.structs.items():
        jl_code = generate_struct(ctx, struct_def)

        # write to file
        with open(f"{output_dir}/{struct_name}.jl", "w") as file:
            file.write(jl_code)

    # single include header file
    generate_single_include_file(output_dir, ctx)


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python fastbin_jl.py <schema.json> <output_dir>")
        sys.exit(1)

    schema_file = sys.argv[1]
    output_dir = sys.argv[2]
    generate_jl_code(schema_file, output_dir)
