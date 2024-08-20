#include "jppron/ajt_audio_index_parser.h"

#include "utils/str.h"

#include "utils/messages.h"
#include "utils/util.h"
#include "yyjson.h"
#include <deinflector.h>
#include <errno.h>

typedef struct {
    s8 root_path;
    s8 name;
    s8 media_dir;
} index_meta;

DEFINE_DROP_FUNC(FILE *, fclose)
DEFINE_DROP_FUNC(yyjson_doc *, yyjson_doc_free)

#define err_ret_on(cond, fmt, ...)                                                                 \
    do {                                                                                           \
        if (unlikely(cond)) {                                                                      \
            err(fmt, ##__VA_ARGS__);                                                               \
            return;                                                                                \
        }                                                                                          \
    } while (0)

static s8 yyjson_get_s8(yyjson_val *val) {
    return (s8){.s = (u8 *)yyjson_get_str(val), .len = yyjson_get_len(val)};
}

static void parse_meta(yyjson_val *metaobj, index_meta *im) {
    err_ret_on(!yyjson_is_obj(metaobj), "Meta entry in index.json does not consist of an object.");

    size_t idx, max;
    yyjson_val *objkey, *objval;
    yyjson_obj_foreach(metaobj, idx, max, objkey, objval) {
        s8 s8key = yyjson_get_s8(objkey);
        if (s8equals(s8key, S("name"))) {
            im->name = yyjson_get_s8(objval);
        } else if (s8equals(s8key, S("media_dir"))) {
            im->media_dir = yyjson_get_s8(objval);
        }
    }
}

static void parse_headwords(yyjson_val *headwordobj, index_meta im,
                            void (*foreach_headword)(void *, s8, s8), void *userdata_hw) {
    err_ret_on(!yyjson_is_obj(headwordobj),
               "Headword entry in index.json does not consist of an object.");

    size_t objidx, objmax;
    yyjson_val *objkey, *objval;
    yyjson_obj_foreach(headwordobj, objidx, objmax, objkey, objval) {
        s8 headword = yyjson_get_s8(objkey);

        err_ret_on(!yyjson_is_arr(objval), "Headword '%.*s' doesn't have an array as value.",
                   (int)headword.len, (char *)headword.s);
        size_t arridx, arrmax;
        yyjson_val *arrval;
        yyjson_arr_foreach(objval, arridx, arrmax, arrval) {
            s8 fn = yyjson_get_s8(arrval);
            _drop_(frees8) s8 fullpath = buildpath(im.root_path, im.media_dir, fn);
            foreach_headword(userdata_hw, headword, fullpath);
        }
    }
}

static FileInfo extract_fileinfo(yyjson_val *fileinfoobj) {

    FileInfo fi = {0};
    size_t objidx, objmax;
    yyjson_val *objkey, *objval;
    yyjson_obj_foreach(fileinfoobj, objidx, objmax, objkey, objval) {
        s8 key = yyjson_get_s8(objkey);
        s8 val = yyjson_get_s8(objval);

        if (s8equals(key, S("kana_reading"))) {
            kata2hira(val); // Warning: changes json. I think yyjson expects immutable
            fi.hira_reading = val;
        } else if (s8equals(key, S("pitch_number")))
            fi.pitch_number = val;
        else if (s8equals(key, S("pitch_pattern")))
            fi.pitch_pattern = val;
    }

    return fi;
}

static void parse_files(yyjson_val *filesobj, index_meta im,
                        void (*foreach_file)(void *, s8, FileInfo), void *userdata_f) {
    err_ret_on(!yyjson_is_obj(filesobj), "Value of \"files\" is not an object.");

    size_t objidx, objmax;
    yyjson_val *objkey, *objval;
    yyjson_obj_foreach(filesobj, objidx, objmax, objkey, objval) {
        err_ret_on(!yyjson_is_obj(objval), "Value of key: '%s' is not an object.",
                   yyjson_get_str(objkey));
        s8 fn = yyjson_get_s8(objkey);

        FileInfo fi = extract_fileinfo(objval);
        fi.origin = im.name;
        _drop_(frees8) s8 fullpath = buildpath(im.root_path, im.media_dir, fn);
        foreach_file(userdata_f, fullpath, fi);
    }
}

void parse_audio_index_from_file(s8 curdir, const char *index_filepath,
                                 void (*foreach_headword)(void *, s8, s8), void *userdata_hw,
                                 void (*foreach_file)(void *, s8, FileInfo), void *userdata_f) {
    _drop_(fclose) FILE *index_fp = fopen(index_filepath, "r");
    err_ret_on(!index_fp, "Failed to open the index '%s': %s", index_filepath, strerror(errno));

    yyjson_read_err error;
    _drop_(yyjson_doc_free) yyjson_doc *doc = yyjson_read_fp(index_fp, 0, NULL, &error);
    err_ret_on(!doc, "index.json read error: %s code: %u at position: %zu\n", error.msg, error.code,
               error.pos);

    yyjson_val *rootobj = yyjson_doc_get_root(doc);
    err_ret_on(!yyjson_is_obj(rootobj),
               "Invalid audio index format: Dictionary not consisting of an object.");

    index_meta im = {.root_path = curdir};
    bool seen_meta = false;

    size_t idx, max;
    yyjson_val *objkey, *objval;
    yyjson_obj_foreach(rootobj, idx, max, objkey, objval) {
        s8 s8key = yyjson_get_s8(objkey);

        if (s8equals(s8key, S("meta"))) {
            parse_meta(objval, &im);
            seen_meta = true;
        } else if (s8equals(s8key, S("headwords"))) {
            err_ret_on(
                !seen_meta,
                "Index in '%s' orders \"headwords\" before \"meta\". This is not supported yet.",
                index_filepath);
            parse_headwords(objval, im, foreach_headword, userdata_hw);
        } else if (s8equals(s8key, S("files"))) {
            err_ret_on(!seen_meta,
                       "Index in '%s' orders \"files\" before \"meta\". This is not supported yet.",
                       index_filepath);
            parse_files(objval, im, foreach_file, userdata_f);
        }
    }
}