import { d as c, u as v, a as g, r as w, o as _, c as b, b as s, w as x, e as i, v as l, f as u, g as V, h as k, i as d } from "./index-DbbiUtAi.js";
import { c as y, u as T, _ as h } from "./_plugin-vue_export-helper-DoCpYK5i.js";
const S = { class: "register-container" }, C = { class: "register-card" }, N = { class: "form-group" }, R = { class: "form-group" }, U = { class: "register-footer" }, B = c({ __name: "RegisterView", setup(j) {
  const o = d(""), r = d(""), p = v(), a = g(), m = () => {
    fetch(`${y.apiUrl}/user/signup`, { method: "POST", headers: { "Content-Type": "application/json" }, body: JSON.stringify({ account: o.value, password: r.value }) }).then((t) => t.json()).then((t) => {
      if (t.status === 200) {
        const e = T();
        a(t.message), e.$patch({ account: o.value, token: t.data.token }), p.push("/view");
      } else a(t.message);
    });
  };
  return (t, e) => {
    const f = w("router-link");
    return _(), b("div", S, [s("div", C, [e[7] || (e[7] = s("h1", { class: "register-title" }, "\u6CE8\u518C", -1)), s("form", { class: "register-form", onSubmit: x(m, ["prevent"]) }, [s("div", N, [e[2] || (e[2] = s("label", { for: "username" }, "\u7528\u6237\u540D", -1)), i(s("input", { id: "username", "onUpdate:modelValue": e[0] || (e[0] = (n) => o.value = n), type: "text", placeholder: "\u8BF7\u8F93\u5165\u7528\u6237\u540D", required: "" }, null, 512), [[l, o.value]])]), s("div", R, [e[3] || (e[3] = s("label", { for: "password" }, "\u5BC6\u7801", -1)), i(s("input", { id: "password", "onUpdate:modelValue": e[1] || (e[1] = (n) => r.value = n), type: "password", placeholder: "\u8BF7\u8F93\u5165\u5BC6\u7801", required: "" }, null, 512), [[l, r.value]])]), e[4] || (e[4] = s("button", { type: "submit", class: "register-btn" }, "\u6CE8\u518C", -1))], 32), s("div", U, [s("p", null, [e[6] || (e[6] = u("\u5DF2\u6709\u8D26\u53F7\uFF1F", -1)), V(f, { to: "/login" }, { default: k(() => [...e[5] || (e[5] = [u("\u7ACB\u5373\u767B\u5F55", -1)])]), _: 1 })])])])]);
  };
} }), O = h(B, [["__scopeId", "data-v-bfb5c9e8"]]);
export {
  O as default
};
