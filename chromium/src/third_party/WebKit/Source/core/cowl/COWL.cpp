/*
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301 USA
 */

#include "core/cowl/COWL.h"

#include "core/dom/Document.h"
#include "core/dom/ExecutionContext.h"
#include "core/dom/SecurityContext.h"
#include "core/frame/LocalDOMWindow.h"
#include "core/inspector/ConsoleMessage.h"
#include "platform/loader/fetch/ResourceRequest.h"
#include "platform/loader/fetch/ResourceResponse.h"
#include "platform/weborigin/SecurityOrigin.h"
#include "public/platform/WebURLRequest.h"

namespace blink {

COWL* COWL::Create() { return new COWL(); }

COWL::COWL()
  : enabled_(false),
    confidentiality_(nullptr),
    integrity_(nullptr),
    privilege_(nullptr),
    execution_context_(nullptr) {}

COWL::~COWL() {}

void COWL::BindToExecutionContext(ExecutionContext* execution_context) {
  execution_context_ = execution_context;
}

void COWL::SetupSelf(const SecurityOrigin& security_origin) {
  confidentiality_ = Label::Create();
  integrity_ = Label::Create();
  privilege_ = Privilege::Create(security_origin.ToString());
}

void COWL::ApplyPolicySideEffectsToExecutionContext() {
  DCHECK(execution_context_ &&
      execution_context_->GetSecurityContext().GetSecurityOrigin());

  SetupSelf(*execution_context_->GetSecurityContext().GetSecurityOrigin());
}

bool COWL::LabelRaiseWillResultInStuckContext(Label* conf, Privilege* priv) const {
  LocalFrame* frame = execution_context_->ExecutingWindow()->GetFrame();
  if (!frame)
    return false;

  // If |document|'s browsing context is not a top-level browsing context, then return false.
  if (!frame->IsMainFrame())
    return false;

  Label* effective_label = conf->Downgrade(priv);
  return !effective_label->IsEmpty();
}

bool COWL::WriteCheck(Label* obj_conf, Label* obj_int) const {
  Label* current_conf = confidentiality_->Downgrade(privilege_);
  Label* current_int = integrity_->Upgrade(privilege_);

  if (!obj_conf->subsumes(current_conf) || !current_int->subsumes(obj_int))
    return false;

  return true;
}

bool COWL::AllowRequest(
    const ResourceRequest& request,
    SecurityViolationReportingPolicy reporting_policy) const {

  if (!enabled_) return true;

  RefPtr<SecurityOrigin> origin = SecurityOrigin::Create(request.Url());

  Label* effective_conf = confidentiality_->Downgrade(privilege_);
  Label* dst_conf = Label::Create(origin->ToString(), ASSERT_NO_EXCEPTION);
  // TODO: dst_conf is nullptr when origin is not valid principal. is this ok?
  if (!dst_conf)
    return false;

  if (dst_conf->subsumes(effective_conf))
    return true;

  if (reporting_policy == SecurityViolationReportingPolicy::kReport) {
    String message =  "COWL::context labeled " + GetConfidentiality()->toString() +
      " attempted to leak data to a remote server: " + origin->ToString();

    LogToConsole(ConsoleMessage::Create(kSecurityMessageSource,
                                        kErrorMessageLevel,
                                        message));
  }
  return false;
}

bool COWL::AllowResponse(const ResourceRequest& request,
                         const ResourceResponse& response) {
  const AtomicString& sec_cowl = response.HttpHeaderField(HTTPNames::Sec_COWL);
  if (sec_cowl.IsEmpty())
    return true;

  CommaDelimitedHeaderSet headers;
  ParseCommaDelimitedHeader(sec_cowl, headers);

  String ctx_header, data_header;

  for (String header : headers) {
    if (header.StartsWith("ctx"))
      ctx_header = header;
    if (header.StartsWith("data"))
      data_header = header;
  }

  WebURLRequest::RequestContext request_context = request.GetRequestContext();
  if (request_context == WebURLRequest::kRequestContextLocation ||
      request_context == WebURLRequest::kRequestContextWorker ||
      request_context == WebURLRequest::kRequestContextServiceWorker ||
      (request_context == WebURLRequest::kRequestContextInternal &&
      request.GetFrameType() == WebURLRequest::kFrameTypeTopLevel)) {

    String self = SecurityOrigin::Create(response.Url())->ToString();

    Label* conf; Label* integrity; Privilege* priv;
    COWLParser::parseLabeledContextHeader(ctx_header, self, conf, integrity, priv);

    if (!conf || !integrity || !priv) {
      String message = "COWL::The server supplied a malformed Sec-COWL header.";
      LogToConsole(ConsoleMessage::Create(kSecurityMessageSource,
            kErrorMessageLevel,
            message));
      return false;
    }

    // If priv is not a delegated privilege of the state context privilege, return blocked 
    if (!privilege_->asLabel()->subsumes(priv->asLabel())) {
      String message = "COWL::The server supplied a privilege that it is not trusted for.";
      LogToConsole(ConsoleMessage::Create(kSecurityMessageSource,
            kErrorMessageLevel,
            message));
      return false;
    }

    Label* effective_label = conf->Downgrade(priv);
    if (!effective_label->IsEmpty()) {
      String message = "COWL::the server supplied a confidentiality label and privilege pair"
        "that would have led to a top-level stuck context.";
      LogToConsole(ConsoleMessage::Create(kSecurityMessageSource,
            kErrorMessageLevel,
            message));
      return false;
    }

    Label* effective_int = integrity_->Upgrade(privilege_);
    if (!effective_int->subsumes(integrity)) {
      String message = "COWL::The server supplied an integrity label that it is not trusted for.";
      LogToConsole(ConsoleMessage::Create(kSecurityMessageSource,
            kErrorMessageLevel,
            message));
      return false;
    }

    enabled_ = true;;
    confidentiality_ = conf;
    integrity_ = integrity;
    privilege_ = priv;

    return true;

  } else {
    String self = SecurityOrigin::Create(response.Url())->ToString();
    Label* conf; Label* integrity;
    COWLParser::parseLabeledDataHeader(data_header, self, conf, integrity);

    if (!conf || !integrity) {
      String message = "COWL::The server supplied a malformed Sec-COWL header.";
      LogToConsole(ConsoleMessage::Create(kSecurityMessageSource,
            kErrorMessageLevel,
            message));
      return false;
    }

    Label* effective_conf = conf->Downgrade(privilege_);
    if (confidentiality_->subsumes(effective_conf) && integrity->subsumes(integrity_))
      return true;
  }
  return false;
}

void COWL::AddCtxHeader(ResourceRequest& request) {
  if (!enabled_ || request.GetReferrerPolicy() == kReferrerPolicyNever)
    return;

  String ctx_header = String::Format(
      "ctx-confidentiality %s; "
      "ctx-integrity %s; "
      "ctx-privilege %s",
      confidentiality_->toString().Utf8().data(),
      integrity_->toString().Utf8().data(),
      privilege_->asLabel()->toString().Utf8().data()
      );
  request.SetHTTPHeaderField(HTTPNames::Sec_COWL, AtomicString(ctx_header));
}

void COWL::LogToConsole(const String& message,
    MessageLevel level) const {
  LogToConsole(ConsoleMessage::Create(kSecurityMessageSource, level, message));
}

void COWL::LogToConsole(ConsoleMessage* console_message,
    LocalFrame* frame) const {
  if (frame)
    frame->GetDocument()->AddConsoleMessage(console_message);
  else if (execution_context_)
    execution_context_->AddConsoleMessage(console_message);
}

SandboxFlags COWL::GetSandboxFlags() {
  SandboxFlags mask = kSandboxPlugins
                    | kSandboxDocumentDomain
                    | kSandboxOrigin
                    | kSandboxNavigation
                    | kSandboxTopNavigation
                    | kSandboxPropagatesToAuxiliaryBrowsingContexts;
  return mask;
}

bool COWL::IsEnabled() const { return enabled_; }
Label* COWL::GetConfidentiality() const { return confidentiality_; }
Label* COWL::GetIntegrity() const { return integrity_; }
Privilege* COWL::GetPrivilege() const { return privilege_; }

void COWL::Enable() { enabled_ = true; }
void COWL::SetConfidentiality(Label* label) { confidentiality_ = label; }
void COWL::SetIntegrity(Label* label) { integrity_ = label; }
void COWL::SetPrivilege(Privilege* priv) { privilege_ = priv; }

DEFINE_TRACE(COWL) { 
  visitor->Trace(confidentiality_);
  visitor->Trace(integrity_);
  visitor->Trace(privilege_);
  visitor->Trace(execution_context_);
}

}  // namespace blink
