/******************************************************************************
 *   Copyright (C) 2026 Nils Schimmelmann <nschimme@gmail.com>                *
 *                                                                            *
 * This program is distributed in the hope that it will be useful, but        *
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY *
 * or FITNESS FOR A PARTICULAR PURPOSE. For licensing and distribution        *
 * details, check the accompanying file 'COPYING'.                            *
 *****************************************************************************/

#include "keychain_p.h"

#include <emscripten.h>

using namespace QKeychain;

// These functions will be called from JavaScript to complete the async operations.
extern "C" {

EMSCRIPTEN_KEEPALIVE
void qtkeychain_read_password_success(JobPrivate *job, const char *data, int length)
{
    job->data = QByteArray(data, length);
    job->q->emitFinished();
}

EMSCRIPTEN_KEEPALIVE
void qtkeychain_write_password_success(JobPrivate *job)
{
    job->q->emitFinished();
}

EMSCRIPTEN_KEEPALIVE
void qtkeychain_error(JobPrivate *job, int error, const char *errorString)
{
    job->q->emitFinishedWithError(static_cast<Error>(error), QString::fromUtf8(errorString));
}
}

namespace {

/*
 * hide_bridge_form: Closes the Bridge UI for a specific job.
 * This is used for lifecycle tracking: if the C++ Job object is destroyed
 * before the user interacts with the UI, we must close it to prevent
 * JavaScript callbacks from accessing a stale pointer.
 */
EM_JS(void, hide_bridge_form, (JobPrivate * job), {
    if (!window.qtk_active_jobs)
        return;
    const cleanup = window.qtk_active_jobs.get(job);
    if (cleanup) {
        cleanup();
        window.qtk_active_jobs.delete(job);
    }
});

/*
 * show_bridge_form: Displays a transient HTML modal (the "Bridge UI").
 * Browsers only offer to save credentials if they detect a form submission,
 * and secure APIs like 'navigator.credentials.store' require a user gesture.
 * To ensure compatibility with strict Content Security Policies (CSP), we use the
 * DOM API.
 */
EM_JS(void, show_bridge_form,
      (JobPrivate * job, const char *service, const char *key, const char *data, int dataLen,
       bool isWrite, int entryNotFound, int accessDeniedByUser),
      {
          const serviceStr = UTF8ToString(service);
          const keyStr = UTF8ToString(key);

          const toCStr = (str) => {
              const len = lengthBytesUTF8(str) + 1;
              const ptr = _malloc(len);
              if (!ptr)
                  return null;
              stringToUTF8(str, ptr, len);
              const res = { ptr: ptr, len: len - 1 };
              return res;
          };

          const createEl = (tag, parent, props, style) => {
              const el = document.createElement(tag);
              if (props)
                  Object.assign(el, props);
              if (style)
                  Object.assign(el.style, style);
              if (parent)
                  parent.appendChild(el);
              return el;
          };

          const hasCredAPI = (api) =>
              !!(navigator.credentials && navigator.credentials[api] &&
                 typeof PasswordCredential !== "undefined");

          const overlay = createEl("div", document.body, { id: "qtk-overlay" }, {
              position: "fixed", inset: "0", background: "var(--qtk-overlay-bg, rgba(0,0,0,0.5))",
              display: "flex", justifyContent: "center", alignItems: "center",
              zIndex: "2147483647", fontFamily: "var(--qtk-font-family, system-ui, sans-serif)"
          });

          const modal = createEl("div", overlay, { id: "qtk-modal" }, {
              background: "var(--qtk-modal-bg, #fff)", padding: "24px",
              borderRadius: "var(--qtk-modal-radius, 12px)",
              boxShadow: "var(--qtk-modal-shadow, 0 8px 30px rgba(0,0,0,0.3))",
              width: "320px", color: "var(--qtk-modal-color, #333)"
          });

          createEl("h2", modal, { id: "qtk-title", textContent: serviceStr },
                   { margin: "0 0 16px", fontSize: "1.1em", fontWeight: "600" });

          const form = createEl("form", modal, { id: "qtk-form" });
          if (isWrite) {
              form.target = "qtk-iframe";
              form.action = "about:blank";
          }

          const labelStyle = { display: "block", marginBottom: "6px", fontSize: "0.9em", fontWeight: "500" };
          const inputStyle = {
              width: "100%", marginBottom: "16px", padding: "10px",
              border: "var(--qtk-input-border, 1px solid #ccc)",
              borderRadius: "var(--qtk-input-radius, 6px)",
              boxSizing: "border-box", fontSize: "1em"
          };

          createEl("label", form, { htmlFor: "qtk-user", textContent: "Username" }, labelStyle);
          const userInput = createEl("input", form, {
              id: "qtk-user", name: "username", autocomplete: "username", value: keyStr
          }, inputStyle);

          createEl("label", form, { htmlFor: "qtk-pass", textContent: "Password" }, labelStyle);
          const passInput = createEl("input", form, {
              id: "qtk-pass", type: "password", name: "password",
              autocomplete: isWrite ? "new-password" : "current-password"
          }, inputStyle);
          if (isWrite && dataLen > 0) {
              const dataBytes = HEAPU8.subarray(data, data + dataLen);
              passInput.value = new TextDecoder().decode(dataBytes);
          }

          const extraDiv = createEl("div", form, { id: "qtk-extra" });

          const actions = createEl("div", form, { className: "actions" },
                                   { display: "flex", flexDirection: "row-reverse", gap: "8px" });

          const btnStyle = {
              flex: "1", padding: "10px", borderRadius: "var(--qtk-btn-radius, 6px)", border: "none",
              fontSize: "1em", cursor: "pointer", fontWeight: "500"
          };

          const createBtn = (parent, props, style) => createEl("button", parent,
              Object.assign({ type: "button" }, props), Object.assign({}, btnStyle, style));

          const submitBtn = createBtn(actions, {
              type: "submit", className: "qtk-primary", textContent: isWrite ? "Save" : "Sign In"
          }, { background: "var(--qtk-primary-bg, #007aff)", color: "var(--qtk-primary-color, #fff)" });

          const cancelBtn = createBtn(actions, {
              id: "qtk-cancel", className: "qtk-secondary", textContent: "Cancel"
          }, { background: "var(--qtk-secondary-bg, #e5e5ea)", color: "var(--qtk-secondary-color, #333)" });

          if (isWrite) {
              createEl("iframe", modal, { name: "qtk-iframe" }, { display: "none" });
          }

          const cleanup = () => {
              userInput.value = "";
              passInput.value = "";
              overlay.remove();
              if (window.qtk_active_jobs)
                  window.qtk_active_jobs.delete(job);
          };

          if (!window.qtk_active_jobs)
              window.qtk_active_jobs = new Map();
          window.qtk_active_jobs.set(job, cleanup);

          const finishWithError = (code, msg) => {
              cleanup();
              console.error("QtKeychain:", msg);
              const res = toCStr(msg);
              if (res) {
                  _qtkeychain_error(job, code, res.ptr);
                  _free(res.ptr);
              } else {
                  _qtkeychain_error(job, code, 0);
              }
          };

          cancelBtn.onclick = () => {
              console.log("QtKeychain: Cancelled");
              finishWithError(accessDeniedByUser, "User cancelled");
          };

          const isJobAlive = () => !!(window.qtk_active_jobs && window.qtk_active_jobs.has(job));

          if (!isWrite && hasCredAPI("get")) {
              const btn = createBtn(extraDiv, {
                  className: "qtk-success", textContent: "Use a saved password..."
              }, {
                  background: "var(--qtk-success-bg, #28a745)",
                  color: "var(--qtk-success-color, #fff)",
                  marginBottom: "16px", width: "100%", display: "block"
              });
              btn.onclick = () => {
                  console.log("QtKeychain: Requesting credentials...");
                  navigator.credentials.get({ password: true })
                      .then(cred => {
                          if (cred && isJobAlive()) {
                              userInput.value = cred.id || "";
                              passInput.value = cred.password || "";
                              form.dispatchEvent(new Event("submit"));
                          }
                      })
                      .catch(err => console.error("QtKeychain:", err));
              };
          }

          form.onsubmit = (e) => {
              const useNativeStore = isWrite && hasCredAPI("store");
              if (!isWrite || useNativeStore)
                  e.preventDefault();

              console.log("QtKeychain: Submitting...", { isWrite: isWrite, user: userInput.value });

              if (isWrite) {
                  const done = () => {
                      console.log("QtKeychain: Save complete");
                      if (!isJobAlive())
                          return;
                      cleanup();
                      _qtkeychain_write_password_success(job);
                  };
                  if (useNativeStore) {
                      navigator.credentials
                          .store(new PasswordCredential({
                              id: userInput.value,
                              password: passInput.value,
                              name: userInput.value
                          }))
                          .catch(err => console.error("QtKeychain:", err))
                          .finally(done);
                  } else {
                      setTimeout(done, 100);
                  }
              } else {
                  if (passInput.value) {
                      const res = toCStr(passInput.value);
                      cleanup();
                      if (res) {
                          _qtkeychain_read_password_success(job, res.ptr, res.len);
                          _free(res.ptr);
                      } else {
                          _qtkeychain_error(job, entryNotFound, 0);
                      }
                  } else {
                      finishWithError(entryNotFound, "Password cannot be empty");
                  }
              }
          };

          const focusInput = (keyStr && !isWrite) ? passInput : userInput;
          focusInput.focus();
      });

} // namespace

void ReadPasswordJobPrivate::scheduledStart()
{
    QObject::connect(this, &QObject::destroyed, [this] { hide_bridge_form(this); });
    show_bridge_form(this, service.toUtf8().constData(), key.toUtf8().constData(), "", 0, false,
                     EntryNotFound, AccessDeniedByUser);
}

void WritePasswordJobPrivate::scheduledStart()
{
    QObject::connect(this, &QObject::destroyed, [this] { hide_bridge_form(this); });
    show_bridge_form(this, service.toUtf8().constData(), key.toUtf8().constData(), data.constData(),
                     data.length(), true, EntryNotFound, AccessDeniedByUser);
}

void DeletePasswordJobPrivate::scheduledStart()
{
    q->emitFinishedWithError(NotImplemented, "Delete is not supported by browser sandboxes");
}

bool QKeychain::isAvailable()
{
    return true;
}
