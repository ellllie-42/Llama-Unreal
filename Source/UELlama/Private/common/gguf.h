#include "ggml.h"

#include <cstdio>
#include <cinttypes>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>

#undef MIN
#undef MAX
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

namespace {

    template <typename T>
    static std::string to_string(const T& val) {
        std::stringstream ss;
        ss << val;
        return ss.str();
    }


}

static bool gguf_ex_write(const std::string& fname) {
    struct gguf_context* ctx = gguf_init_empty();

    gguf_set_val_u8(ctx, "some.parameter.uint8", 0x12);
    gguf_set_val_i8(ctx, "some.parameter.int8", -0x13);
    gguf_set_val_u16(ctx, "some.parameter.uint16", 0x1234);
    gguf_set_val_i16(ctx, "some.parameter.int16", -0x1235);
    gguf_set_val_u32(ctx, "some.parameter.uint32", 0x12345678);
    gguf_set_val_i32(ctx, "some.parameter.int32", -0x12345679);
    gguf_set_val_f32(ctx, "some.parameter.float32", 0.123456789f);
    gguf_set_val_u64(ctx, "some.parameter.uint64", 0x123456789abcdef0ull);
    gguf_set_val_i64(ctx, "some.parameter.int64", -0x123456789abcdef1ll);
    gguf_set_val_f64(ctx, "some.parameter.float64", 0.1234567890123456789);
    gguf_set_val_bool(ctx, "some.parameter.bool", true);
    gguf_set_val_str(ctx, "some.parameter.string", "hello world");

    gguf_set_arr_data(ctx, "some.parameter.arr.i16", GGUF_TYPE_INT16, std::vector<int16_t>{ 1, 2, 3, 4, }.data(), 4);
    gguf_set_arr_data(ctx, "some.parameter.arr.f32", GGUF_TYPE_FLOAT32, std::vector<float>{ 3.145f, 2.718f, 1.414f, }.data(), 3);
    gguf_set_arr_str(ctx, "some.parameter.arr.str", std::vector<const char*>{ "hello", "world", "!" }.data(), 3);

    struct ggml_init_params params = {
        /*.mem_size   =*/ 128ull * 1024ull * 1024ull,
        /*.mem_buffer =*/ NULL,
        /*.no_alloc   =*/ false,
    };

    struct ggml_context* ctx_data = ggml_init(params);

    const int n_tensors = 10;

    // tensor infos
    for (int i = 0; i < n_tensors; ++i) {
        const std::string name = "tensor_" + to_string(i);

        int64_t ne[GGML_MAX_DIMS] = { 1 };
        int32_t n_dims = rand() % GGML_MAX_DIMS + 1;

        for (int j = 0; j < n_dims; ++j) {
            ne[j] = rand() % 10 + 1;
        }

        struct ggml_tensor* cur = ggml_new_tensor(ctx_data, GGML_TYPE_F32, n_dims, ne);
        ggml_set_name(cur, name.c_str());

        {
            float* data = (float*)cur->data;
            for (int j = 0; j < ggml_nelements(cur); ++j) {
                data[j] = 100 + i;
            }
        }

        gguf_add_tensor(ctx, cur);
    }

    gguf_write_to_file(ctx, fname.c_str(), false);

    UE_LOG(LogTemp, Log, TEXT("%s: wrote file '%s;\n"), ANSI_TO_TCHAR(__func__), fname.c_str());

    ggml_free(ctx_data);
    gguf_free(ctx);

    return true;
}

// just read tensor info
static bool gguf_ex_read_0(const std::string& fname) {
    struct gguf_init_params params = {
        /*.no_alloc = */ false,
        /*.ctx      = */ NULL,
    };

    struct gguf_context* ctx = gguf_init_from_file(fname.c_str(), params);

    UE_LOG(LogTemp, Log, TEXT("%s: version:      %d\n"), ANSI_TO_TCHAR(__func__), gguf_get_version(ctx));
    UE_LOG(LogTemp, Log, TEXT("%s: alignment:   %zu\n"), ANSI_TO_TCHAR(__func__), gguf_get_alignment(ctx));
    UE_LOG(LogTemp, Log, TEXT("%s: data offset: %zu\n"), ANSI_TO_TCHAR(__func__), gguf_get_data_offset(ctx));

    // kv
    {
        const int n_kv = gguf_get_n_kv(ctx);

        UE_LOG(LogTemp, Log, TEXT("%s: n_kv: %d\n"), ANSI_TO_TCHAR(__func__), n_kv);

        for (int i = 0; i < n_kv; ++i) {
            const char* key = gguf_get_key(ctx, i);

            UE_LOG(LogTemp, Log, TEXT("%s: kv[%d]: key = %s\n"), ANSI_TO_TCHAR(__func__), i, ANSI_TO_TCHAR(key));
        }
    }

    // find kv string
    {
        const char* findkey = "some.parameter.string";

        const int keyidx = gguf_find_key(ctx, findkey);
        if (keyidx == -1) {
            UE_LOG(LogTemp, Log, TEXT("%s: find key: %s not found.\n"), ANSI_TO_TCHAR(__func__), ANSI_TO_TCHAR(findkey));
        }
        else {
            const char* key_value = gguf_get_val_str(ctx, keyidx);
            UE_LOG(LogTemp, Log, TEXT("%s: find key: %s found, kv[%d] value = %s\n"), ANSI_TO_TCHAR(__func__), ANSI_TO_TCHAR(findkey), keyidx, ANSI_TO_TCHAR(key_value));
        }
    }

    // tensor info
    {
        const int n_tensors = gguf_get_n_tensors(ctx);

        UE_LOG(LogTemp, Log, TEXT("%s: n_tensors: %d\n"), ANSI_TO_TCHAR(__func__), n_tensors);

        for (int i = 0; i < n_tensors; ++i) {
            const char* name = gguf_get_tensor_name(ctx, i);
            const size_t offset = gguf_get_tensor_offset(ctx, i);

            UE_LOG(LogTemp, Log, TEXT("%s: tensor[%d]: name = %s, offset = %zu\n"), ANSI_TO_TCHAR(__func__), i, ANSI_TO_TCHAR(name), offset);
        }
    }

    gguf_free(ctx);

    return true;
}

// read and create ggml_context containing the tensors and their data
static bool gguf_ex_read_1(const std::string& fname) {
    struct ggml_context* ctx_data = NULL;

    struct gguf_init_params params = {
        /*.no_alloc = */ false,
        /*.ctx      = */ &ctx_data,
    };

    struct gguf_context* ctx = gguf_init_from_file(fname.c_str(), params);

    UE_LOG(LogTemp, Log, TEXT("%s: version:      %d\n"), ANSI_TO_TCHAR(__func__), gguf_get_version(ctx));
    UE_LOG(LogTemp, Log, TEXT("%s: alignment:   %zu\n"), ANSI_TO_TCHAR(__func__), gguf_get_alignment(ctx));
    UE_LOG(LogTemp, Log, TEXT("%s: data offset: %zu\n"), ANSI_TO_TCHAR(__func__), gguf_get_data_offset(ctx));

    // kv
    {
        const int n_kv = gguf_get_n_kv(ctx);

        UE_LOG(LogTemp, Log, TEXT("%s: n_kv: %d\n"), ANSI_TO_TCHAR(__func__), n_kv);

        for (int i = 0; i < n_kv; ++i) {
            const char* key = gguf_get_key(ctx, i);

            UE_LOG(LogTemp, Log, TEXT("%s: kv[%d]: key = %s\n"), ANSI_TO_TCHAR(__func__), i, key);
        }
    }

    // tensor info
    {
        const int n_tensors = gguf_get_n_tensors(ctx);

        UE_LOG(LogTemp, Log, TEXT("%s: n_tensors: %d\n"), ANSI_TO_TCHAR(__func__), n_tensors);

        for (int i = 0; i < n_tensors; ++i) {
            const char* name = gguf_get_tensor_name(ctx, i);
            const size_t offset = gguf_get_tensor_offset(ctx, i);

            UE_LOG(LogTemp, Log, TEXT("%s: tensor[%d]: name = %s, offset = %zu\n"), ANSI_TO_TCHAR(__func__), i, ANSI_TO_TCHAR(name), offset);
        }
    }

    // data
    {
        const int n_tensors = gguf_get_n_tensors(ctx);

        for (int i = 0; i < n_tensors; ++i) {
            UE_LOG(LogTemp, Log, TEXT("%s: reading tensor %d data\n"), ANSI_TO_TCHAR(__func__), i);

            const char* name = gguf_get_tensor_name(ctx, i);

            struct ggml_tensor* cur = ggml_get_tensor(ctx_data, name);

            UE_LOG(LogTemp, Log, TEXT("%s: tensor[%d]: n_dims = %d, name = %s, data = %p\n"), ANSI_TO_TCHAR(__func__), i, ggml_n_dims(cur), ANSI_TO_TCHAR(cur->name), cur->data);

            // print first 10 elements
            const float* data = (const float*)cur->data;

            UE_LOG(LogTemp, Log, TEXT("%s data[:10] : "), name);
            for (int j = 0; j < MIN(10, ggml_nelements(cur)); ++j) {
                UE_LOG(LogTemp, Log, TEXT("%f "), data[j]);
            }
            UE_LOG(LogTemp, Log, TEXT("\n\n"));

            // check data
            {
                data = (const float*)cur->data;
                for (int j = 0; j < ggml_nelements(cur); ++j) {
                    if (data[j] != 100 + i) {
                        UE_LOG(LogTemp, Log, TEXT("%s: tensor[%d]: data[%d] = %f\n"), ANSI_TO_TCHAR(__func__), i, j, data[j]);
                        return false;
                    }
                }
            }
        }
    }

    UE_LOG(LogTemp, Log, TEXT("%s: ctx_data size: %zu\n"), ANSI_TO_TCHAR(__func__), ggml_get_mem_size(ctx_data));

    ggml_free(ctx_data);
    gguf_free(ctx);

    return true;
}