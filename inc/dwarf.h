#ifndef JOS_DWARF_H
#define JOS_DWARF_H
#include <inc/types.h>
#include <inc/string.h>

/* DWARF4 debugging info */

#define DW_TAG_array_type       0x01
#define DW_TAG_class_type       0x02
#define DW_TAG_entry_point      0x03
#define DW_TAG_enumeration_type 0x04
#define DW_TAG_formal_parameter 0x05
/*  TAG_global_subroutine               0x06 DWARF1 only */
/*  TAG_global_variable                 0x07 DWARF1 only */
#define DW_TAG_imported_declaration 0x08
/*  reserved by DWARF1                  0x09 DWARF1 only */
#define DW_TAG_label         0x0a
#define DW_TAG_lexical_block 0x0b
/*  TAG_local_variable                  0x0c DWARF1 only. */
#define DW_TAG_member 0x0d
/*  reserved by DWARF1                  0x0e DWARF1 only */
#define DW_TAG_pointer_type   0x0f
#define DW_TAG_reference_type 0x10
#define DW_TAG_compile_unit   0x11
#define DW_TAG_string_type    0x12
#define DW_TAG_structure_type 0x13
/* TAG_subroutine                       0x14 DWARF1 only */
#define DW_TAG_subroutine_type        0x15
#define DW_TAG_typedef                0x16
#define DW_TAG_union_type             0x17
#define DW_TAG_unspecified_parameters 0x18
#define DW_TAG_variant                0x19
#define DW_TAG_common_block           0x1a
#define DW_TAG_common_inclusion       0x1b
#define DW_TAG_inheritance            0x1c
#define DW_TAG_inlined_subroutine     0x1d
#define DW_TAG_module                 0x1e
#define DW_TAG_ptr_to_member_type     0x1f
#define DW_TAG_set_type               0x20
#define DW_TAG_subrange_type          0x21
#define DW_TAG_with_stmt              0x22
#define DW_TAG_access_declaration     0x23
#define DW_TAG_base_type              0x24
#define DW_TAG_catch_block            0x25
#define DW_TAG_const_type             0x26
#define DW_TAG_constant               0x27
#define DW_TAG_enumerator             0x28
#define DW_TAG_file_type              0x29
#define DW_TAG_friend                 0x2a
#define DW_TAG_namelist               0x2b
/*  Early releases of this header had the following
            misspelled with a trailing 's' */
#define DW_TAG_namelist_item  0x2c /* DWARF3/2 spelling */
#define DW_TAG_namelist_items 0x2c /* SGI misspelling/typo */
#define DW_TAG_packed_type    0x2d
#define DW_TAG_subprogram     0x2e
/*  The DWARF2 document had two spellings of the following
            two TAGs, DWARF3 specifies the longer spelling. */
#define DW_TAG_template_type_parameter  0x2f /* DWARF3/2 spelling*/
#define DW_TAG_template_type_param      0x2f /* DWARF2   spelling*/
#define DW_TAG_template_value_parameter 0x30 /* DWARF3/2 spelling*/
#define DW_TAG_template_value_param     0x30 /* DWARF2   spelling*/
#define DW_TAG_thrown_type              0x31
#define DW_TAG_try_block                0x32
#define DW_TAG_variant_part             0x33
#define DW_TAG_variable                 0x34
#define DW_TAG_volatile_type            0x35
#define DW_TAG_dwarf_procedure          0x36 /* DWARF3 */
#define DW_TAG_restrict_type            0x37 /* DWARF3 */
#define DW_TAG_interface_type           0x38 /* DWARF3 */
#define DW_TAG_namespace                0x39 /* DWARF3 */
#define DW_TAG_imported_module          0x3a /* DWARF3 */
#define DW_TAG_unspecified_type         0x3b /* DWARF3 */
#define DW_TAG_partial_unit             0x3c /* DWARF3 */
#define DW_TAG_imported_unit            0x3d /* DWARF3 */
                                             /*  Do not use DW_TAG_mutable_type */
#define DW_TAG_mutable_type          0x3e    /* Withdrawn from DWARF3 by DWARF3f. */
#define DW_TAG_condition             0x3f    /* DWARF3f */
#define DW_TAG_shared_type           0x40    /* DWARF3f */
#define DW_TAG_type_unit             0x41    /* DWARF4 */
#define DW_TAG_rvalue_reference_type 0x42    /* DWARF4 */
#define DW_TAG_template_alias        0x43    /* DWARF4 */
#define DW_TAG_coarray_type          0x44    /* DWARF5 */
#define DW_TAG_generic_subrange      0x45    /* DWARF5 */
#define DW_TAG_dynamic_type          0x46    /* DWARF5 */
#define DW_TAG_atomic_type           0x47    /* DWARF5 */
#define DW_TAG_call_site             0x48    /* DWARF5 */
#define DW_TAG_call_site_parameter   0x49    /* DWARF5 */
#define DW_TAG_skeleton_unit         0x4a    /* DWARF5 */
#define DW_TAG_immutable_type        0x4b    /* DWARF5 */
#define DW_TAG_lo_user               0x4080

#define DW_TAG_hi_user 0xffff

/*  The following two are non-standard. Use DW_CHILDREN_yes
    and DW_CHILDREN_no instead.  These could
    probably be deleted, but someone might be using them,
    so they remain.  */
#define DW_children_no  0
#define DW_children_yes 1

#define DW_FORM_addr 0x01
/* FORM_REF                             0x02 DWARF1 only */
#define DW_FORM_block2         0x03
#define DW_FORM_block4         0x04
#define DW_FORM_data2          0x05
#define DW_FORM_data4          0x06
#define DW_FORM_data8          0x07
#define DW_FORM_string         0x08
#define DW_FORM_block          0x09
#define DW_FORM_block1         0x0a
#define DW_FORM_data1          0x0b
#define DW_FORM_flag           0x0c
#define DW_FORM_sdata          0x0d
#define DW_FORM_strp           0x0e
#define DW_FORM_udata          0x0f
#define DW_FORM_ref_addr       0x10
#define DW_FORM_ref1           0x11
#define DW_FORM_ref2           0x12
#define DW_FORM_ref4           0x13
#define DW_FORM_ref8           0x14
#define DW_FORM_ref_udata      0x15
#define DW_FORM_indirect       0x16
#define DW_FORM_sec_offset     0x17 /* DWARF4 */
#define DW_FORM_exprloc        0x18 /* DWARF4 */
#define DW_FORM_flag_present   0x19 /* DWARF4 */
#define DW_FORM_strx           0x1a /* DWARF5 */
#define DW_FORM_addrx          0x1b /* DWARF5 */
#define DW_FORM_ref_sup4       0x1c /* DWARF5 */
#define DW_FORM_strp_sup       0x1d /* DWARF5 */
#define DW_FORM_data16         0x1e /* DWARF5 */
#define DW_FORM_line_strp      0x1f /* DWARF5 */
#define DW_FORM_ref_sig8       0x20 /* DWARF4 */
#define DW_FORM_implicit_const 0x21 /* DWARF5 */
#define DW_FORM_loclistx       0x22 /* DWARF5 */
#define DW_FORM_rnglistx       0x23 /* DWARF5 */
#define DW_FORM_ref_sup8       0x24 /* DWARF5 */
#define DW_FORM_strx1          0x25 /* DWARF5 */
#define DW_FORM_strx2          0x26 /* DWARF5 */
#define DW_FORM_strx3          0x27 /* DWARF5 */
#define DW_FORM_strx4          0x28 /* DWARF5 */
#define DW_FORM_addrx1         0x29 /* DWARF5 */
#define DW_FORM_addrx2         0x2a /* DWARF5 */
#define DW_FORM_addrx3         0x2b /* DWARF5 */
#define DW_FORM_addrx4         0x2c /* DWARF5 */

#define DW_AT_sibling  0x01
#define DW_AT_location 0x02
#define DW_AT_name     0x03
/* reserved DWARF1                              0x04, DWARF1 only */
/* AT_fund_type                                 0x05, DWARF1 only */
/* AT_mod_fund_type                             0x06, DWARF1 only */
/* AT_user_def_type                             0x07, DWARF1 only */
/* AT_mod_u_d_type                              0x08, DWARF1 only */
#define DW_AT_ordering    0x09
#define DW_AT_subscr_data 0x0a
#define DW_AT_byte_size   0x0b
#define DW_AT_bit_offset  0x0c
#define DW_AT_bit_size    0x0d
/* reserved DWARF1                              0x0d, DWARF1 only */
#define DW_AT_element_list     0x0f
#define DW_AT_stmt_list        0x10
#define DW_AT_low_pc           0x11
#define DW_AT_high_pc          0x12
#define DW_AT_language         0x13
#define DW_AT_member           0x14
#define DW_AT_discr            0x15
#define DW_AT_discr_value      0x16
#define DW_AT_visibility       0x17
#define DW_AT_import           0x18
#define DW_AT_string_length    0x19
#define DW_AT_common_reference 0x1a
#define DW_AT_comp_dir         0x1b
#define DW_AT_const_value      0x1c
#define DW_AT_containing_type  0x1d
#define DW_AT_default_value    0x1e
/*  reserved                                    0x1f */
#define DW_AT_inline      0x20
#define DW_AT_is_optional 0x21
#define DW_AT_lower_bound 0x22
/*  reserved                                    0x23 */
/*  reserved                                    0x24 */
#define DW_AT_producer 0x25
/*  reserved                                    0x26 */
#define DW_AT_prototyped 0x27
/*  reserved                                    0x28 */
/*  reserved                                    0x29 */
#define DW_AT_return_addr 0x2a
/*  reserved                                    0x2b */
#define DW_AT_start_scope 0x2c
/*  reserved                                    0x2d */
#define DW_AT_bit_stride  0x2e /* DWARF3 name */
#define DW_AT_stride_size 0x2e /* DWARF2 name */
#define DW_AT_upper_bound 0x2f
/* AT_virtual                                   0x30, DWARF1 only */
#define DW_AT_abstract_origin         0x31
#define DW_AT_accessibility           0x32
#define DW_AT_address_class           0x33
#define DW_AT_artificial              0x34
#define DW_AT_base_types              0x35
#define DW_AT_calling_convention      0x36
#define DW_AT_count                   0x37
#define DW_AT_data_member_location    0x38
#define DW_AT_decl_column             0x39
#define DW_AT_decl_file               0x3a
#define DW_AT_decl_line               0x3b
#define DW_AT_declaration             0x3c
#define DW_AT_discr_list              0x3d /* DWARF2 */
#define DW_AT_encoding                0x3e
#define DW_AT_external                0x3f
#define DW_AT_frame_base              0x40
#define DW_AT_friend                  0x41
#define DW_AT_identifier_case         0x42
#define DW_AT_macro_info              0x43 /* DWARF{234} not DWARF5 */
#define DW_AT_namelist_item           0x44
#define DW_AT_priority                0x45
#define DW_AT_segment                 0x46
#define DW_AT_specification           0x47
#define DW_AT_static_link             0x48
#define DW_AT_type                    0x49
#define DW_AT_use_location            0x4a
#define DW_AT_variable_parameter      0x4b
#define DW_AT_virtuality              0x4c
#define DW_AT_vtable_elem_location    0x4d
#define DW_AT_allocated               0x4e /* DWARF3 */
#define DW_AT_associated              0x4f /* DWARF3 */
#define DW_AT_data_location           0x50 /* DWARF3 */
#define DW_AT_byte_stride             0x51 /* DWARF3f */
#define DW_AT_stride                  0x51 /* DWARF3 (do not use) */
#define DW_AT_entry_pc                0x52 /* DWARF3 */
#define DW_AT_use_UTF8                0x53 /* DWARF3 */
#define DW_AT_extension               0x54 /* DWARF3 */
#define DW_AT_ranges                  0x55 /* DWARF3 */
#define DW_AT_trampoline              0x56 /* DWARF3 */
#define DW_AT_call_column             0x57 /* DWARF3 */
#define DW_AT_call_file               0x58 /* DWARF3 */
#define DW_AT_call_line               0x59 /* DWARF3 */
#define DW_AT_description             0x5a /* DWARF3 */
#define DW_AT_binary_scale            0x5b /* DWARF3f */
#define DW_AT_decimal_scale           0x5c /* DWARF3f */
#define DW_AT_small                   0x5d /* DWARF3f */
#define DW_AT_decimal_sign            0x5e /* DWARF3f */
#define DW_AT_digit_count             0x5f /* DWARF3f */
#define DW_AT_picture_string          0x60 /* DWARF3f */
#define DW_AT_mutable                 0x61 /* DWARF3f */
#define DW_AT_threads_scaled          0x62 /* DWARF3f */
#define DW_AT_explicit                0x63 /* DWARF3f */
#define DW_AT_object_pointer          0x64 /* DWARF3f */
#define DW_AT_endianity               0x65 /* DWARF3f */
#define DW_AT_elemental               0x66 /* DWARF3f */
#define DW_AT_pure                    0x67 /* DWARF3f */
#define DW_AT_recursive               0x68 /* DWARF3f */
#define DW_AT_signature               0x69 /* DWARF4 */
#define DW_AT_main_subprogram         0x6a /* DWARF4 */
#define DW_AT_data_bit_offset         0x6b /* DWARF4 */
#define DW_AT_const_expr              0x6c /* DWARF4 */
#define DW_AT_enum_class              0x6d /* DWARF4 */
#define DW_AT_linkage_name            0x6e /* DWARF4 */
#define DW_AT_string_length_bit_size  0x6f /* DWARF5 */
#define DW_AT_string_length_byte_size 0x70 /* DWARF5 */
#define DW_AT_rank                    0x71 /* DWARF5 */
#define DW_AT_str_offsets_base        0x72 /* DWARF5 */
#define DW_AT_addr_base               0x73 /* DWARF5 */
                                           /* Use DW_AT_rnglists_base, DW_AT_ranges_base is obsolete as */
                                           /* it was only used in some DWARF5 drafts, not the final DWARF5. */
#define DW_AT_rnglists_base 0x74           /* DWARF5 */
                                           /*  DW_AT_dwo_id, an experiment in some DWARF4+. Not DWARF5! */
#define DW_AT_dwo_id                0x75   /* DWARF4!*/
#define DW_AT_dwo_name              0x76   /* DWARF5 */
#define DW_AT_reference             0x77   /* DWARF5 */
#define DW_AT_rvalue_reference      0x78   /* DWARF5 */
#define DW_AT_macros                0x79   /* DWARF5 */
#define DW_AT_call_all_calls        0x7a   /* DWARF5 */
#define DW_AT_call_all_source_calls 0x7b   /* DWARF5 */
#define DW_AT_call_all_tail_calls   0x7c   /* DWARF5 */
#define DW_AT_call_return_pc        0x7d   /* DWARF5 */
#define DW_AT_call_value            0x7e   /* DWARF5 */
#define DW_AT_call_origin           0x7f   /* DWARF5 */
#define DW_AT_call_parameter        0x80   /* DWARF5 */
#define DW_AT_call_pc               0x81   /* DWARF5 */
#define DW_AT_call_tail_call        0x82   /* DWARF5 */
#define DW_AT_call_target           0x83   /* DWARF5 */
#define DW_AT_call_target_clobbered 0x84   /* DWARF5 */
#define DW_AT_call_data_location    0x85   /* DWARF5 */
#define DW_AT_call_data_value       0x86   /* DWARF5 */
#define DW_AT_noreturn              0x87   /* DWARF5 */
#define DW_AT_alignment             0x88   /* DWARF5 */
#define DW_AT_export_symbols        0x89   /* DWARF5 */
#define DW_AT_deleted               0x8a   /* DWARF5 */
#define DW_AT_defaulted             0x8b   /* DWARF5 */
#define DW_AT_loclists_base         0x8c   /* DWARF5 */

#define DW_AT_hi_user 0x3fff

/*
 * DWARF FDE/CIE length field values.
 */
#define DW_EXT_LO      0xfffffff0
#define DW_EXT_HI      0xffffffff
#define DW_EXT_DWARF64 DW_EXT_HI

/* Line number standard opcode name. */
#define DW_LNS_copy               0x01
#define DW_LNS_advance_pc         0x02
#define DW_LNS_advance_line       0x03
#define DW_LNS_set_file           0x04
#define DW_LNS_set_column         0x05
#define DW_LNS_negate_stmt        0x06
#define DW_LNS_set_basic_block    0x07
#define DW_LNS_const_add_pc       0x08
#define DW_LNS_fixed_advance_pc   0x09
#define DW_LNS_set_prologue_end   0x0a /* DWARF3 */
#define DW_LNS_set_epilogue_begin 0x0b /* DWARF3 */
#define DW_LNS_set_isa            0x0c /* DWARF3 */

/* Line number extended opcode name. */
#define DW_LNE_end_sequence      0x01
#define DW_LNE_set_address       0x02
#define DW_LNE_define_file       0x03 /* DWARF4 and earlier only */
#define DW_LNE_set_discriminator 0x04 /* DWARF4 */
#define DW_LNE_lo_user           0x80 /* DWARF3 */
#define DW_LNE_hi_user           0xff /* DWARF3 */

typedef unsigned long long Dwarf_Unsigned;
typedef signed long long Dwarf_Signed;
typedef unsigned long long Dwarf_Off;
typedef unsigned long long Dwarf_Addr;
typedef int Dwarf_Bool;            /* boolean type */
typedef unsigned short Dwarf_Half; /* 2 byte unsigned value */
typedef unsigned char Dwarf_Small; /* 1 byte unsigned value */

struct Dwarf_Addrs {
    const unsigned char *abbrev_begin;
    const unsigned char *abbrev_end;
    const unsigned char *aranges_begin;
    const unsigned char *aranges_end;
    const unsigned char *info_begin;
    const unsigned char *info_end;
    const unsigned char *line_begin;
    const unsigned char *line_end;
    const unsigned char *str_begin;
    const unsigned char *str_end;
    const unsigned char *pubnames_begin;
    const unsigned char *pubnames_end;
    const unsigned char *pubtypes_begin;
    const unsigned char *pubtypes_end;
};

/* Unaligned read from address `addr` */
#define get_unaligned(addr, type) ({            \
    type val;                                   \
    memcpy((void *)&val, (addr), sizeof(type)); \
    val;                                        \
})

/* Write value `val` to unaligned address `addr` */
#define put_unaligned(val, addr) ({                          \
    void *__ptr = (addr);                                    \
    switch (sizeof(*(addr))) {                               \
    case 1:                                                  \
        *(uint8_t *)__ptr = (uint8_t)(uintptr_t)(val);       \
        break;                                               \
    case 2: {                                                \
        uint16_t __var = (uint16_t)(uintptr_t)(val);         \
        memcpy(addr, (const void *)&__var, sizeof(*(addr))); \
    } break;                                                 \
    case 4: {                                                \
        uint32_t __var = (uint32_t)(uintptr_t)(val);         \
        memcpy(addr, (const void *)&__var, sizeof(*(addr))); \
    } break;                                                 \
    case 8: {                                                \
        uint64_t __var = (uint64_t)(uintptr_t)(val);         \
        memcpy(addr, (const void *)&__var, sizeof(*(addr))); \
    } break;                                                 \
    default:                                                 \
        panic("Bad unaligned access");                       \
        break;                                               \
    }                                                        \
})

int info_by_address(const struct Dwarf_Addrs *addrs, uintptr_t p, Dwarf_Off *store);
int file_name_by_info(const struct Dwarf_Addrs *addrs, Dwarf_Off offset, char **buf, Dwarf_Off *line_off);
int line_for_address(const struct Dwarf_Addrs *addrs, uintptr_t p, Dwarf_Off line_offset, int *store);
int function_by_info(const struct Dwarf_Addrs *addrs, uintptr_t p, Dwarf_Off cu_offset, char **buf, uintptr_t *offset);
int address_by_fname(const struct Dwarf_Addrs *addrs, const char *fname, uintptr_t *offset);
int naive_address_by_fname(const struct Dwarf_Addrs *addrs, const char *fname, uintptr_t *offset);

/* dwarf_entry_len - return the length of an FDE or CIE
 *
 * Read the initial_length field of the entry and store the size of
 * the entry in @len. We return the number of bytes read. Return a
 * count of 0 on error
 */
static inline uint32_t
dwarf_entry_len(const uint8_t *addr, uint64_t *len) {
    uint64_t initial_len = get_unaligned(addr, uint32_t);
    uint64_t count = sizeof(uint32_t);

    /* An initial length field value in the range DW_LEN_EXT_LO -
   * DW_LEN_EXT_HI indicates an extension, and should not be
   * interpreted as a length. The only extension that we currently
   * understand is the use of DWARF64 addresses */
    if (initial_len >= DW_EXT_LO && initial_len <= DW_EXT_HI) {
        /* The 64-bit length field immediately follows the
     * compulsory 32-bit length field */
        if (initial_len == DW_EXT_DWARF64) {
            *len = get_unaligned((uint64_t *)addr + sizeof(uint32_t), uint64_t);
            count += sizeof(uint64_t);
        } else {
            cprintf("Unknown DWARF extension\n");
            count = 0;
        }
    } else
        *len = initial_len;

    return count;
}

/* Decode an unsigned LEB128 encoded datum. The algorithm is taken from Appendix C
 * of the DWARF 4 spec. Return the number of bytes read */
static inline uint64_t
dwarf_read_uleb128(const uint8_t *addr, uint64_t *ret) {
    uint64_t result = 0;
    size_t shift = 0, count = 0;
    uint8_t byte;

    do {
        byte = *addr++;
        result |= (byte & 0x7FULL) << shift;
        shift += 7;
        count++;
    } while (byte & 0x80 && shift < 64);

    while (byte & 0x80) {
        byte = *addr++;
        count++;
    }

    *ret = result;
    return count;
}

/* Decode signed LEB128 data. The Algorithm is taken from Appendix C
 * of the DWARF 4 spec. Return the number of bytes read */
static inline uint64_t
dwarf_read_leb128(const char *addr, int64_t *ret) {
    size_t shift = 0, count = 0;
    uint64_t result = 0;
    uint8_t byte;

    do {
        byte = *addr++;
        result |= (byte & 0x7FULL) << shift;
        shift += 7;
        count++;
    } while (byte & 0x80 && shift < 64);

    while (byte & 0x80) {
        byte = *addr++;
        count++;
    }

    /* The number of bits in a signed integer. */
    if (shift < 8 * sizeof(result) && byte & 0x40)
        result |= (-1U << shift);

    *ret = result;
    return count;
}
#endif
