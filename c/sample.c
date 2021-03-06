//
// mtpng - a multithreaded parallel PNG encoder in Rust
// sample.c - C API example
//
// Copyright (c) 2018 Brion Vibber
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include <stdio.h>

#include "mtpng.h"

typedef struct read_state_t {
    size_t width;
    size_t bpp;
    size_t stride;
    size_t y;
} read_state;

static size_t read_func(void* user_data, uint8_t* bytes, size_t len)
{
    read_state* state = (read_state*)user_data;
    for (size_t x = 0; x < state->width; x++) {
        size_t i = x * state->bpp;
        bytes[i] = (x + state->y) % 256;
        bytes[i + 1] = (2 * x + state->y) % 256;
        bytes[i + 2] = (x + 2 * state->y) % 256;
    }
    state->y++;
    return len;
}

static size_t write_func(void* user_data, const uint8_t* bytes, size_t len)
{
    FILE* out = (FILE*)user_data;
    return fwrite(bytes, 1, len, out);
}

static bool flush_func(void* user_data)
{
    FILE* out = (FILE*)user_data;
    if (fflush(out) == 0) {
        return true;
    } else {
        return false;
    }
}

//
// Note that ol' "do/while(0)" trick. Yay C macros!
// Helps ensure consistency in if statements and stuff.
//
#define TRY(ret) \
do { \
    mtpng_result _ret = (ret); \
    if (_ret != MTPNG_RESULT_OK) { \
        fprintf(stderr, "Error: %d\n", (int)(_ret)); \
        goto cleanup; \
    }\
} while (0)

int main(int argc, char **argv) {
    size_t const threads = MTPNG_THREADS_DEFAULT;

    uint32_t const width = 1024;
    uint32_t const height = 768;

    size_t const channels = 3;
    size_t const bpp = channels;
    size_t const stride = width * bpp;

    read_state state;
    state.stride = stride;
    state.width = width;
    state.bpp = bpp;
    state.y = 0;

    FILE* out = fopen("out/csample.png", "wb");
    if (!out) {
        fprintf(stderr, "Error: failed to open output file\n");
        return 1;
    }

    //
    // Create a custom thread pool and the encoder.
    //
    mtpng_threadpool* pool = NULL;
    TRY(mtpng_threadpool_new(&pool, threads));

    mtpng_encoder* encoder = NULL;
    TRY(mtpng_encoder_new(&encoder,
                          write_func,
                          flush_func,
                          (void*)out,
                          pool));

    //
    // Set some encoding options
    //
    TRY(mtpng_encoder_set_chunk_size(encoder, 200000));

    //
    // Set up the PNG image state
    //
    TRY(mtpng_encoder_set_size(encoder, 1024, 768));
    TRY(mtpng_encoder_set_color(encoder, MTPNG_COLOR_TRUECOLOR, 8));
    TRY(mtpng_encoder_set_filter(encoder, MTPNG_FILTER_ADAPTIVE));

    //
    // Write the data!
    //
    TRY(mtpng_encoder_write_header(encoder));
    TRY(mtpng_encoder_write_image(encoder, read_func, (void*)&state));
    TRY(mtpng_encoder_finish(&encoder));
    TRY(mtpng_threadpool_release(&pool));

    printf("Done.\n");
    return 0;

    // Error handler for the TRY macros:
cleanup:
    if (encoder) {
        TRY(mtpng_encoder_release(&encoder));
    }
    if (pool) {
        TRY(mtpng_threadpool_release(&pool));
    }

    printf("Failed!\n");
    return 1;
}
