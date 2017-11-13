// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebFrameOwnerProperties_h
#define WebFrameOwnerProperties_h

#include "public/platform/WebFeaturePolicy.h"
#include "public/platform/WebScrollbar.h"
#include "public/platform/WebString.h"
#include "public/platform/WebVector.h"

#include <algorithm>

namespace blink {

struct WebFrameOwnerProperties {
  using ScrollingMode = WebScrollbar::ScrollingMode;

  WebString name;  // browsing context container's name
  ScrollingMode scrolling_mode;
  int margin_width;
  int margin_height;
  bool allow_fullscreen;
  bool allow_payment_request;
  bool cowl;
  bool is_display_none;
  WebString required_csp;

 public:
  WebVector<WebFeaturePolicyFeature> allowed_features;

  WebFrameOwnerProperties()
      : scrolling_mode(ScrollingMode::kAuto),
        margin_width(-1),
        margin_height(-1),
        allow_fullscreen(false),
        allow_payment_request(false),
        cowl(false),
        is_display_none(false) {}

#if INSIDE_BLINK
  WebFrameOwnerProperties(
      const WebString& name,
      ScrollbarMode scrolling_mode,
      int margin_width,
      int margin_height,
      bool allow_fullscreen,
      bool allow_payment_request,
      bool is_display_none,
      const WebString& required_csp,
      bool cowl,
      const WebVector<WebFeaturePolicyFeature>& allowed_features)
      : name(name),
        scrolling_mode(static_cast<ScrollingMode>(scrolling_mode)),
        margin_width(margin_width),
        margin_height(margin_height),
        allow_fullscreen(allow_fullscreen),
        allow_payment_request(allow_payment_request),
        cowl(cowl),
        is_display_none(is_display_none),
        required_csp(required_csp),
        allowed_features(allowed_features) {}
#endif
};

}  // namespace blink

#endif
