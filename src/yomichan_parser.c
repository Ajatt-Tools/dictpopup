#include "yomichan_parser.h"

#include <zip.h>

#include "messages.h"
#include "util.h"
#include "yyjson.h"

DEFINE_DROP_FUNC(struct zip_file *, zip_fclose)
DEFINE_DROP_FUNC(yyjson_doc *, yyjson_doc_free)

#define error_return(retval, ...)                                                                  \
    do {                                                                                           \
        err(__VA_ARGS__);                                                                          \
        return retval;                                                                             \
    } while (0)

#define return_on(cond)                                                                            \
    do {                                                                                           \
        if (unlikely(cond)) {                                                                      \
            return;                                                                                \
        }                                                                                          \
    } while (0)

enum tag_type { TAG_UNKNOWN, TAG_DIV, TAG_SPAN, TAG_UL, TAG_OL, TAG_LI, TAG_RUBY_RT };

static void append_definition(yyjson_val *val, stringbuilder_s sb[static 1], s8 liststyle,
                              i32 listdepth, bool first);

static void frequency_entry_free(freqentry *fe) {
    frees8(&(fe->word));
    frees8(&(fe->reading));
}

static s8 yyjson_get_s8(yyjson_val *val) {
    return (s8){.s = (u8 *)yyjson_get_str(val), .len = yyjson_get_len(val)};
}

/* ---------- ZIP ---------- */
static zip_t *open_zip(const char *filename) {
    int err = 0;
    zip_t *za = zip_open(filename, 0, &err);
    if (!za) {
        zip_error_t error;
        zip_error_init_with_code(&error, err);
        err("Cannot open zip archive '%s': %s\n", filename, zip_error_strerror(&error));
        zip_error_fini(&error);
        return NULL;
    }

    return za;
}

static s8 unzip_file(zip_t *archive, const char *filename) {
    struct zip_stat finfo;
    zip_stat_init(&finfo);
    if (zip_stat(archive, filename, 0, &finfo) == -1)
        error_return((s8){0}, "Error stat'ing file '%s': %s\n", filename, zip_strerror(archive));

    s8 buffer = news8(finfo.size); // automatically null-terminated

    _drop_(zip_fclose) struct zip_file *index_file = zip_fopen(archive, filename, 0);
    if (!index_file)
        error_return((s8){0}, "Error opening file '%s': %s\n", filename, zip_strerror(archive));

    buffer.len = zip_fread(index_file, buffer.s, buffer.len);
    if (buffer.len == -1)
        error_return((s8){0}, "Could not read file '%s': %s", filename, zip_strerror(archive));
    if ((size_t)buffer.len != finfo.size)
        dbg("Did not read all of %s!", filename);

    return buffer;
}
/* -------------------- */

static s8 parse_liststyletype(s8 lst) {
    // TODO: This is a memory leak
    if (startswith(lst, S("\"")) && endswith(lst, S("\"")))
        return cuttail(cuthead(lst, 1), 1);
    else if (s8equals(lst, S("circle")))
        return S("◦");
    else if (s8equals(lst, S("square")))
        return S("▪");
    else if (s8equals(lst, S("none")))
        return S("  ");
    else
        return lst; // そのまま
}

static void append_structured_content(yyjson_val *obj, stringbuilder_s sb[static 1], s8 liststyle,
                                      i32 listdepth) {
    enum tag_type tag = TAG_UNKNOWN;

    if (!yyjson_is_obj(obj))
        err("Structured content is not an object.");

    size_t idx, max;
    yyjson_val *key, *val;
    yyjson_obj_foreach(obj, idx, max, key, val) {

        assert(yyjson_is_str(key));
        s8 s8key = yyjson_get_s8(key);
        s8 s8val = yyjson_get_s8(val);

        if (s8equals(s8key, S("tag"))) {
            if (s8equals(s8val, S("div")))
                tag = TAG_DIV;
            else if (s8equals(s8val, S("ul"))) {
                tag = TAG_UL;
                listdepth++;

                liststyle = S("•"); // default
            } else if (s8equals(s8val, S("ol"))) {
                tag = TAG_OL;
                listdepth++;
            } else if (s8equals(s8val, S("li"))) {
                tag = TAG_LI;
                // TODO: Default numbers for tag ol
            } else if (s8equals(s8val, S("rt"))) {
                tag = TAG_RUBY_RT;
            } else
                tag = TAG_UNKNOWN;
        } else if ((tag == TAG_UL || tag == TAG_LI) && s8equals(s8key, S("style"))) {
            if (yyjson_is_obj(val)) {

                size_t idx2, max2;
                yyjson_val *key2, *val2;
                yyjson_obj_foreach(val, idx2, max2, key2, val2) {
                    if (yyjson_is_str(key2) && s8equals(yyjson_get_s8(key2), S("listStyleType"))) {
                        liststyle = parse_liststyletype(yyjson_get_s8(val2));
                    }
                }
            } else
                dbg("Could not parse style of list. Skipping..");
        } else if (s8equals(s8key, S("content"))) {
            if (tag == TAG_RUBY_RT) {
                continue;
            }

            if (tag == TAG_DIV || tag == TAG_LI) {
                if (sb->len != 0 && sb->data[sb->len - 1] != '\n')
                    sb_append(sb, S("\n"));
            }

            if (tag == TAG_LI) {
                for (i32 i = 0; i < listdepth - 1; i++)
                    sb_append(sb, S("\t"));
                sb_append(sb, liststyle);
                sb_append(sb, S(" "));
            }

            append_definition(val, sb, liststyle, listdepth, false);
        }
    }
}

static void append_definition(yyjson_val *def, stringbuilder_s sb[static 1], s8 liststyle,
                              i32 listdepth, bool first) {
    if (yyjson_is_arr(def)) {
        size_t idx, max;
        yyjson_val *val;
        yyjson_arr_foreach(def, idx, max, val) {
            if (yyjson_is_str(val)) {
                sb_append(sb, yyjson_get_s8(val));
            } else if (yyjson_is_obj(val)) {
                append_structured_content(val, sb, liststyle, listdepth);
            } else
                dbg("Encountered unknown type within definition.");

            if (first)
                sb_append_char(sb, '\n');
        }
    } else if (yyjson_is_str(def))
        sb_append(sb, yyjson_get_s8(def));
    else if (yyjson_is_obj(def))
        append_structured_content(def, sb, liststyle, listdepth);
    else
        dbg("Encountered unknown type at start of definition.");
}

static s8 parse_definition(yyjson_val *val) {
    stringbuilder_s sb = sb_init(200);

    append_definition(val, &sb, (s8){0}, 0, true);

    s8 def = sb_steals8(sb);
    strip_trailing_whitespace(&def);
    return def;
}

static dictentry parse_dictionary_entry(yyjson_val *arr) {
    if (!yyjson_is_arr(arr)) {
        err("Invalid dictionary format: Entry is not an array.");
    }
    if (yyjson_arr_size(arr) != 8)
        err("Invalid dictionary format: Faulty array size.");

    dictentry de = {0};
    // first
    yyjson_val *val = yyjson_arr_get_first(arr);
    de.kanji = yyjson_get_s8(val);

    // second
    val = unsafe_yyjson_get_next(val);
    de.reading = yyjson_get_s8(val);

    // third
    val = unsafe_yyjson_get_next(val);
    // Skip

    // fourth
    val = unsafe_yyjson_get_next(val);
    // Skip

    // fifth
    val = unsafe_yyjson_get_next(val);
    // Skip

    // sixth
    val = unsafe_yyjson_get_next(val);
    de.definition = parse_definition(val);

    // seventh
    val = unsafe_yyjson_get_next(val);
    // Skip

    // eighth
    val = unsafe_yyjson_get_next(val);
    // Skip

    return de;
}

static _nonnull_n_(3) void parse_yomichan_dictentries_from_buffer(
    const s8 buffer, const s8 dictname, void (*foreach_dictentry)(void *, dictentry),
    void *userdata_de) {
    _drop_(yyjson_doc_free) yyjson_doc *doc = yyjson_read((char *)buffer.s, buffer.len, 0);
    if (!doc)
        return;

    yyjson_val *root = yyjson_doc_get_root(doc);
    if (!yyjson_is_arr(root)) {
        err("Invalid dictionary format: Dictionary not consisting of an array.");
        return;
    }

    size_t idx, max;
    yyjson_val *entry;
    yyjson_arr_foreach(root, idx, max, entry) {
        if (unlikely(!yyjson_is_arr(entry)))
            dbg("Entry is not an array. Skipping..");

        dictentry de = parse_dictionary_entry(entry);
        de.dictname = dictname;
        (*foreach_dictentry)(userdata_de, de);
        frees8(&de.definition);
    }
}

static freqentry parse_single_frequency_entry(yyjson_val *arr) {
    freqentry fe = {0};

    /* first entry */
    yyjson_val *firstval = yyjson_arr_get_first(arr);
    fe.word = yyjson_get_s8(firstval);
    /* ----------- */

    /* second entry */
    yyjson_val *secondval = unsafe_yyjson_get_next(firstval);
    assert(yyjson_is_str(secondval));
    assert(s8equals(yyjson_get_s8(secondval), S("freq")));
    /* ----------- */

    /* third entry */
    yyjson_val *thirdval = unsafe_yyjson_get_next(secondval);
    if (yyjson_is_obj(thirdval)) {
        size_t idx, max;
        yyjson_val *objkey, *objval;
        yyjson_obj_foreach(thirdval, idx, max, objkey, objval) {
            if (yyjson_is_str(objkey)) {
                s8 s8key = yyjson_get_s8(objkey);
                if (s8equals(s8key, S("reading"))) {
                    assert(yyjson_is_str(objval));
                    fe.reading = yyjson_get_s8(objval);
                } else if (s8equals(s8key, S("frequency"))) {
                    assert(yyjson_is_uint(objval));
                    fe.frequency = yyjson_get_uint(objval);
                } else
                    dbg("Skipping unknown frequency key: '%s'", (char *)s8key.s);
            } else
                dbg("Skipping unknown frequency key");
        }
    } else if (yyjson_is_uint(thirdval))
        fe.frequency = yyjson_get_uint(thirdval);
    else
        dbg("Unexpected frequency entry format");

    return fe;
}

static _nonnull_n_(2) void parse_yomichan_freqentries_from_buffer(
    const s8 buffer, void (*foreach_frequency_entry)(void *, freqentry), void *userdata_fe) {
    _drop_(yyjson_doc_free) yyjson_doc *doc = yyjson_read((char *)buffer.s, buffer.len, 0);
    if (!doc)
        return;

    yyjson_val *root = yyjson_doc_get_root(doc);
    if (!yyjson_is_arr(root)) {
        err("Invalid dictionary format: Dictionary not consisting of an array.");
        return;
    }

    size_t idx, max;
    yyjson_val *entry;
    yyjson_arr_foreach(root, idx, max, entry) {
        if (!yyjson_is_arr(entry))
            dbg("Frequency entry is not an array. Skipping..");

        freqentry fe = parse_single_frequency_entry(entry);
        (*foreach_frequency_entry)(userdata_fe, fe);
    }
}

static s8 extract_dictname_from_buffer(s8 index) {
    _drop_(yyjson_doc_free) yyjson_doc *doc = yyjson_read((char *)index.s, index.len, 0);
    yyjson_val *root = yyjson_doc_get_root(doc);
    if (!yyjson_is_obj(root)) {
        err("Index file has invalid formating.");
        return (s8){0};
    }
    yyjson_val *val = yyjson_obj_getn(root, "title", lengthof("title"));
    if (!val) {
        err("Could not find dictionary name in index.json");
        return (s8){0};
    }
    if (!yyjson_is_str(val)) {
        err("Dictionary name is not a string!");
        return (s8){0};
    }

    return s8dup(fromcstr_((char *)yyjson_get_str(val)));
}

static s8 _nonnull_ extract_dictname_from_zip(const char *zipfn, zip_t *zipfile) {
    _drop_(frees8) s8 index = unzip_file(zipfile, "index.json");
    if (!index.len) {
        dbg("Zipfile '%s' does not contain an index.json file."
            " Assuming it is not a Yomichan dictionary file..",
            zipfn);
        return (s8){0};
    }

    s8 dictname = extract_dictname_from_buffer(index);
    if (!dictname.len) {
        dbg("Could not parse a dictionary from the index.json of zipfile '%s'."
            " Assuming it is not a Yomichan dictionary file..",
            zipfn);
    }
    return dictname;
}

void parse_yomichan_dict(const char *zipfn, void (*foreach_dictentry)(void *, dictentry),
                         void *userdata_de, void (*forach_freqentry)(void *, freqentry),
                         void *userdata_fe) {
    zip_t *zipfile = open_zip(zipfn);
    return_on(!zipfile);

    _drop_(frees8) s8 dictname = extract_dictname_from_zip(zipfn, zipfile);
    return_on(!dictname.len);
    printf("Processing dictionary: %s\n", (char *)dictname.s);

    struct zip_stat finfo;
    zip_stat_init(&finfo);
    for (int i = 0; (zip_stat_index(zipfile, i, 0, &finfo)) == 0; i++) {
        s8 fn = fromcstr_((char *)finfo.name);
        if (startswith(fn, S("term_bank_"))) {
            _drop_(frees8) s8 buffer = unzip_file(zipfile, finfo.name);
            if (buffer.len)
                parse_yomichan_dictentries_from_buffer(buffer, dictname, foreach_dictentry,
                                                       userdata_de);
        } else if (startswith(fn, S("term_meta_bank"))) {
            _drop_(frees8) s8 buffer = unzip_file(zipfile, finfo.name);
            if (buffer.len)
                parse_yomichan_freqentries_from_buffer(buffer, forach_freqentry, userdata_fe);
        }
    }

    zip_discard(zipfile);
}