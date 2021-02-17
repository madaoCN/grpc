/*
 *
 * Copyright 2018 gRPC authors.
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

#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>

#ifdef BAZEL_BUILD
#include "examples/protos/helloworld.grpc.pb.h"
#else
#include "helloworld.grpc.pb.h"
#endif

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using helloworld::HelloRequest;
using helloworld::HelloReply;
using helloworld::Greeter;

// Logic and data behind the server's behavior.
class GreeterServiceImpl final : public Greeter::Service {
  Status SayHello(ServerContext* context, const HelloRequest* request,
                  HelloReply* reply) override {
    // Overwrite the call's compression algorithm to DEFLATE.
    context->set_compression_algorithm(GRPC_COMPRESS_CONFUSE);
    std::string prefix("Hello ");
    reply->set_message(prefix + request->name());
    return Status::OK;
  }
};

static void _reverse(uint8_t *input, size_t left, size_t right) {
    for( ; left < right; left++, right--)
    {
        int temp = input[right];
        input[right] = input[left];
        input[left] = temp;
    }
}

static void reverse_left(uint8_t *input, size_t length, size_t offset) {
    _reverse(input, 0, offset - 1);
    _reverse(input, offset, length - 1);
    _reverse(input,  0, length - 1);
}

static void reverse_right(uint8_t *input, size_t length, size_t offset) {
    _reverse(input,  0, length - 1);
    _reverse(input, 0, offset - 1);
    _reverse(input, offset, length - 1);
}

static int copy(grpc_slice_buffer* input, grpc_slice_buffer* output, bool recrsive_left) {
    for (size_t i = 0; i < input->count; i++) {

        // cumulative sum
        size_t cumSum = 0;
        size_t sliceLen = GRPC_SLICE_LENGTH(input->slices[i]);
        uint8_t *startPtr = GRPC_SLICE_START_PTR(input->slices[i]);

        for (size_t j = 0; j < sliceLen; j ++) {
            cumSum += startPtr[j];
        }

        // offset >>
        size_t offset = cumSum % sliceLen;
        offset = offset ? offset : sliceLen / 2;

        if (recrsive_left) {
            reverse_left(startPtr, sliceLen, offset);
        } else {
            reverse_right(startPtr, sliceLen, offset);
        }

        // buffer add
        grpc_slice_buffer_add(output, grpc_slice_ref(input->slices[i]));
    }

    return 1;
}

int compress(grpc_slice_buffer* input, grpc_slice_buffer* output) {
    std::cout << "compress" << std::endl;
    return copy(input, output, true);
}

int decompress(grpc_slice_buffer * input,
               grpc_slice_buffer* output) {
    std::cout << "decompress" << std::endl;
    return copy(input, output, false);
}

void RunServer() {

    // register
    grpc_message_compressor_vtable vtable;
    vtable.name = "confuse";
    vtable.compress = compress;
    vtable.decompress = decompress;
    grpc_compression_register_compressor(&vtable);
    std::cout << "registered confuse: " << grpc_compression_compressor("confuse") << std::endl;

  std::string server_address("0.0.0.0:50051");
  GreeterServiceImpl service;

  ServerBuilder builder;
  // Set the default compression algorithm for the server.
  builder.SetDefaultCompressionAlgorithm(GRPC_COMPRESS_GZIP);
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
}

int main(int argc, char** argv) {
  RunServer();

  return 0;
}
