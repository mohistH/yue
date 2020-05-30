// Copyright 2020 Cheng Zhao. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NATIVEUI_WIN_BROWSER_BROWSER_IMPL_IE_H_
#define NATIVEUI_WIN_BROWSER_BROWSER_IMPL_IE_H_

#include <unknwn.h>
#include <exdisp.h>  // NOLINT
#include <mshtml.h>
#include <wrl.h>

#include "nativeui/win/browser/browser_document_events.h"
#include "nativeui/win/browser/browser_event_sink.h"
#include "nativeui/win/browser/browser_external_sink.h"
#include "nativeui/win/browser/browser_html_moniker.h"
#include "nativeui/win/browser/browser_ole_site.h"
#include "nativeui/win/browser_win.h"

namespace nu {

// Implementation of Browser based on IE.
class BrowserImplIE : public BrowserImpl {
 public:
  BrowserImplIE(const Browser::Options& options, Browser* delegate);
  ~BrowserImplIE() override;

  void LoadURL(base::string16 str) override;
  void LoadHTML(base::string16 str, base::string16 base_url) override;
  base::string16 GetURL() override;
  base::string16 GetTitle() override;
  bool Eval(base::string16 code, base::string16* result) override;

  void GoBack() override;
  bool CanGoBack() const override;
  void GoForward() override;
  bool CanGoForward() const override;
  void Reload() override;
  void Stop() override;
  bool IsLoading() const override;

  template<typename T>
  bool GetBrowser(Microsoft::WRL::ComPtr<T>* out) {
    return SUCCEEDED(browser_.As(out));
  }

  void set_can_go_back(bool b) { can_go_back_ = b; }
  void set_can_go_forward(bool b) { can_go_forward_ = b; }

 protected:
  // ViewImpl:
  void SizeAllocate(const Rect& bounds) override;
  bool HasFocus() const override;
  bool OnMouseWheel(NativeEvent event) override;

  // SubwinView:
  LRESULT OnMouseWheelFromSelf(
      UINT message, WPARAM w_param, LPARAM l_param) override;

  CR_BEGIN_MSG_MAP_EX(BrowserImplIE, SubwinView)
    CR_MSG_WM_DESTROY(OnDestroy)
    CR_MSG_WM_SETFOCUS(OnSetFocus)
    CR_MESSAGE_HANDLER_EX(WM_PARENTNOTIFY, OnParentNotify)
  CR_END_MSG_MAP()

 private:
  friend class BrowserEventSink;

  void OnDestroy();
  void OnSetFocus(HWND hwnd);
  LRESULT OnParentNotify(UINT msg, WPARAM w_param, LPARAM l_param);
  LRESULT IgnoreEvent(UINT msg, WPARAM w_param, LPARAM l_param);

  // Get the HWND of the IE control and add hooks.
  void ReceiveBrowserHWND();

  // Cleanup the hooks on IE control.
  void CleanupBrowserHWND();

  // Called when document is ready.
  void OnDocumentReady();

  // The proc of IE control.
  static LRESULT CALLBACK BrowserWndProc(HWND hwnd,
                                         UINT message,
                                         WPARAM w_param,
                                         LPARAM l_param);

  Microsoft::WRL::ComPtr<BrowserExternalSink> external_sink_;
  Microsoft::WRL::ComPtr<BrowserOleSite> ole_site_;
  Microsoft::WRL::ComPtr<BrowserEventSink> event_sink_;
  Microsoft::WRL::ComPtr<BrowserDocumentEvents> document_events_;
  Microsoft::WRL::ComPtr<BrowserHTMLMoniker> html_moniker_;
  HWND browser_hwnd_ = NULL;
  WNDPROC browser_proc_ = nullptr;

  Microsoft::WRL::ComPtr<IWebBrowser2> browser_;
  Microsoft::WRL::ComPtr<IHTMLDocument2> document_;

  // Whether we have loaded the HTML.
  bool is_html_loaded_ = false;

  // Browser states.
  bool can_go_back_ = false;
  bool can_go_forward_ = false;
};

}  // namespace nu

#endif  // NATIVEUI_WIN_BROWSER_BROWSER_IMPL_IE_H_