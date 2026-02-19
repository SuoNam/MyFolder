import { j as n, H as u, i as s, p as l } from "./index-DbbiUtAi.js";
const g = { apiUrl: "/api" }, m = n("user", () => {
  const o = localStorage.getItem("user-token"), a = localStorage.getItem("user-account"), e = s(""), t = s("");
  o && a && (e.value = a, t.value = o);
  const c = l(() => !!t.value);
  return u(t, (r) => {
    r && (localStorage.setItem("user-token", t.value), localStorage.setItem("user-account", e.value));
  }), { account: e, token: t, isLogin: c };
}), f = (o, a) => {
  const e = o.__vccOpts || o;
  for (const [t, c] of a) e[t] = c;
  return e;
};
export {
  f as _,
  g as c,
  m as u
};
