/*
 *
 * Copyright 2016 gRPC authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifndef GRPC_IMPL_CODEGEN_COMPRESSION_TYPES_H
#define GRPC_IMPL_CODEGEN_COMPRESSION_TYPES_H

#include <grpc/impl/codegen/port_platform.h>
#include <grpc/slice_buffer.h>

#ifdef __cplusplus
extern "C" {
#endif

/** To be used as initial metadata key for the request of a concrete compression
 * algorithm */
#define GRPC_COMPRESSION_REQUEST_ALGORITHM_MD_KEY \
  "grpc-internal-encoding-request"

/** To be used in channel arguments.
 *
 * \addtogroup grpc_arg_keys
 * \{ */
/** Default compression algorithm for the channel.
 * Its value is an int from the \a grpc_compression_algorithm enum. */
#define GRPC_COMPRESSION_CHANNEL_DEFAULT_ALGORITHM \
  "grpc.default_compression_algorithm"
/** Default compression level for the channel.
 * Its value is an int from the \a grpc_compression_level enum. */
#define GRPC_COMPRESSION_CHANNEL_DEFAULT_LEVEL "grpc.default_compression_level"
/** Compression algorithms supported by the channel.
 * Its value is a bitset (an int). Bits correspond to algorithms in \a
 * grpc_compression_algorithm. For example, its LSB corresponds to
 * GRPC_COMPRESS_NONE, the next bit to GRPC_COMPRESS_DEFLATE, etc.
 * Unset bits disable support for the algorithm. By default all algorithms are
 * supported. It's not possible to disable GRPC_COMPRESS_NONE (the attempt will
 * be ignored). */
#define GRPC_COMPRESSION_CHANNEL_ENABLED_ALGORITHMS_BITSET \
  "grpc.compression_enabled_algorithms_bitset"
/** \} */

/** The various compression algorithms supported by gRPC (not sorted by
 * compression level) */
typedef enum {
  GRPC_COMPRESS_NONE = 0,
  GRPC_COMPRESS_DEFLATE,
  GRPC_COMPRESS_GZIP,
  /* EXPERIMENTAL: confuse compression is currently experimental. */
  GRPC_COMPRESS_CONFUSE,
  /* EXPERIMENTAL: Stream compression is currently experimental. */
  GRPC_COMPRESS_STREAM_GZIP,
  GRPC_COMPRESS_STREAM_CONFUSE,
  /* TODO(ctiller): snappy */
  GRPC_COMPRESS_ALGORITHMS_COUNT
} grpc_compression_algorithm;

/** Compression levels allow a party with knowledge of its peer's accepted
 * encodings to request compression in an abstract way. The level-algorithm
 * mapping is performed internally and depends on the peer's supported
 * compression algorithms. */
typedef enum {
  GRPC_COMPRESS_LEVEL_NONE = 0,
  GRPC_COMPRESS_LEVEL_LOW,
  GRPC_COMPRESS_LEVEL_MED,
  GRPC_COMPRESS_LEVEL_HIGH,
  GRPC_COMPRESS_LEVEL_COUNT
} grpc_compression_level;

typedef struct grpc_compression_options {
  /** All algs are enabled by default. This option corresponds to the channel
   * argument key behind \a GRPC_COMPRESSION_CHANNEL_ENABLED_ALGORITHMS_BITSET
   */
  uint32_t enabled_algorithms_bitset;

  /** The default compression level. It'll be used in the absence of call
   * specific settings. This option corresponds to the channel
   * argument key behind \a GRPC_COMPRESSION_CHANNEL_DEFAULT_LEVEL. If present,
   * takes precedence over \a default_algorithm.
   * TODO(dgq): currently only available for server channels. */
  struct grpc_compression_options_default_level {
    int is_set;
    grpc_compression_level level;
  } default_level;

  /** The default message compression algorithm. It'll be used in the absence of
   * call specific settings. This option corresponds to the channel argument key
   * behind \a GRPC_COMPRESSION_CHANNEL_DEFAULT_ALGORITHM. */
  struct grpc_compression_options_default_algorithm {
    int is_set;
    grpc_compression_algorithm algorithm;
  } default_algorithm;
} grpc_compression_options;

typedef struct grpc_message_compressor_vtable {

  /* compress 'input' to 'output'.
   On success, appends compressed slices to output and returns 1.
   On failure, appends uncompressed slices to output and returns 0. */
  int (*compress)(grpc_slice_buffer* input, grpc_slice_buffer* output);

  /* decompress 'input' to 'output'.
   On success, appends slices to output and returns 1.
   On failure, output is unchanged, and returns 0. */
  int (*decompress)(grpc_slice_buffer * input,
                            grpc_slice_buffer* output);

  /*
   * compressor name
   */
  const grpc_slice *name;
} grpc_message_compressor_vtable;


typedef struct grpc_stream_compressor_vtable grpc_stream_compressor_vtable;

/* Stream compression/decompression context */
typedef struct grpc_stream_compressor_context {
    const grpc_stream_compressor_vtable* vtable;
} grpc_stream_compressor_context;

typedef enum grpc_stream_compressor_method {
    GRPC_STREAM_COMPRESSOR_IDENTITY_COMPRESS = 0,
    GRPC_STREAM_COMPRESSOR_IDENTITY_DECOMPRESS,
    GRPC_STREAM_COMPRESSOR_GZIP_COMPRESS,
    GRPC_STREAM_COMPRESSOR_GZIP_DECOMPRESS,
    GRPC_STREAM_COMPRESSOR_CONFUSE_COMPRESS,
    GRPC_STREAM_COMPRESSOR_CONFUSE_DECOMPRESS,
    GRPC_STREAM_COMPRESSOR_METHOD_COUNT
} grpc_stream_compressor_method;

typedef enum grpc_stream_compressor_flush {
    GRPC_STREAM_COMPRESSOR_FLUSH_NONE = 0,
    GRPC_STREAM_COMPRESSOR_FLUSH_SYNC,
    GRPC_STREAM_COMPRESSOR_FLUSH_FINISH,
    GRPC_STREAM_COMPRESSOR_FLUSH_COUNT
} grpc_stream_compressor_flush;

struct grpc_stream_compressor_vtable {
    int (*compress)(grpc_stream_compressor_context* ctx, grpc_slice_buffer* in,
                     grpc_slice_buffer* out, size_t* output_size,
                     size_t max_output_size, grpc_stream_compressor_flush flush);
    int (*decompress)(grpc_stream_compressor_context* ctx,
                       grpc_slice_buffer* in, grpc_slice_buffer* out,
                       size_t* output_size, size_t max_output_size,
                       int* end_of_context);
    grpc_stream_compressor_context* (*context_create)(
            grpc_stream_compressor_method method);
    void (*context_destroy)(grpc_stream_compressor_context* ctx);

    /*
     * compressor name
     */
    const grpc_slice *name;
};

#ifdef __cplusplus
}
#endif

#endif /* GRPC_IMPL_CODEGEN_COMPRESSION_TYPES_H */
