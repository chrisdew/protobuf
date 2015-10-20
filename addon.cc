#include <node.h>
#include <nan.h>
#include "protobuf_for_node.h"

void init(v8::Handle<v8::Object> target) {
  target->Set(Nan::New<v8::String>("Schema").ToLocalChecked(), protobuf_for_node::SchemaConstructor());
}

NODE_MODULE(protobuf_for_node, init)

