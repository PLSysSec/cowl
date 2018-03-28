// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/fetch/Body.h"

#include <memory>
#include "base/memory/scoped_refptr.h"
#include "bindings/core/v8/ScriptPromiseResolver.h"
#include "bindings/core/v8/V8ArrayBuffer.h"
#include "core/dom/ExecutionContext.h"
#include "core/cowl/COWLParser.h"
#include "core/cowl/LabeledObject.h"
#include "core/fileapi/Blob.h"
#include "core/html/forms/FormData.h"
#include "core/typed_arrays/DOMArrayBuffer.h"
#include "core/typed_arrays/DOMTypedArray.h"
#include "core/url/URLSearchParams.h"
#include "modules/fetch/BodyStreamBuffer.h"
#include "modules/fetch/FetchDataLoader.h"
#include "platform/bindings/ScriptState.h"
#include "platform/bindings/V8ThrowException.h"
#include "platform/network/ParsedContentType.h"
#include "public/platform/WebDataConsumerHandle.h"

namespace blink {

namespace {

class BodyConsumerBase : public GarbageCollectedFinalized<BodyConsumerBase>,
                         public FetchDataLoader::Client {
  WTF_MAKE_NONCOPYABLE(BodyConsumerBase);
  USING_GARBAGE_COLLECTED_MIXIN(BodyConsumerBase);

 public:
  explicit BodyConsumerBase(ScriptPromiseResolver* resolver)
      : resolver_(resolver) {}
  ScriptPromiseResolver* Resolver() { return resolver_; }
  void DidFetchDataLoadFailed() override {
    ScriptState::Scope scope(Resolver()->GetScriptState());
    resolver_->Reject(V8ThrowException::CreateTypeError(
        Resolver()->GetScriptState()->GetIsolate(), "Failed to fetch"));
  }

  void Trace(blink::Visitor* visitor) override {
    visitor->Trace(resolver_);
    FetchDataLoader::Client::Trace(visitor);
  }

 private:
  Member<ScriptPromiseResolver> resolver_;
};

class BodyBlobConsumer final : public BodyConsumerBase {
  WTF_MAKE_NONCOPYABLE(BodyBlobConsumer);

 public:
  explicit BodyBlobConsumer(ScriptPromiseResolver* resolver)
      : BodyConsumerBase(resolver) {}

  void DidFetchDataLoadedBlobHandle(
      scoped_refptr<BlobDataHandle> blob_data_handle) override {
    Resolver()->Resolve(Blob::Create(std::move(blob_data_handle)));
  }
};

class BodyArrayBufferConsumer final : public BodyConsumerBase {
  WTF_MAKE_NONCOPYABLE(BodyArrayBufferConsumer);

 public:
  explicit BodyArrayBufferConsumer(ScriptPromiseResolver* resolver)
      : BodyConsumerBase(resolver) {}

  void DidFetchDataLoadedArrayBuffer(DOMArrayBuffer* array_buffer) override {
    Resolver()->Resolve(array_buffer);
  }
};

class BodyFormDataConsumer final : public BodyConsumerBase {
  WTF_MAKE_NONCOPYABLE(BodyFormDataConsumer);

 public:
  explicit BodyFormDataConsumer(ScriptPromiseResolver* resolver)
      : BodyConsumerBase(resolver) {}

  void DidFetchDataLoadedFormData(FormData* formData) override {
    Resolver()->Resolve(formData);
  }

  void DidFetchDataLoadedString(const String& string) override {
    FormData* formData = FormData::Create();
    for (const auto& pair : URLSearchParams::Create(string)->Params())
      formData->append(pair.first, pair.second);
    DidFetchDataLoadedFormData(formData);
  }
};

class BodyTextConsumer final : public BodyConsumerBase {
  WTF_MAKE_NONCOPYABLE(BodyTextConsumer);

 public:
  explicit BodyTextConsumer(ScriptPromiseResolver* resolver)
      : BodyConsumerBase(resolver) {}

  void DidFetchDataLoadedString(const String& string) override {
    Resolver()->Resolve(string);
  }
};

class BodyJsonConsumer final : public BodyConsumerBase {
  WTF_MAKE_NONCOPYABLE(BodyJsonConsumer);

 public:
  explicit BodyJsonConsumer(ScriptPromiseResolver* resolver)
      : BodyConsumerBase(resolver) {}

  void DidFetchDataLoadedString(const String& string) override {
    if (!Resolver()->GetExecutionContext() ||
        Resolver()->GetExecutionContext()->IsContextDestroyed())
      return;
    ScriptState::Scope scope(Resolver()->GetScriptState());
    v8::Isolate* isolate = Resolver()->GetScriptState()->GetIsolate();
    v8::Local<v8::String> input_string = V8String(isolate, string);
    v8::TryCatch trycatch(isolate);
    v8::Local<v8::Value> parsed;
    if (v8::JSON::Parse(isolate, input_string).ToLocal(&parsed))
      Resolver()->Resolve(parsed);
    else
      Resolver()->Reject(trycatch.Exception());
  }
};

class BodyLabeledJsonConsumer final : public BodyConsumerBase {
  WTF_MAKE_NONCOPYABLE(BodyLabeledJsonConsumer);

 public:
  explicit BodyLabeledJsonConsumer(ScriptPromiseResolver* resolver)
      : BodyConsumerBase(resolver) {}

  void DidFetchDataLoadedLabeledJson(const String& ljson, const String& origin) override {
    if (!Resolver()->GetExecutionContext() ||
        Resolver()->GetExecutionContext()->IsContextDestroyed())
      return;
    ScriptState::Scope scope(Resolver()->GetScriptState());
    v8::Isolate* isolate = Resolver()->GetScriptState()->GetIsolate();
    v8::Local<v8::String> ljson_string = V8String(isolate, ljson);
    v8::TryCatch trycatch(isolate);
    v8::Local<v8::Value> ljson_value;
    if (!v8::JSON::Parse(isolate, ljson_string).ToLocal(&ljson_value))
      Resolver()->Reject(trycatch.Exception());

  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  v8::Local<v8::Object> response_object = ljson_value.As<v8::Object>();

  v8::Local<v8::Value> conf_value;
  if (!response_object->Get(context, v8::String::NewFromUtf8(isolate, "confidentiality")).ToLocal(&conf_value))
    Resolver()->Reject(trycatch.Exception());
  String conf_expr = ToCoreStringWithNullCheck(conf_value.As<v8::String>());

  v8::Local<v8::Value> integrity_value;
  if (!response_object->Get(context, v8::String::NewFromUtf8(isolate, "integrity")).ToLocal(&integrity_value))
    Resolver()->Reject(trycatch.Exception());
  String integrity_expr = ToCoreStringWithNullCheck(integrity_value.As<v8::String>());

  v8::Local<v8::Value> obj_value;
  if (!response_object->Get(context, v8::String::NewFromUtf8(isolate, "object")).ToLocal(&obj_value))
    Resolver()->Reject(trycatch.Exception());

  ScriptValue obj = ScriptValue(Resolver()->GetScriptState(), obj_value);

  Label* conf = COWLParser::ParseLabelExpression(conf_expr, origin);
  Label* integrity  = COWLParser::ParseLabelExpression(integrity_expr, origin);

  CILabel labels;
  labels.setConfidentiality(conf);
  labels.setIntegrity(integrity);

  //ExceptionState exception_state(info.GetIsolate(),
  //    ExceptionState::kGetterContext,
  //    "XMLHttpRequest", "response");
  LabeledObject* lobj = LabeledObject::Create(Resolver()->GetScriptState(), obj, labels,  ASSERT_NO_EXCEPTION);
  if (!lobj)
    Resolver()->Reject(trycatch.Exception());

   Resolver()->Resolve(lobj);
  }
};

}  // namespace

ScriptPromise Body::arrayBuffer(ScriptState* script_state) {
  ScriptPromise promise = RejectInvalidConsumption(script_state);
  if (!promise.IsEmpty())
    return promise;

  // When the main thread sends a V8::TerminateExecution() signal to a worker
  // thread, any V8 API on the worker thread starts returning an empty
  // handle. This can happen in this function. To avoid the situation, we
  // first check the ExecutionContext and return immediately if it's already
  // gone (which means that the V8::TerminateExecution() signal has been sent
  // to this worker thread).
  if (!ExecutionContext::From(script_state))
    return ScriptPromise();

  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  promise = resolver->Promise();

  if (MimeType() == "application/labeled-json")
    resolver->Reject(V8ThrowException::CreateError(script_state->GetIsolate(),
                                                   "The value is only accessible using labeledObject()"));

  if (BodyBuffer()) {
    BodyBuffer()->StartLoading(FetchDataLoader::CreateLoaderAsArrayBuffer(),
                               new BodyArrayBufferConsumer(resolver));
  } else {
    resolver->Resolve(DOMArrayBuffer::Create(0u, 1));
  }
  return promise;
}

ScriptPromise Body::blob(ScriptState* script_state) {
  ScriptPromise promise = RejectInvalidConsumption(script_state);
  if (!promise.IsEmpty())
    return promise;

  // See above comment.
  if (!ExecutionContext::From(script_state))
    return ScriptPromise();

  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  promise = resolver->Promise();

  if (MimeType() == "application/labeled-json")
    resolver->Reject(V8ThrowException::CreateError(script_state->GetIsolate(),
                                                   "The value is only accessible using labeledObject()"));

  if (BodyBuffer()) {
    BodyBuffer()->StartLoading(
        FetchDataLoader::CreateLoaderAsBlobHandle(MimeType()),
        new BodyBlobConsumer(resolver));
  } else {
    std::unique_ptr<BlobData> blob_data = BlobData::Create();
    blob_data->SetContentType(MimeType());
    resolver->Resolve(
        Blob::Create(BlobDataHandle::Create(std::move(blob_data), 0)));
  }
  return promise;
}

ScriptPromise Body::formData(ScriptState* script_state) {
  ScriptPromise promise = RejectInvalidConsumption(script_state);
  if (!promise.IsEmpty())
    return promise;

  // See above comment.
  if (!ExecutionContext::From(script_state))
    return ScriptPromise();

  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  const ParsedContentType parsedTypeWithParameters(ContentType());
  const String parsedType = parsedTypeWithParameters.MimeType().LowerASCII();
  promise = resolver->Promise();

  if (MimeType() == "application/labeled-json")
    resolver->Reject(V8ThrowException::CreateError(script_state->GetIsolate(),
                                                   "The value is only accessible using labeledObject()"));

  if (parsedType == "multipart/form-data") {
    const String boundary =
        parsedTypeWithParameters.ParameterValueForName("boundary");
    if (BodyBuffer() && !boundary.IsEmpty()) {
      BodyBuffer()->StartLoading(
          FetchDataLoader::CreateLoaderAsFormData(boundary),
          new BodyFormDataConsumer(resolver));
      return promise;
    }
  } else if (parsedType == "application/x-www-form-urlencoded") {
    if (BodyBuffer()) {
      BodyBuffer()->StartLoading(FetchDataLoader::CreateLoaderAsString(),
                                 new BodyFormDataConsumer(resolver));
    } else {
      resolver->Resolve(FormData::Create());
    }
    return promise;
  } else {
    if (BodyBuffer()) {
      BodyBuffer()->StartLoading(FetchDataLoader::CreateLoaderAsFailure(),
                                 new BodyFormDataConsumer(resolver));
      return promise;
    }
  }

  resolver->Reject(V8ThrowException::CreateTypeError(script_state->GetIsolate(),
                                                     "Invalid MIME type"));
  return promise;
}

ScriptPromise Body::json(ScriptState* script_state) {
  ScriptPromise promise = RejectInvalidConsumption(script_state);
  if (!promise.IsEmpty())
    return promise;

  // See above comment.
  if (!ExecutionContext::From(script_state))
    return ScriptPromise();

  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  promise = resolver->Promise();

  if (MimeType() == "application/labeled-json")
    resolver->Reject(V8ThrowException::CreateError(script_state->GetIsolate(),
                                                   "The value is only accessible using labeledObject()"));

  if (BodyBuffer()) {
    BodyBuffer()->StartLoading(FetchDataLoader::CreateLoaderAsString(),
                               new BodyJsonConsumer(resolver));
  } else {
    resolver->Reject(V8ThrowException::CreateSyntaxError(
        script_state->GetIsolate(), "Unexpected end of input"));
  }
  return promise;
}

ScriptPromise Body::labeledObject(ScriptState* script_state) {
  ScriptPromise promise = RejectInvalidConsumption(script_state);
  if (!promise.IsEmpty())
    return promise;

  // See above comment.
  if (!ExecutionContext::From(script_state))
    return ScriptPromise();

  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  promise = resolver->Promise();
  if (BodyBuffer()) {
    BodyBuffer()->StartLoading(FetchDataLoader::CreateLoaderAsLabeledJson(MimeType(), Origin()),
                               new BodyLabeledJsonConsumer(resolver));
  } else {
    resolver->Reject(V8ThrowException::CreateSyntaxError(
        script_state->GetIsolate(), "Unexpected end of input"));
  }
  return promise;
}

ScriptPromise Body::text(ScriptState* script_state) {
  ScriptPromise promise = RejectInvalidConsumption(script_state);
  if (!promise.IsEmpty())
    return promise;

  // See above comment.
  if (!ExecutionContext::From(script_state))
    return ScriptPromise();

  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  promise = resolver->Promise();

  if (MimeType() == "application/labeled-json")
    resolver->Reject(V8ThrowException::CreateError(script_state->GetIsolate(),
                                                   "The value is only accessible using labeledObject()"));

  if (BodyBuffer()) {
    BodyBuffer()->StartLoading(FetchDataLoader::CreateLoaderAsString(),
                               new BodyTextConsumer(resolver));
  } else {
    resolver->Resolve(String());
  }
  return promise;
}

ScriptValue Body::body(ScriptState* script_state) {
  if (MimeType() == "application/labeled-json")
    return ScriptValue::CreateNull(script_state);

  if (!BodyBuffer())
    return ScriptValue::CreateNull(script_state);
  ScriptValue stream = BodyBuffer()->Stream();
  DCHECK_EQ(stream.GetScriptState(), script_state);
  return stream;
}

bool Body::bodyUsed() {
  return BodyBuffer() && BodyBuffer()->IsStreamDisturbed();
}

bool Body::IsBodyLocked() {
  return BodyBuffer() && BodyBuffer()->IsStreamLocked();
}

bool Body::HasPendingActivity() const {
  if (!GetExecutionContext() || GetExecutionContext()->IsContextDestroyed())
    return false;
  if (!BodyBuffer())
    return false;
  return BodyBuffer()->HasPendingActivity();
}

Body::Body(ExecutionContext* context) : ContextClient(context) {}

ScriptPromise Body::RejectInvalidConsumption(ScriptState* script_state) {
  const bool used = bodyUsed();
  if (IsBodyLocked() || used) {
    return ScriptPromise::Reject(
        script_state,
        V8ThrowException::CreateTypeError(
            script_state->GetIsolate(),
            used ? "body stream already read" : "body stream is locked"));
  }
  return ScriptPromise();
}

}  // namespace blink
