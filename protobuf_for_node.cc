// Copyright 2010 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License"); you
// may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
// implied. See the License for the specific language governing
// permissions and limitations under the License.

#include <map>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>

#include <node.h>
#include <nan.h>

#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/service.h>

#include "protobuf_for_node.h"

using google::protobuf::Descriptor;
using google::protobuf::DescriptorPool;
using google::protobuf::DynamicMessageFactory;
using google::protobuf::FieldDescriptor;
using google::protobuf::FileDescriptorSet;
using google::protobuf::Message;
using google::protobuf::MethodDescriptor;
using google::protobuf::Reflection;
using google::protobuf::Service;
using google::protobuf::ServiceDescriptor;

using Nan::ObjectWrap;

using std::map;
using std::string;
using std::vector;
using std::cerr;
using std::endl;

using v8::Array;
using v8::Context;
using v8::External;
using v8::Function;
using v8::FunctionTemplate;
using v8::Integer;
using v8::Handle;
using v8::Local;
using v8::Number;
using v8::Object;
using v8::ObjectTemplate;
using v8::Persistent;
using v8::Script;
using v8::String;
using v8::Value;
using v8::V8;

namespace protobuf_for_node {
  const char E_NO_ARRAY[] = "Not an array";
  const char E_NO_OBJECT[] = "Not an object";
  const char E_UNKNOWN_ENUM[] = "Unknown enum value";

  Nan::Persistent<FunctionTemplate> SchemaTemplate;
  Nan::Persistent<FunctionTemplate> ServiceSchemaTemplate;
  Nan::Persistent<FunctionTemplate> ServiceTemplate;
  Nan::Persistent<FunctionTemplate> MethodTemplate;
  Nan::Persistent<FunctionTemplate> TypeTemplate;
  Nan::Persistent<FunctionTemplate> ParseTemplate;
  Nan::Persistent<FunctionTemplate> SerializeTemplate;

  class Schema : public Nan::ObjectWrap {
  public:
    Schema(Local<Object> self, const DescriptorPool* pool)
        : pool_(pool) {
      factory_.SetDelegateToGeneratedFactory(true);
      self->SetInternalField(1, Nan::New<Array>());
      Wrap(self);
    }

    virtual ~Schema() {
      if (pool_ != DescriptorPool::generated_pool())
        delete pool_;
    }

    class Type : public Nan::ObjectWrap {
    public:
      Schema* schema_;
      const Descriptor* descriptor_;

      Message* NewMessage() const {
        return schema_->NewMessage(descriptor_);
      }

      Local<Function> Constructor() const {
        Local<Object> handle = const_cast<Type *>(this)->handle();
        return handle->GetInternalField(2).As<Function>();
      }

      Local<Object> NewObject(Local<Value> properties) const {
        return Constructor()->NewInstance(1, &properties);
      }

      Type(Schema* schema, const Descriptor* descriptor, Local<Object> self)
        : schema_(schema), descriptor_(descriptor) {
        // Generate functions for bulk conversion between a JS object
        // and an array in descriptor order:
        //   from = function(arr) { this.f0 = arr[0]; this.f1 = arr[1]; ... }
        //   to   = function()    { return [ this.f0, this.f1, ... ] }
        // This is faster than repeatedly calling Get/Set on a v8::Object.
        std::ostringstream from, to;
        from << "(function(arr) { if(arr) {";
        to << "(function() { return [ ";

        for (int i = 0; i < descriptor->field_count(); i++) {
          from <<
            "var x = arr[" << i << "]; "
            "if(x !== undefined) this['" <<
            descriptor->field(i)->camelcase_name() <<
            "'] = x; ";

          if (i > 0) to << ", ";
          to << "this['" << descriptor->field(i)->camelcase_name() << "']";
        }

        from << " }})";
        to << " ]; })";

        // managed type->schema link
        self->SetInternalField(1, schema_->handle());

        Local<Function> constructor =
          Script::Compile(Nan::New<String>(from.str()).ToLocalChecked())->Run().As<Function>();
        constructor->SetHiddenValue(Nan::New<String>("type").ToLocalChecked(), self);

        Local<Function> bind =
          Script::Compile(Nan::New<String>(
              "(function(self) {"
              "  var f = this;"
              "  return function(arg) {"
              "    return f.call(self, arg);"
              "  };"
              "})").ToLocalChecked())->Run().As<Function>();
        Local<Value> arg = self;

        Local<FunctionTemplate> parseTemplate = Nan::New(ParseTemplate);
        Local<FunctionTemplate> serializeTemplate = Nan::New(SerializeTemplate);

        constructor->Set(Nan::New<String>("parse").ToLocalChecked(), bind->Call(parseTemplate->GetFunction(), 1, &arg));
        constructor->Set(Nan::New<String>("serialize").ToLocalChecked(), bind->Call(serializeTemplate->GetFunction(), 1, &arg));
        self->SetInternalField(2, constructor);
        self->SetInternalField(3, Script::Compile(Nan::New<String>(to.str()).ToLocalChecked())->Run());

        Wrap(self);
      }

#define GET(TYPE)                                                        \
      (index >= 0 ?                                                      \
       reflection->GetRepeated##TYPE(instance, field, index) :           \
       reflection->Get##TYPE(instance, field))

      static Local<Value> ToJs(const Message& instance,
                                const Reflection* reflection,
                                const FieldDescriptor* field,
                                const Type* message_type,
                                int index) {
        switch (field->cpp_type()) {
        case FieldDescriptor::CPPTYPE_MESSAGE:
          return message_type->ToJs(GET(Message));
        case FieldDescriptor::CPPTYPE_STRING: {
          const string& value = GET(String);
          if (field->type() == FieldDescriptor::TYPE_BYTES) {
            return Nan::CopyBuffer(const_cast<char *>(value.data()), value.length()).ToLocalChecked();
          } else {
            return Nan::New<String>(value.data(), value.length()).ToLocalChecked();
          }
        }
        case FieldDescriptor::CPPTYPE_INT32:
          return Nan::New<v8::Int32>(GET(Int32));
        case FieldDescriptor::CPPTYPE_UINT32:
          return Nan::New<v8::Uint32>(GET(UInt32));
        case FieldDescriptor::CPPTYPE_INT64: {
          std::ostringstream ss;
          ss << GET(Int64);
          string s = ss.str();
          return Nan::New<String>(s.data(), s.length()).ToLocalChecked();
        }
        case FieldDescriptor::CPPTYPE_UINT64: {
          std::ostringstream ss;
          ss << GET(UInt64);
          string s = ss.str();
          return Nan::New<String>(s.data(), s.length()).ToLocalChecked();
        }
        case FieldDescriptor::CPPTYPE_FLOAT:
          return Nan::New<Number>(GET(Float));
        case FieldDescriptor::CPPTYPE_DOUBLE:
          return Nan::New<Number>(GET(Double));
        case FieldDescriptor::CPPTYPE_BOOL:
          if (GET(Bool)) {
            return Nan::True();
          }
          return Nan::False();
        case FieldDescriptor::CPPTYPE_ENUM:
          return Nan::New<String>(GET(Enum)->name().c_str()).ToLocalChecked();
        }

        return Nan::Undefined();  // NOTREACHED
      }
#undef GET

      Local<Object> ToJs(const Message& instance) const {
        const Reflection* reflection = instance.GetReflection();
        const Descriptor* descriptor = instance.GetDescriptor();

        Local<Array> properties = Nan::New<Array>(descriptor->field_count());
        for (int i = 0; i < descriptor->field_count(); i++) {
          Nan::HandleScope scope;

          const FieldDescriptor* field = descriptor->field(i);
          bool repeated = field->is_repeated();
          if (repeated && !reflection->FieldSize(instance, field)) continue;
          if (!repeated && !reflection->HasField(instance, field)) continue;

          const Type* child_type =
            (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) ?
            schema_->GetType(field->message_type()) : NULL;

          Local<Value> value;
          if (field->is_repeated()) {
            int size = reflection->FieldSize(instance, field);
            Local<Array> array = Nan::New<Array>(size);
            for (int j = 0; j < size; j++) {
              array->Set(j, ToJs(instance, reflection, field, child_type, j));
            }
            value = array;
          } else {
            value = ToJs(instance, reflection, field, child_type, -1);
          }

          properties->Set(i, value);
        }

        return NewObject(properties);
      }

      static NAN_METHOD(Parse) {
        if ((info.Length() < 1) || (!node::Buffer::HasInstance(info[0]))) {
          return Nan::ThrowTypeError("Argument should be a buffer");
        }

        Local<Object> buffer_obj = info[0]->ToObject();

        Type *type = Unwrap<Type>(info.This());
        Message* message = type->NewMessage();
        bool success =
          message->ParseFromArray(node::Buffer::Data(buffer_obj), node::Buffer::Length(buffer_obj));

        if (!success) {
          return Nan::ThrowError("Malformed message");
        }

        Local<Object> result = type->ToJs(*message);
        delete message;
        info.GetReturnValue().Set(result);
      }

#define SET(TYPE, EXPR)                                                 \
      if (repeated) reflection->Add##TYPE(instance, field, EXPR);       \
      else reflection->Set##TYPE(instance, field, EXPR)

      static const char* ToProto(Message* instance,
                                 const FieldDescriptor* field,
                                 Local<Value> value,
                                 const Type* type,
                                 bool repeated) {
        Nan::HandleScope scope;

        const Reflection* reflection = instance->GetReflection();
        switch (field->cpp_type()) {
        case FieldDescriptor::CPPTYPE_MESSAGE:
          if (!value->IsObject()) {
            return E_NO_OBJECT;
          }
          type->ToProto(repeated ?
                        reflection->AddMessage(instance, field) :
                        reflection->MutableMessage(instance, field),
                        value.As<Object>());
          break;
        case FieldDescriptor::CPPTYPE_STRING: {
          // Shortcutting Utf8value(buffer.toString())
          if (node::Buffer::HasInstance(value)) {
            Local<Object> buf = value->ToObject();
            SET(String, string(node::Buffer::Data(buf), node::Buffer::Length(buf)));
          } else {
            String::Utf8Value utf8(value);
            SET(String, string(*utf8, utf8.length()));
          }
          break;
        }
        case FieldDescriptor::CPPTYPE_INT32:
          SET(Int32, value->Int32Value());
          break;
        case FieldDescriptor::CPPTYPE_UINT32:
          SET(UInt32, value->Uint32Value());
          break;
        case FieldDescriptor::CPPTYPE_INT64:
          if (value->IsString()) {
            google::protobuf::int64 ll;
            std::istringstream(*String::Utf8Value(value)) >> ll;
            SET(Int64, ll);
          } else {
            SET(Int64, value->NumberValue());
          }
          break;
        case FieldDescriptor::CPPTYPE_UINT64:
          if (value->IsString()) {
            google::protobuf::uint64 ull;
            std::istringstream(*String::Utf8Value(value)) >> ull;
            SET(UInt64, ull);
          } else {
            SET(UInt64, value->NumberValue());
          }
          break;
        case FieldDescriptor::CPPTYPE_FLOAT:
          SET(Float, value->NumberValue());
          break;
        case FieldDescriptor::CPPTYPE_DOUBLE:
          SET(Double, value->NumberValue());
          break;
        case FieldDescriptor::CPPTYPE_BOOL:
          SET(Bool, value->BooleanValue());
          break;
        case FieldDescriptor::CPPTYPE_ENUM:
          const google::protobuf::EnumValueDescriptor* enum_value =
            value->IsNumber() ?
            field->enum_type()->FindValueByNumber(value->Int32Value()) :
            field->enum_type()->FindValueByName(*String::Utf8Value(value));
          if (!enum_value) {
            return E_UNKNOWN_ENUM;
          }
          SET(Enum, enum_value);
          break;
        }

        return NULL;
      }
#undef SET

      const char* ToProto(Message* instance, Local<Object> src) const {
        Local<Object> handle = const_cast<Type *>(this)->handle();
        Local<Function> to_array = handle->GetInternalField(3).As<Function>();
        Local<Array> properties = to_array->Call(src, 0, NULL).As<Array>();

        const char* error = NULL;
        for (int i = 0; !error && i < descriptor_->field_count(); i++) {
          Local<Value> value = properties->Get(i);
          if (value->IsUndefined() ||
              value->IsNull()) continue;

          const FieldDescriptor* field = descriptor_->field(i);
          const Type* child_type =
            (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) ?
            schema_->GetType(field->message_type()) : NULL;
          if (field->is_repeated()) {
            if(!value->IsArray()) {
              error = E_NO_ARRAY;
              continue;
            }

            Local<Array> array = value.As<Array>();
            int length = array->Length();

            for (int j = 0; !error && j < length; j++) {
              error = ToProto(instance, field, array->Get(j), child_type, true);
            }
          } else {
            error = ToProto(instance, field, value, child_type, false);
          }
        }
        return error;
      }

      static NAN_METHOD(Serialize) {
        if ((info.Length() < 1) || (!info[0]->IsObject())) {
          return Nan::ThrowTypeError("Not an object");
        }

        Type *type = Unwrap<Type>(info.This());
        Message* message = type->NewMessage();
        const char* error = type->ToProto(message, info[0].As<Object>());
        Local<Object> result;
        if (!error) {
          result = Nan::NewBuffer(message->ByteSize()).ToLocalChecked();
          message->SerializeWithCachedSizesToArray(
              (google::protobuf::uint8*)node::Buffer::Data(result));
        }
        delete message;

        if (error) {
          return Nan::ThrowError(error);
        }

        info.GetReturnValue().Set(result);
      }

      static NAN_METHOD(ToString) {
        Type *type = Unwrap<Type>(info.This());
        info.GetReturnValue().Set(Nan::New<String>(type->descriptor_->full_name()).ToLocalChecked());
      }
    };

    Message* NewMessage(const Descriptor* descriptor) {
      return factory_.GetPrototype(descriptor)->New();
    }

    Type* GetType(const Descriptor* descriptor) {
      Type* result = types_[descriptor];
      if (result) return result;

      Local<FunctionTemplate> typeTemplate = Nan::New(TypeTemplate);
      result = types_[descriptor] =
        new Type(this, descriptor, typeTemplate->GetFunction()->NewInstance());

      // managed schema->[type] link
      //
      Local<Object> handle = const_cast<Schema *>(this)->handle();
      Local<Array> types = handle->GetInternalField(1).As<Array>();
      types->Set(types->Length(), result->handle());
      return result;
    }

    const DescriptorPool* pool_;
    map<const Descriptor*, Type*> types_;
    DynamicMessageFactory factory_;

    static NAN_PROPERTY_GETTER(GetType) {
      Schema *schema = Unwrap<Schema>(info.This());
      const Descriptor* descriptor =
        schema->pool_->FindMessageTypeByName(*String::Utf8Value(property));

      if (descriptor != NULL) {
        info.GetReturnValue().Set(schema->GetType(descriptor)->Constructor());
        return;
      }

      info.GetReturnValue().SetUndefined();
    }

    static NAN_METHOD(NewSchema) {
      Schema *schema;

      if (info.Length() < 1) {
        schema = new Schema(info.This(), DescriptorPool::generated_pool());
        info.GetReturnValue().Set(schema->handle());
        return;
      }

      if (!node::Buffer::HasInstance(info[0])) {
        return Nan::ThrowTypeError("Argument should be a buffer");
      }

      Local<Object> buffer_obj = info[0]->ToObject();
      char *buffer_data = node::Buffer::Data(buffer_obj);
      size_t buffer_length = node::Buffer::Length(buffer_obj);

      FileDescriptorSet descriptors;
      if (!descriptors.ParseFromArray(buffer_data, buffer_length)) {
        return Nan::ThrowError("Malformed descriptor");
      }

      DescriptorPool* pool = new DescriptorPool;
      for (int i = 0; i < descriptors.file_size(); i++) {
        pool->BuildFile(descriptors.file(i));
      }

      schema = new Schema(info.This(), pool);
      info.GetReturnValue().Set(schema->handle());
    }
  };

 // services

/*
  class WrappedService : public Nan::ObjectWrap {
  public:
    static Handle<Value> Invoke(const Arguments& args) {
      WrappedService* self = UnwrapThis<WrappedService>(args);

      MethodDescriptor* method = static_cast<MethodDescriptor*>(External::Unwrap(info[0]));
      Schema* schema = Nan::ObjectWrap::Unwrap<Schema>(self->handle_->GetInternalField(1).As<Object>());
      Schema::Type* input_type = schema->GetType(method->input_type());
      Schema::Type* output_type = schema->GetType(method->output_type());
      Message* request = self->service_->GetRequestPrototype(method).New();
      Message* response = self->service_->GetResponsePrototype(method).New();
      input_type->ToProto(request, info[1].As<Object>());

      if (info.Length() > 2) {
        AsyncInvocation::Start(self, method, request, response, output_type, info[2].As<Function>());
        return v8::Undefined();
      } else {
        bool done = false;
        self->service_->CallMethod(method,
                                   NULL,
                                   request,
                                   response,
                                   google::protobuf::NewCallback(&WrappedService::Done, &done));
        if (!done)
          fprintf(stderr, "ERROR: Synchronous proto service invocation %s didn't invoke 'done->Run()'\n", method->full_name().c_str());

        Handle<Object> result = output_type->ToJs(*response);

        delete response;
        delete request;

        return result;
      }
    }

    WrappedService(Service* service) : service_(service) {
      Wrap(ServiceTemplate->GetFunction()->NewInstance());

      const ServiceDescriptor* descriptor = service->GetDescriptor();
      Schema* schema = new Schema(ServiceSchemaTemplate->GetFunction()->NewInstance(),
                                  descriptor->file()->pool());
      handle_->SetInternalField(1, schema->handle_);

      Handle<Function> bind =
        Script::Compile(Nan::New<String>(
            "(function(m) {"
            "  var f = this;"
            "  return function(arg1, arg2) {"
            "    return arg2 ? f.call(this, m, arg1, arg2) : f.call(this, m, arg1);"
            "  };"
            "})"))->Run().As<Function>();
      for (int i = 0; i < descriptor->method_count(); i++) {
        const MethodDescriptor* method = descriptor->method(i);
        Handle<Value> arg = External::Wrap(const_cast<MethodDescriptor*>(method));
        handle_->Set(Nan::New<String>(method->name().c_str()),
                     bind->Call(MethodTemplate->GetFunction(), 1, &arg));
      }
    }

    virtual ~WrappedService() {
      delete service_;
    }

    static void Init() {
      AsyncInvocation::Init();
    }

  private:
    Service* service_;

    static void Done(bool* done) {
      *done = true;
    }

    class AsyncInvocation {
    public:
      static void Init() {
        uv_async_init(uv_default_loop(), &ev_done, &InvokeAsyncCallbacks);
      }

      // main thread:
      static void Start(WrappedService* service,
                        MethodDescriptor* method,
                        Message* request,
                        Message* response,
                        Schema::Type* response_type,
                        Handle<Function> cb) {

        uv_work_t *req = new uv_work_t;

        uv_queue_work(uv_default_loop(), req, &AsyncInvocation::Run, &AsyncInvocation::Returned);
        ev_ref(EV_DEFAULT_UC);
      }

    private:
      AsyncInvocation(WrappedService* service,
                      MethodDescriptor* method,
                      Message* request,
                      Message* response,
                      Schema::Type* response_type,
                      Handle<Function> cb) : service_(service),
                                               method_(method),
                                               request_(request),
                                               response_(response),
                                               response_type_(response_type),
                                               cb_(Persistent<Function>::New(cb)),
                                               done_(false) {
        service_->Ref();  // includes ->schema->type
      }

      ~AsyncInvocation() {
        service_->Unref();
        cb_.Dispose();
        delete request_;
        delete response_;
      }

      static AsyncInvocation* head;
      static uv_async_t ev_done;

      // in some thread:
      static void Run(uv_work_t *req) {
        AsyncInvocation* self = static_cast<AsyncInvocation*>(req->data);
        self->service_->service_->CallMethod(self->method_,
                                             NULL,
                                             self->request_,
                                             self->response_,
                                             google::protobuf::NewCallback(&Done, self));
      }

      // in some thread:
      static void Done(AsyncInvocation* self) {
        self->done_ = true;
        uv_async_send(&ev_done);
      }

      // main thread: service method returned
      static void Returned(uv_work_t* req) {
        AsyncInvocation* self = static_cast<AsyncInvocation*>(req->data);
        if (self->done_) {
          InvokeCallback(self);
        } else {
          // first delayed response?

          // enqueue
          self->next_ = head;
          head = self;
        }
      }

      // main thread: service called Done() delayed
      static void InvokeAsyncCallbacks(uv_async_t *a, int b) {
        AsyncInvocation** pself = &head;
        AsyncInvocation* self;
        while ((self = *pself)) {
          if (self->done_) {
            *pself = self->next_;
            InvokeCallback(self);
          } else {
            pself = &(self->next_);
          }
        }

        // all outstanding callbacks delivered
        if (!head) {
          uv_close((uv_handle_t*) &ev_done, NULL);
	}
      }

      static void InvokeCallback(AsyncInvocation* self) {
        Nan::HandleScope scope;
        Handle<Value> result = self->response_type_->ToJs(*(self->response_));
        self->cb_->Call(Context::GetCurrent()->Global(), 1, &result);
        delete self;
        ev_unref(EV_DEFAULT_UC);
      }

    private:
      WrappedService* service_;
      MethodDescriptor* method_;
      Message* request_;
      Message* response_;
      Schema::Type* response_type_;
      Persistent<Function> cb_;
      bool done_;
      AsyncInvocation* next_;
    };
  };
  WrappedService::AsyncInvocation* WrappedService::AsyncInvocation::head = NULL;
  uv_async_t WrappedService::AsyncInvocation::ev_done;
*/

  static void Init() {
    if (!TypeTemplate.IsEmpty()) return;

    Local<FunctionTemplate> t;

    t = Nan::New<FunctionTemplate>();
    t->SetClassName(Nan::New<String>("Type").ToLocalChecked());
    // native self
    // owning schema (so GC can manage our lifecyle)
    // constructor
    // toArray
    t->InstanceTemplate()->SetInternalFieldCount(4);
    TypeTemplate.Reset(t);

    t = Nan::New<FunctionTemplate>(Schema::NewSchema);
    t->SetClassName(Nan::New<String>("Schema").ToLocalChecked());
    // native self
    // array of types (so GC can manage our lifecyle)
    t->InstanceTemplate()->SetInternalFieldCount(2);
    Nan::SetNamedPropertyHandler(t->InstanceTemplate(), Schema::GetType);
    SchemaTemplate.Reset(t);

    t = Nan::New<FunctionTemplate>();
    t->SetClassName(Nan::New<String>("Schema").ToLocalChecked());
    // native self
    // array of types (so GC can manage our lifecyle)
    t->InstanceTemplate()->SetInternalFieldCount(2);
    Nan::SetNamedPropertyHandler(t->InstanceTemplate(), Schema::GetType);
    ServiceSchemaTemplate.Reset(t);

    t = Nan::New<FunctionTemplate>();
    t->SetClassName(Nan::New<String>("Service").ToLocalChecked());
    t->InstanceTemplate()->SetInternalFieldCount(2);
    ServiceTemplate.Reset(t);

    //MethodTemplate  = Persistent<FunctionTemplate>::New(FunctionTemplate::New(WrappedService::Invoke));

    t = Nan::New<FunctionTemplate>(Schema::Type::Parse);
    ParseTemplate.Reset(t);

    t = Nan::New<FunctionTemplate>(Schema::Type::Serialize);
    SerializeTemplate.Reset(t);

    //WrappedService::Init();
  }

  Local<Function> SchemaConstructor() {
    Init();
    Local<FunctionTemplate> schemaTemplate = Nan::New(SchemaTemplate);
    return schemaTemplate->GetFunction();
  }

  void ExportService(Local<Object> target, const char* name, Service* service) {
    Init();
    //target->Set(Nan::New<String>(name), (new WrappedService(service))->handle_).ToLocalChecked();
  }

}  // namespace protobuf_for_node
