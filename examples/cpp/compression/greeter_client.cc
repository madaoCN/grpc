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
#include <grpcpp/impl/codegen/core_codegen.h>

#ifdef BAZEL_BUILD
#include "examples/protos/helloworld.grpc.pb.h"
#else
#include "helloworld.grpc.pb.h"
#endif

using grpc::Channel;
using grpc::ChannelArguments;
using grpc::ClientContext;
using grpc::Status;
using helloworld::HelloRequest;
using helloworld::HelloReply;
using helloworld::Greeter;

class GreeterClient {
 public:
  GreeterClient(std::shared_ptr<Channel> channel)
      : stub_(Greeter::NewStub(channel)) {}

  // Assembles the client's payload, sends it and presents the response back
  // from the server.
  std::string SayHello(const std::string& user) {
    // Data we are sending to the server.
    HelloRequest request;
    request.set_name(user);

    // Container for the data we expect from the server.
    HelloReply reply;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;

    // Overwrite the call's compression algorithm to DEFLATE.
    context.set_compression_algorithm(GRPC_COMPRESS_CONFUSE);

    // The actual RPC.
    Status status = stub_->SayHello(&context, request, &reply);

    // Act upon its status.
    if (status.ok()) {
      return reply.message();
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return "RPC failed";
    }
  }

 private:
  std::unique_ptr<Greeter::Stub> stub_;
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

int main(int argc, char** argv) {
  // Instantiate the client. It requires a channel, out of which the actual RPCs
  // are created. This channel models a connection to an endpoint (in this case,
  // localhost at port 50051). We indicate that the channel isn't authenticated
  // (use of InsecureChannelCredentials()).

  // register
  grpc_message_compressor_vtable vtable;
  vtable.name = "confuse";
  vtable.compress = compress;
  vtable.decompress = decompress;
  grpc_compression_register_compressor(&vtable);
  std::cout << "registered confuse: " << grpc_compression_compressor("confuse") << std::endl;

  ChannelArguments args;
  // Set the default compression algorithm for the channel.
  args.SetCompressionAlgorithm(GRPC_COMPRESS_GZIP);
  GreeterClient greeter(grpc::CreateCustomChannel(
      "localhost:50051", grpc::InsecureChannelCredentials(), args));
    std::string user("world world world world");
  std::string reply = greeter.SayHello(user);
  std::cout << "Greeter received: " << reply << std::endl;

  return 0;
}
