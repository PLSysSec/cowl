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
  ApplyPolicySideEffectsToExecutionContext();
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

bool COWL::LabelRaiseWillResultInStuckContext(Label* conf, Privilege* priv) {
  LocalFrame* frame = execution_context_->ExecutingWindow()->GetFrame();
  if (!frame)
    return false;

  // If |document|'s browsing context is not a top-level browsing context, then return false.
  if (!frame->IsMainFrame())
    return false;

  Label* effective_label = conf->Downgrade(priv);
  return !effective_label->IsEmpty();
}

bool COWL::WriteCheck(Label* obj_conf, Label* obj_int) {
  Privilege* priv = privilege_;
  Label* current_conf = confidentiality_->Downgrade(priv);
  Label* current_int = integrity_->Upgrade(priv);

  if (!obj_conf->subsumes(current_conf) || !current_int->subsumes(obj_int))
    return false;

  return true;
}

bool COWL::AllowRequest(
    const ResourceRequest& request,
    SecurityViolationReportingPolicy reporting_policy) const {

  if (!IsEnabled()) return true;

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
