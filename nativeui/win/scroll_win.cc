// Copyright 2016 Cheng Zhao. All rights reserved.
// Use of this source code is governed by the license that can be found in the
// LICENSE file.

#include "nativeui/win/scroll_win.h"

#include <tuple>

#include "nativeui/gfx/geometry/size_conversions.h"
#include "nativeui/win/scroll_bar/scroll_bar.h"

namespace nu {

ScrollView::ScrollView(Scroll* delegate)
    : ContainerView(this, ControlType::Scroll),
      scrollbar_height_(GetSystemMetrics(SM_CXVSCROLL)),
      delegate_(delegate) {
  SetScrollBarPolicy(Scroll::Policy::Automatic, Scroll::Policy::Automatic);
}

void ScrollView::SetOrigin(const Vector2d& origin) {
  UpdateOrigin(origin);
  Layout();
  Invalidate();
}

void ScrollView::SetContentSize(const Size& size) {
  content_size_ = size;
  UpdateScrollbar();
  UpdateOrigin(origin_);
  Layout();
  Invalidate();
}

void ScrollView::SetScrollBarPolicy(Scroll::Policy h_policy,
                                    Scroll::Policy v_policy) {
  h_policy_ = h_policy;
  v_policy_ = v_policy;
  UpdateScrollbar();
  UpdateOrigin(origin_);
  Layout();
  Invalidate();
}

Rect ScrollView::GetViewportRect() const {
  Rect viewport(size_allocation());
  viewport.Inset(0, 0,
                 v_scrollbar_ ? scrollbar_height_ : 0,
                 h_scrollbar_ ? scrollbar_height_ : 0);
  return viewport;
}

void ScrollView::OnScroll(int x, int y) {
  if (UpdateOrigin(origin_ + Vector2d(x, y))) {
    Layout();
    Invalidate();
  }
}

void ScrollView::Layout() {
  if (h_scrollbar_)
    h_scrollbar_->SizeAllocate(GetScrollBarRect(false) +
                               size_allocation().OffsetFromOrigin());
  if (v_scrollbar_)
    v_scrollbar_->SizeAllocate(GetScrollBarRect(true) +
                               size_allocation().OffsetFromOrigin());

  if (delegate_->GetContentView()) {
    Rect content_alloc = Rect(size_allocation().origin() + origin_,
                              content_size_);
    delegate_->GetContentView()->view()->SizeAllocate(content_alloc);
  }
}

std::vector<BaseView*> ScrollView::GetChildren() {
  std::vector<BaseView*> children{delegate_->GetContentView()->view()};
  if (h_scrollbar_)
    children.push_back(h_scrollbar_.get());
  if (v_scrollbar_)
    children.push_back(v_scrollbar_.get());
  return children;
}

void ScrollView::SizeAllocate(const Rect& size_allocation) {
  BaseView::SizeAllocate(size_allocation);
  UpdateScrollbar();
  UpdateOrigin(origin_);
  Layout();
}

bool ScrollView::OnMouseWheel(bool vertical, UINT flags, int delta,
                              const Point& point) {
  if (vertical)
    OnScroll(0, delta);
  else
    OnScroll(-delta, 0);
  return true;
}

void ScrollView::Draw(PainterWin* painter, const Rect& dirty) {
  if (h_scrollbar_)
    DrawChild(h_scrollbar_.get(), painter, dirty);
  if (v_scrollbar_)
    DrawChild(v_scrollbar_.get(), painter, dirty);

  // The scroll view must be clipped.
  RectF clip(GetViewportRect() - size_allocation().OffsetFromOrigin());
  painter->ClipPixelRect(clip, Painter::CombineMode::Replace);
  DrawChild(delegate_->GetContentView()->view(), painter, dirty);
}

void ScrollView::UpdateScrollbar() {
  Rect viewport = GetViewportRect();
  bool show_h_scrollbar = (h_policy_ == Scroll::Policy::Always) ||
                          (h_policy_ == Scroll::Policy::Automatic &&
                           viewport.width() < content_size_.width());
  bool show_v_scrollbar = (v_policy_ == Scroll::Policy::Always) ||
                          (v_policy_ == Scroll::Policy::Automatic &&
                           viewport.height() < content_size_.height());
  if (show_h_scrollbar && !h_scrollbar_) {
    h_scrollbar_.reset(new ScrollBarView(false, this));
    h_scrollbar_->SetParent(this);
  } else if (!show_h_scrollbar) {
    h_scrollbar_.reset();
  }
  if (show_v_scrollbar && !v_scrollbar_) {
    v_scrollbar_.reset(new ScrollBarView(true, this));
    v_scrollbar_->SetParent(this);
  } else if (!show_v_scrollbar) {
    v_scrollbar_.reset();
  }
}

bool ScrollView::UpdateOrigin(Vector2d new_origin) {
  Rect viewport = GetViewportRect();
  if (-new_origin.x() + viewport.width() > content_size_.width())
    new_origin.set_x(viewport.width() - content_size_.width());
  if (new_origin.x() > 0)
    new_origin.set_x(0);
  if (-new_origin.y() + viewport.height() > content_size_.height())
    new_origin.set_y(viewport.height() - content_size_.height());
  if (new_origin.y() > 0)
    new_origin.set_y(0);

  if (new_origin == origin_)
    return false;
  origin_ = new_origin;
  return true;
}

Rect ScrollView::GetScrollBarRect(bool vertical) const {
  if (vertical)
    return Rect(size_allocation().width() - scrollbar_height_,
                0,
                scrollbar_height_,
                size_allocation().height() -
                (h_scrollbar_ ? scrollbar_height_ : 0));
  else
    return Rect(0,
                size_allocation().height() - scrollbar_height_,
                size_allocation().width() -
                (v_scrollbar_ ? scrollbar_height_ : 0),
                scrollbar_height_);
}

void ScrollView::DrawScrollBar(bool vertical, PainterWin* painter,
                               const Rect& dirty) {
  Rect rect = GetScrollBarRect(vertical);
  if (!rect.Intersects(dirty))
    return;
  painter->Save();
  painter->TranslatePixel(rect.OffsetFromOrigin());
  if (vertical)
    v_scrollbar_->Draw(painter, Rect(rect.size()));
  else
    h_scrollbar_->Draw(painter, Rect(rect.size()));
  painter->Restore();
}

///////////////////////////////////////////////////////////////////////////////
// Public Container API implementation.

void Scroll::PlatformInit() {
  TakeOverView(new ScrollView(this));
}

void Scroll::PlatformSetContentView(Container* container) {
  auto* scroll = static_cast<ScrollView*>(view());
  container->view()->SetParent(scroll);
  scroll->SetContentSize(container->view()->size_allocation().size());
}

void Scroll::SetContentSize(const SizeF& size) {
  auto* scroll = static_cast<ScrollView*>(view());
  scroll->SetContentSize(ToCeiledSize(ScaleSize(size, scroll->scale_factor())));
}

void Scroll::SetScrollBarPolicy(Policy h_policy, Policy v_policy) {
  auto* scroll = static_cast<ScrollView*>(view());
  scroll->SetScrollBarPolicy(h_policy, v_policy);
}

std::tuple<Scroll::Policy, Scroll::Policy> Scroll::GetScrollBarPolicy() const {
  auto* scroll = static_cast<ScrollView*>(view());
  return std::make_tuple(scroll->h_policy(), scroll->v_policy());
}

}  // namespace nu