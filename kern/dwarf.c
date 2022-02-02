#include <inc/assert.h>
#include <inc/error.h>
#include <inc/dwarf.h>
#include <inc/string.h>
#include <inc/types.h>
#include <inc/stdio.h>

struct Slice {
    const void *mem;
    int len;
};

static int
info_by_address_debug_aranges(const struct Dwarf_Addrs *addrs, uintptr_t p, Dwarf_Off *store) {
    const uint8_t *set = addrs->aranges_begin;
    while ((unsigned char *)set < addrs->aranges_end) {
        int count = 0;
        unsigned long len;
        const uint8_t *header = set;
        set += dwarf_entry_len(set, &len);
        if (!count) return -E_BAD_DWARF;
        const uint8_t *set_end = set + len;

        /* Parse compilation unit header */
        Dwarf_Half version = get_unaligned(set, Dwarf_Half);
        assert(version == 2);
        set += sizeof(Dwarf_Half);
        Dwarf_Off offset = get_unaligned(set, uint32_t);
        set += sizeof(uint32_t);
        Dwarf_Small address_size = get_unaligned(set, Dwarf_Small);
        assert(address_size == 8);
        set += sizeof(Dwarf_Small);
        Dwarf_Small segment_size = get_unaligned(set, Dwarf_Small);
        set += sizeof(Dwarf_Small);
        assert(!segment_size);

        void *addr = NULL;
        uint32_t entry_size = 2 * address_size + segment_size;
        uint32_t remainder = (set - header) % entry_size;
        if (remainder) set += 2 * address_size - remainder;

        Dwarf_Off size = 0;
        do {
            addr = (void *)get_unaligned(set, uintptr_t);
            set += address_size;
            size = get_unaligned(set, uint32_t);
            set += address_size;
            if ((uintptr_t)addr <= p && p <= (uintptr_t)addr + size) {
                *store = offset;
                return 0;
            }
        } while (set < set_end);
        assert(set == set_end);
    }
    return -E_BAD_DWARF;
}

/* Read value from .debug_abbrev table in buf. Returns number of bytes read */
static int
dwarf_read_abbrev_entry(const void *entry, unsigned form, void *buf, int bufsize, size_t address_size) {
    int bytes = 0;
    switch (form) {
    case DW_FORM_addr: {
        uintptr_t data = 0;
        memcpy(&data, entry, address_size);
        entry += address_size;
        if (buf && bufsize >= sizeof(uintptr_t))
            put_unaligned(data, (uintptr_t *)buf);
        bytes = address_size;
    } break;
    case DW_FORM_block2: {
        /* Read block of 2-byte length followed by 0 to 65535 contiguous information bytes */
        // LAB 2: Your code here
    } break;
    case DW_FORM_block4: {
        uint32_t length = get_unaligned(entry, uint32_t);
        entry += sizeof(uint32_t);
        struct Slice slice = {
                .mem = entry,
                .len = length,
        };
        if (buf) memcpy(buf, &slice, sizeof(struct Slice));
        entry += length;
        bytes = sizeof(uint32_t) + length;
    } break;
    case DW_FORM_data2: {
        Dwarf_Half data = get_unaligned(entry, Dwarf_Half);
        entry += sizeof(Dwarf_Half);
        if (buf && bufsize >= sizeof(Dwarf_Half))
            put_unaligned(data, (Dwarf_Half *)buf);
        bytes = sizeof(Dwarf_Half);
    } break;
    case DW_FORM_data4: {
        uint32_t data = get_unaligned(entry, uint32_t);
        entry += sizeof(uint32_t);
        if (buf && bufsize >= sizeof(uint32_t))
            put_unaligned(data, (uint32_t *)buf);
        bytes = sizeof(uint32_t);
    } break;
    case DW_FORM_data8: {
        uint64_t data = get_unaligned(entry, uint64_t);
        entry += sizeof(uint64_t);
        if (buf && bufsize >= sizeof(uint64_t))
            put_unaligned(data, (uint64_t *)buf);
        bytes = sizeof(uint64_t);
    } break;
    case DW_FORM_string: {
        if (buf && bufsize >= sizeof(char *))
            memcpy(buf, &entry, sizeof(char *));
        bytes = strlen(entry) + 1;
    } break;
    case DW_FORM_block: {
        uint64_t length = 0;
        uint32_t count = dwarf_read_uleb128(entry, &length);
        entry += count;
        struct Slice slice = {
                .mem = entry,
                .len = length,
        };
        if (buf) memcpy(buf, &slice, sizeof(struct Slice));
        entry += length;
        bytes = count + length;
    } break;
    case DW_FORM_block1: {
        uint32_t length = get_unaligned(entry, Dwarf_Small);
        entry += sizeof(Dwarf_Small);
        struct Slice slice = {
                .mem = entry,
                .len = length,
        };
        if (buf) memcpy(buf, &slice, sizeof(struct Slice));
        entry += length;
        bytes = length + sizeof(Dwarf_Small);
    } break;
    case DW_FORM_data1: {
        Dwarf_Small data = get_unaligned(entry, Dwarf_Small);
        entry += sizeof(Dwarf_Small);
        if (buf && bufsize >= sizeof(Dwarf_Small)) {
            put_unaligned(data, (Dwarf_Small *)buf);
        }
        bytes = sizeof(Dwarf_Small);
    } break;
    case DW_FORM_flag: {
        bool data = get_unaligned(entry, Dwarf_Small);
        entry += sizeof(Dwarf_Small);
        if (buf && bufsize >= sizeof(bool)) {
            put_unaligned(data, (bool *)buf);
        }
        bytes = sizeof(Dwarf_Small);
    } break;
    case DW_FORM_sdata: {
        int64_t data = 0;
        uint32_t count = dwarf_read_leb128(entry, &data);
        entry += count;
        if (buf && bufsize >= sizeof(int32_t))
            put_unaligned(data, (int32_t *)buf);
        bytes = count;
    } break;
    case DW_FORM_strp: {
        uint64_t length = 0;
        uint32_t count = dwarf_entry_len(entry, &length);
        entry += count;
        if (buf && bufsize >= sizeof(uint64_t))
            put_unaligned(length, (uint64_t *)buf);
        bytes = count;
    } break;
    case DW_FORM_udata: {
        uint64_t data = 0;
        uint32_t count = dwarf_read_uleb128(entry, &data);
        entry += count;
        if (buf && bufsize >= sizeof(uint32_t))
            put_unaligned(data, (uint32_t *)buf);
        bytes = count;
    } break;
    case DW_FORM_ref_addr: {
        uint64_t length = 0;
        uint32_t count = dwarf_entry_len(entry, &length);
        entry += count;
        if (buf && bufsize >= sizeof(uint64_t))
            put_unaligned(length, (uint64_t *)buf);
        bytes = count;
    } break;
    case DW_FORM_ref1: {
        Dwarf_Small data = get_unaligned(entry, Dwarf_Small);
        entry += sizeof(Dwarf_Small);
        if (buf && bufsize >= sizeof(Dwarf_Small))
            put_unaligned(data, (Dwarf_Small *)buf);
        bytes = sizeof(Dwarf_Small);
    } break;
    case DW_FORM_ref2: {
        Dwarf_Half data = get_unaligned(entry, Dwarf_Half);
        entry += sizeof(Dwarf_Half);
        if (buf && bufsize >= sizeof(Dwarf_Half))
            put_unaligned(data, (Dwarf_Half *)buf);
        bytes = sizeof(Dwarf_Half);
    } break;
    case DW_FORM_ref4: {
        uint32_t data = get_unaligned(entry, uint32_t);
        entry += sizeof(uint32_t);
        if (buf && bufsize >= sizeof(uint32_t))
            put_unaligned(data, (uint32_t *)buf);
        bytes = sizeof(uint32_t);
    } break;
    case DW_FORM_ref8: {
        uint64_t data = get_unaligned(entry, uint64_t);
        entry += sizeof(uint64_t);
        if (buf && bufsize >= sizeof(uint64_t))
            put_unaligned(data, (uint64_t *)buf);
        bytes = sizeof(uint64_t);
    } break;
    case DW_FORM_ref_udata: {
        uint64_t data = 0;
        uint32_t count = dwarf_read_uleb128(entry, &data);
        entry += count;
        if (buf && bufsize >= sizeof(unsigned int))
            put_unaligned(data, (unsigned int *)buf);
        bytes = count;
    } break;
    case DW_FORM_indirect: {
        uint64_t form = 0;
        uint32_t count = dwarf_read_uleb128(entry, &form);
        entry += count;
        uint32_t read = dwarf_read_abbrev_entry(entry, form, buf, bufsize, address_size);
        bytes = count + read;
    } break;
    case DW_FORM_sec_offset: {
        uint64_t length = 0;
        uint32_t count = dwarf_entry_len(entry, &length);
        entry += count;
        if (buf && bufsize >= sizeof(unsigned long))
            put_unaligned(length, (unsigned long *)buf);
        bytes = count;
    } break;
    case DW_FORM_exprloc: {
        uint64_t length = 0;
        uint64_t count = dwarf_read_uleb128(entry, &length);
        entry += count;
        if (buf) memcpy(buf, entry, MIN(length, bufsize));
        entry += length;
        bytes = count + length;
    } break;
    case DW_FORM_flag_present:
        if (buf && sizeof(buf) >= sizeof(bool)) {
            put_unaligned(true, (bool *)buf);
        }
        bytes = 0;
        break;
    case DW_FORM_ref_sig8: {
        uint64_t data = get_unaligned(entry, uint64_t);
        entry += sizeof(uint64_t);
        if (buf && bufsize >= sizeof(uint64_t))
            put_unaligned(data, (uint64_t *)buf);
        bytes = sizeof(uint64_t);
    } break;
    }
    return bytes;
}

/* Find a compilation unit, which contains given address from .debug_info section */
static int
info_by_address_debug_info(const struct Dwarf_Addrs *addrs, uintptr_t p, Dwarf_Off *store) {
    const uint8_t *entry = addrs->info_begin;

    while (entry < addrs->info_end) {
        const uint8_t *header = entry;

        uint32_t count;
        uint64_t len = 0;
        entry += count = dwarf_entry_len(entry, &len);
        if (!count) return -E_BAD_DWARF;

        const uint8_t *entry_end = entry + len;

        /* Parse compilation unit header */
        Dwarf_Half version = get_unaligned(entry, Dwarf_Half);
        entry += sizeof(Dwarf_Half);
        assert(version == 4 || version == 2);
        Dwarf_Off abbrev_offset = get_unaligned(entry, uint32_t);
        entry += sizeof(uint32_t);
        Dwarf_Small address_size = get_unaligned(entry, Dwarf_Small);
        entry += sizeof(Dwarf_Small);
        assert(address_size == sizeof(uintptr_t));

        /* Read abbreviation code */
        uint64_t abbrev_code = 0;
        entry += dwarf_read_uleb128(entry, &abbrev_code);
        assert(abbrev_code);

        /* Read abbreviations table */
        const uint8_t *abbrev_entry = addrs->abbrev_begin + abbrev_offset;
        uint64_t table_abbrev_code = 0;
        abbrev_entry += dwarf_read_uleb128(abbrev_entry, &table_abbrev_code);
        assert(table_abbrev_code == abbrev_code);
        uint64_t tag = 0;
        abbrev_entry += dwarf_read_uleb128(abbrev_entry, &tag);
        assert(tag == DW_TAG_compile_unit);
        abbrev_entry += sizeof(Dwarf_Small);

        uint64_t name = 0, form = 0;
        uintptr_t low_pc = 0, high_pc = 0;
        do {
            abbrev_entry += dwarf_read_uleb128(abbrev_entry, &name);
            abbrev_entry += dwarf_read_uleb128(abbrev_entry, &form);
            if (name == DW_AT_low_pc) {
                entry += dwarf_read_abbrev_entry(entry, form, &low_pc, sizeof(low_pc), address_size);
            } else if (name == DW_AT_high_pc) {
                entry += dwarf_read_abbrev_entry(entry, form, &high_pc, sizeof(high_pc), address_size);
                if (form != DW_FORM_addr) high_pc += low_pc;
            } else {
                entry += dwarf_read_abbrev_entry(entry, form, NULL, 0, address_size);
            }
        } while (name || form);

        if (p >= low_pc && p <= high_pc) {
            *store = (const unsigned char *)header - addrs->info_begin;
            return 0;
        }

        entry = entry_end;
    }
    return -E_NO_ENT;
}

int
info_by_address(const struct Dwarf_Addrs *addrs, uintptr_t addr, Dwarf_Off *store) {
    int res = info_by_address_debug_aranges(addrs, addr, store);
    if (res < 0) res = info_by_address_debug_info(addrs, addr, store);
    return res;
}

int
file_name_by_info(const struct Dwarf_Addrs *addrs, Dwarf_Off offset, char **buf, Dwarf_Off *line_off) {
    if (offset > addrs->info_end - addrs->info_begin) return -E_INVAL;

    const uint8_t *entry = addrs->info_begin + offset;
    uint32_t count;
    uint64_t len = 0;
    entry += count = dwarf_entry_len(entry, &len);
    if (!count) return -E_BAD_DWARF;

    /* Parse compilation unit header */
    Dwarf_Half version = get_unaligned(entry, Dwarf_Half);
    entry += sizeof(Dwarf_Half);
    assert(version == 4 || version == 2);
    Dwarf_Off abbrev_offset = get_unaligned(entry, uint32_t);
    entry += sizeof(uint32_t);
    Dwarf_Small address_size = get_unaligned(entry, Dwarf_Small);
    entry += sizeof(Dwarf_Small);
    assert(address_size == sizeof(uintptr_t));

    /* Read abbreviation code */
    uint64_t abbrev_code = 0;
    entry += dwarf_read_uleb128(entry, &abbrev_code);
    assert(abbrev_code);

    /* Read abbreviations table */
    const uint8_t *abbrev_entry = addrs->abbrev_begin + abbrev_offset;
    uint64_t table_abbrev_code = 0;
    abbrev_entry += dwarf_read_uleb128(abbrev_entry, &table_abbrev_code);
    assert(table_abbrev_code == abbrev_code);
    uint64_t tag = 0;
    abbrev_entry += dwarf_read_uleb128(abbrev_entry, &tag);
    assert(tag == DW_TAG_compile_unit);
    abbrev_entry += sizeof(Dwarf_Small);

    uint64_t name = 0, form = 0;
    do {
        abbrev_entry += dwarf_read_uleb128(abbrev_entry, &name);
        abbrev_entry += dwarf_read_uleb128(abbrev_entry, &form);
        if (name == DW_AT_name) {
            if (form == DW_FORM_strp) {
                uint64_t offset = 0;
                entry += dwarf_read_abbrev_entry(entry, form, &offset, sizeof(uint64_t), address_size);
                if (buf) put_unaligned((const uint8_t *)addrs->str_begin + offset, buf);
            } else {
                entry += dwarf_read_abbrev_entry(entry, form, buf, sizeof(char *), address_size);
            }
        } else if (name == DW_AT_stmt_list) {
            entry += dwarf_read_abbrev_entry(entry, form, line_off, sizeof(Dwarf_Off), address_size);
        } else {
            entry += dwarf_read_abbrev_entry(entry, form, NULL, 0, address_size);
        }
    } while (name || form);

    return 0;
}

int
function_by_info(const struct Dwarf_Addrs *addrs, uintptr_t p, Dwarf_Off cu_offset, char **buf, uintptr_t *offset) {
    uint64_t len = 0;
    uint32_t count;

    const void *entry = addrs->info_begin + cu_offset;
    entry += count = dwarf_entry_len(entry, &len);
    if (!count) return -E_BAD_DWARF;

    const void *entry_end = entry + len;

    /* Parse compilation unit header */
    Dwarf_Half version = get_unaligned(entry, Dwarf_Half);
    entry += sizeof(Dwarf_Half);
    assert(version == 4 || version == 2);
    Dwarf_Off abbrev_offset = get_unaligned(entry, uint32_t);
    entry += sizeof(uint32_t);
    Dwarf_Small address_size = get_unaligned(entry, Dwarf_Small);
    entry += sizeof(Dwarf_Small);
    assert(address_size == sizeof(uintptr_t));

    /* Parse abbrev and info sections */
    uint64_t abbrev_code = 0;
    uint64_t table_abbrev_code = 0;
    const uint8_t *abbrev_entry = addrs->abbrev_begin + abbrev_offset;

    while (entry < entry_end) {
        /* Read info abbreviation code */
        entry += dwarf_read_uleb128(entry, &abbrev_code);
        if (!abbrev_code) continue;

        const uint8_t *curr_abbrev_entry = abbrev_entry;
        uint64_t name = 0, form = 0, tag = 0;

        /* Find abbreviation in abbrev section */
        /* UNSAFE Needs to be replaced */
        while (curr_abbrev_entry < addrs->abbrev_end) {
            curr_abbrev_entry += dwarf_read_uleb128(curr_abbrev_entry, &table_abbrev_code);
            curr_abbrev_entry += dwarf_read_uleb128(curr_abbrev_entry, &tag);
            curr_abbrev_entry += sizeof(Dwarf_Small);
            if (table_abbrev_code == abbrev_code) break;

            /* Skip attributes */
            do {
                curr_abbrev_entry += dwarf_read_uleb128(curr_abbrev_entry, &name);
                curr_abbrev_entry += dwarf_read_uleb128(curr_abbrev_entry, &form);
            } while (name != 0 || form != 0);
        }
        /* Parse subprogram DIE */
        if (tag == DW_TAG_subprogram) {
            uintptr_t low_pc = 0, high_pc = 0;
            const uint8_t *fn_name_entry = 0;
            uint64_t name_form = 0;
            do {
                curr_abbrev_entry += dwarf_read_uleb128(curr_abbrev_entry, &name);
                curr_abbrev_entry += dwarf_read_uleb128(curr_abbrev_entry, &form);
                if (name == DW_AT_low_pc) {
                    entry += dwarf_read_abbrev_entry(entry, form, &low_pc, sizeof(low_pc), address_size);
                } else if (name == DW_AT_high_pc) {
                    entry += dwarf_read_abbrev_entry(entry, form, &high_pc, sizeof(high_pc), address_size);
                    if (form != DW_FORM_addr) high_pc += low_pc;
                } else {
                    if (name == DW_AT_name) {
                        fn_name_entry = entry;
                        name_form = form;
                    }
                    entry += dwarf_read_abbrev_entry(entry, form, NULL, 0, address_size);
                }
            } while (name || form);

            /* Load info and finish if address is inside of the function */
            if (p >= low_pc && p <= high_pc) {
                *offset = low_pc;
                if (name_form == DW_FORM_strp) {
                    uintptr_t str_offset = 0;
                    entry += dwarf_read_abbrev_entry(fn_name_entry, name_form, &str_offset, sizeof(uintptr_t), address_size);
                    (void)entry;
                    if (buf) put_unaligned((const uint8_t *)addrs->str_begin + str_offset, buf);
                } else {
                    entry += dwarf_read_abbrev_entry(fn_name_entry, name_form, buf, sizeof(uint8_t *), address_size);
                    (void)entry;
                }
                return 0;
            }
        } else {
            /* Skip if not a subprogram */
            do {
                curr_abbrev_entry += dwarf_read_uleb128(curr_abbrev_entry, &name);
                curr_abbrev_entry += dwarf_read_uleb128(curr_abbrev_entry, &form);
                entry += dwarf_read_abbrev_entry(entry, form, NULL, 0, address_size);
            } while (name || form);
        }
    }
    return -E_NO_ENT;
}

int
address_by_fname(const struct Dwarf_Addrs *addrs, const char *fname, uintptr_t *offset) {
    const int flen = strlen(fname);
    if (!flen) return -E_INVAL;

    const uint8_t *pubnames_entry = addrs->pubnames_begin;
    uint32_t count = 0;
    uint64_t len = 0;
    Dwarf_Off cu_offset = 0, func_offset = 0;

    /* parse pubnames section */
    while (pubnames_entry < addrs->pubnames_end) {
        count = dwarf_entry_len(pubnames_entry, &len);
        if (!count) return -E_BAD_DWARF;
        pubnames_entry += count;

        const uint8_t *pubnames_entry_end = pubnames_entry + len;
        Dwarf_Half version = get_unaligned(pubnames_entry, Dwarf_Half);
        assert(version == 2);
        pubnames_entry += sizeof(Dwarf_Half);
        cu_offset = get_unaligned(pubnames_entry, uint32_t);
        pubnames_entry += sizeof(uint32_t);
        count = dwarf_entry_len(pubnames_entry, &len);
        pubnames_entry += count;

        while (pubnames_entry < pubnames_entry_end) {
            func_offset = get_unaligned(pubnames_entry, uint32_t);
            pubnames_entry += sizeof(uint32_t);

            if (!func_offset) break;

            if (!strcmp(fname, (const char *)pubnames_entry)) {
                /* Parse compilation unit header */
                const uint8_t *entry = addrs->info_begin + cu_offset;
                const uint8_t *func_entry = entry + func_offset;
                entry += count = dwarf_entry_len(entry, &len);
                if (!count) return -E_BAD_DWARF;

                Dwarf_Half version = get_unaligned(entry, Dwarf_Half);
                assert(version == 4 || version == 2);
                entry += sizeof(Dwarf_Half);
                Dwarf_Off abbrev_offset = get_unaligned(entry, uint32_t);
                entry += sizeof(uint32_t);
                const uint8_t *abbrev_entry = addrs->abbrev_begin + abbrev_offset;
                Dwarf_Small address_size = get_unaligned(entry, Dwarf_Small);
                assert(address_size == sizeof(uintptr_t));

                entry = func_entry;
                uint64_t abbrev_code = 0, table_abbrev_code = 0;
                entry += dwarf_read_uleb128(entry, &abbrev_code);
                uint64_t name = 0, form = 0, tag = 0;

                /* Find abbreviation in abbrev section */
                /* UNSAFE Needs to be replaced */
                while (abbrev_entry < addrs->abbrev_end) {
                    abbrev_entry += dwarf_read_uleb128(abbrev_entry, &table_abbrev_code);
                    abbrev_entry += dwarf_read_uleb128(abbrev_entry, &tag);
                    abbrev_entry += sizeof(Dwarf_Small);
                    if (table_abbrev_code == abbrev_code) break;

                    /* skip attributes */
                    do {
                        abbrev_entry += dwarf_read_uleb128(abbrev_entry, &name);
                        abbrev_entry += dwarf_read_uleb128(abbrev_entry, &form);
                    } while (name || form);
                }
                /* Find low_pc */
                if (tag == DW_TAG_subprogram) {
                    /* At this point entry points to the beginning of function's DIE attributes
                     * and abbrev_entry points to abbreviation table entry corresponding to this DIE.
                     * Abbreviation table entry consists of pairs of unsigned LEB128 numbers, the first
                     * encodes name of attribute and the second encodes its form. Attribute entry ends
                     * with a pair where both name and form equal zero.
                     * Address of a function is encoded in attribute with name DW_AT_low_pc.
                     * To find it, we need to scan both abbreviation table and attribute values.
                     * You can read unsigned LEB128 number using dwarf_read_uleb128 function.
                     * Attribute value can be obtained using dwarf_read_abbrev_entry function. */
                    uintptr_t low_pc = 0;
                    // LAB 3: Your code here:


                    *offset = low_pc;
                } else {
                    /* Skip if not a subprogram or label */
                    do {
                        abbrev_entry += dwarf_read_uleb128(abbrev_entry, &name);
                        abbrev_entry += dwarf_read_uleb128(abbrev_entry, &form);
                        entry += dwarf_read_abbrev_entry(entry, form, NULL, 0, address_size);
                    } while (name || form);
                }
                return 0;
            }
            pubnames_entry += strlen((const char *)pubnames_entry) + 1;
        }
    }
    return -E_NO_ENT;
}

int
naive_address_by_fname(const struct Dwarf_Addrs *addrs, const char *fname, uintptr_t *offset) {
    const int flen = strlen(fname);
    if (!flen) return -E_INVAL;

    for (const uint8_t *entry = addrs->info_begin; (const unsigned char *)entry < addrs->info_end;) {
        uint64_t len = 0;
        uint32_t count = dwarf_entry_len(entry, &len);
        entry += count;
        if (!count) return -E_BAD_DWARF;

        const uint8_t *entry_end = entry + len;

        /* Parse compilation unit header */
        Dwarf_Half version = get_unaligned(entry, Dwarf_Half);
        entry += sizeof(Dwarf_Half);
        assert(version == 4 || version == 2);
        Dwarf_Off abbrev_offset = get_unaligned(entry, uint32_t);
        /**/ entry += sizeof(uint32_t);
        Dwarf_Small address_size = get_unaligned(entry, Dwarf_Small);
        entry += sizeof(Dwarf_Small);
        assert(address_size == sizeof(uintptr_t));

        /* Parse related DIE's */
        uint64_t abbrev_code = 0, table_abbrev_code = 0;
        const uint8_t *abbrev_entry = addrs->abbrev_begin + abbrev_offset;

        while (entry < entry_end) {
            /* Read info abbreviation code */
            count = dwarf_read_uleb128(entry, &abbrev_code);
            entry += count;
            if (!abbrev_code) continue;

            /* Find abbreviation in abbrev section */
            /* UNSAFE, Needs to be replaced */
            const uint8_t *curr_abbrev_entry = abbrev_entry;
            uint64_t name = 0, form = 0, tag = 0;
            while ((const unsigned char *)curr_abbrev_entry < addrs->abbrev_end) {
                curr_abbrev_entry += dwarf_read_uleb128(curr_abbrev_entry, &table_abbrev_code);
                curr_abbrev_entry += dwarf_read_uleb128(curr_abbrev_entry, &tag);
                curr_abbrev_entry += sizeof(Dwarf_Small);
                if (table_abbrev_code == abbrev_code) break;

                /* skip attributes */
                do {
                    curr_abbrev_entry += dwarf_read_uleb128(curr_abbrev_entry, &name);
                    curr_abbrev_entry += dwarf_read_uleb128(curr_abbrev_entry, &form);
                } while (name || form);
            }
            /* parse subprogram or label DIE */
            if (tag == DW_TAG_subprogram || tag == DW_TAG_label) {
                uintptr_t low_pc = 0;
                bool found = 0;
                do {
                    curr_abbrev_entry += dwarf_read_uleb128(curr_abbrev_entry, &name);
                    curr_abbrev_entry += dwarf_read_uleb128(curr_abbrev_entry, &form);
                    if (name == DW_AT_low_pc) {
                        entry += dwarf_read_abbrev_entry(entry, form, &low_pc, sizeof(low_pc), address_size);
                    } else if (name == DW_AT_name) {
                        if (form == DW_FORM_strp) {
                            uint64_t str_offset = 0;
                            entry += dwarf_read_abbrev_entry(entry, form, &str_offset, sizeof(uint64_t), address_size);
                            if (!strcmp(fname, (const char *)addrs->str_begin + str_offset)) found = 1;
                        } else {
                            if (!strcmp(fname, (const char *)entry)) found = 1;
                            entry += dwarf_read_abbrev_entry(entry, form, NULL, 0, address_size);
                        }
                    } else
                        entry += dwarf_read_abbrev_entry(entry, form, NULL, 0, address_size);
                } while (name || form);
                if (found) {
                    /* finish if fname found */
                    *offset = low_pc;
                    return 0;
                }
            } else {
                /* Skip if not a subprogram or label */
                do {
                    curr_abbrev_entry += dwarf_read_uleb128(curr_abbrev_entry, &name);
                    curr_abbrev_entry += dwarf_read_uleb128(curr_abbrev_entry, &form);
                    entry += dwarf_read_abbrev_entry(entry, form, NULL, 0, address_size);
                } while (name || form);
            }
        }
    }

    return -E_NO_ENT;
}
