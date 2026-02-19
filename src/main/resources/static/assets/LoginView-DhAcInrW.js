import { d as c, u as g, a as v, r as w, o as b, c as _, b as s, w as x, e as i, v as l, f as u, g as V, h, i as d } from "./index-DbbiUtAi.js";
import { c as k, u as y, _ as T } from "./_plugin-vue_export-helper-DoCpYK5i.js";
const S = { class: "login-container" }, C = { class: "login-card" }, N = { class: "form-group" }, U = { class: "form-group" }, B = { class: "login-footer" }, L = c({ __name: "LoginView", setup(j) {
  const t = d(""), n = d(""), p = g(), r = v(), m = () => {
    fetch(`${k.apiUrl}/user/login`, { method: "POST", headers: { "Content-Type": "application/json" }, body: JSON.stringify({ account: t.value, password: n.value }) }).then((o) => o.json()).then((o) => {
      if (o.status === 200) {
        const e = y();
        r(o.message), e.$patch({ account: t.value, token: o.data.token }), p.push("/view");
      } else r(o.message);
    }).catch((o) => {
      r.error(o.message);
    });
  };
  return (o, e) => {
    const f = w("router-link");
    return b(), _("div", S, [s("div", C, [e[7] || (e[7] = s("h1", { class: "login-title" }, "\u767B\u5F55", -1)), s("form", { class: "login-form", onSubmit: x(m, ["prevent"]) }, [s("div", N, [e[2] || (e[2] = s("label", { for: "username" }, "\u7528\u6237\u540D", -1)), i(s("input", { id: "username", "onUpdate:modelValue": e[0] || (e[0] = (a) => t.value = a), type: "text", placeholder: "\u8BF7\u8F93\u5165\u7528\u6237\u540D", required: "" }, null, 512), [[l, t.value]])]), s("div", U, [e[3] || (e[3] = s("label", { for: "password" }, "\u5BC6\u7801", -1)), i(s("input", { id: "password", "onUpdate:modelValue": e[1] || (e[1] = (a) => n.value = a), type: "password", placeholder: "\u8BF7\u8F93\u5165\u5BC6\u7801", required: "" }, null, 512), [[l, n.value]])]), e[4] || (e[4] = s("button", { type: "submit", class: "login-btn" }, "\u767B\u5F55", -1))], 32), s("div", B, [s("p", null, [e[6] || (e[6] = u("\u8FD8\u6CA1\u6709\u8D26\u53F7\uFF1F", -1)), V(f, { to: "/register" }, { default: h(() => [...e[5] || (e[5] = [u("\u7ACB\u5373\u6CE8\u518C", -1)])]), _: 1 })])])])]);
  };
} }), O = T(L, [["__scopeId", "data-v-ebb7b3e3"]]);
export {
  O as default
};
