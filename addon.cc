#include "protobuf_for_node.h"

extern "C" void init(v8::Handle<v8::Object> target) {
  target->Set(v8::String::New("Schema"), protobuf_for_node::SchemaConstructor());
}
