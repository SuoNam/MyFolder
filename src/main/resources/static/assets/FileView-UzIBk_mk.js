const __vite__mapDeps=(i,m=__vite__mapDeps,d=(m.f||(m.f=["assets/index-DbbiUtAi.js","assets/index-_vZVzdzn.css"])))=>i.map(i=>d[i]);
var __defProp = Object.defineProperty;
var __defNormalProp = (obj, key, value) => key in obj ? __defProp(obj, key, { enumerable: true, configurable: true, writable: true, value }) : obj[key] = value;
var __publicField = (obj, key, value) => __defNormalProp(obj, typeof key !== "symbol" ? key + "" : key, value);
import { j as Kt, i as le, d as pe, k as Gt, o as $, c as L, b as F, f as Ce, n as he, l as Y, F as re, m as Me, t as z, p as fe, q as Xt, s as Yt, x as G, y as rt, w as ye, z as ct, g as ue, h as ge, T as wt, A as Zt, B as Oe, C as Qt, D as ut, E as On, r as Tn, _ as kn, a as ft, G as Pn, u as Nn, H as vt, __tla as __tla_0 } from "./index-DbbiUtAi.js";
import { u as ze, _ as dt, c as st } from "./_plugin-vue_export-helper-DoCpYK5i.js";
let Oi;
let __tla = Promise.all([
  (() => {
    try {
      return __tla_0;
    } catch {
    }
  })()
]).then(async () => {
  const Bn = Kt("clipboard", () => {
    const e = localStorage.getItem("clipboard"), t = le({
      mode: "none",
      items: []
    });
    try {
      e && JSON.parse(e) && (t.value = JSON.parse(e));
    } catch (o) {
      console.warn("Failed to parse clipboard data from localStorage:", o), localStorage.removeItem("clipboard");
    }
    window.addEventListener("storage", (o) => {
      if (o.key === "clipboard") try {
        const i = JSON.parse(o.newValue || "");
        i && i.mode && Array.isArray(i.items) && (t.value = i);
      } catch (i) {
        console.warn("Failed to parse clipboard data from storage event:", i);
      }
    });
    function n(o) {
      t.value.mode = "copy", t.value.items = o, localStorage.setItem("clipboard", JSON.stringify(t.value));
    }
    function r(o) {
      t.value.mode = "cut", t.value.items = o, localStorage.setItem("clipboard", JSON.stringify(t.value));
    }
    function s() {
      t.value.items = [], t.value.mode = "none", localStorage.setItem("clipboard", JSON.stringify(t.value));
    }
    return {
      clipboard: t,
      cut: r,
      copy: n,
      clear: s
    };
  });
  const Dn = "array", $n = "bit", _t = "bits", Un = "byte", Et = "bytes", _e = "", In = "exponent", Ln = "function", St = "iec", jn = "Invalid number", Mn = "Invalid rounding method", Ke = "jedec", zn = "object", Ft = ".", Hn = "round", qn = "s", Jn = "si", Vn = "kbit", Wn = "kB", Kn = " ", Gn = "string", Xn = "0", Ge = {
    symbol: {
      iec: {
        bits: [
          "bit",
          "Kibit",
          "Mibit",
          "Gibit",
          "Tibit",
          "Pibit",
          "Eibit",
          "Zibit",
          "Yibit"
        ],
        bytes: [
          "B",
          "KiB",
          "MiB",
          "GiB",
          "TiB",
          "PiB",
          "EiB",
          "ZiB",
          "YiB"
        ]
      },
      jedec: {
        bits: [
          "bit",
          "Kbit",
          "Mbit",
          "Gbit",
          "Tbit",
          "Pbit",
          "Ebit",
          "Zbit",
          "Ybit"
        ],
        bytes: [
          "B",
          "KB",
          "MB",
          "GB",
          "TB",
          "PB",
          "EB",
          "ZB",
          "YB"
        ]
      }
    },
    fullform: {
      iec: [
        "",
        "kibi",
        "mebi",
        "gibi",
        "tebi",
        "pebi",
        "exbi",
        "zebi",
        "yobi"
      ],
      jedec: [
        "",
        "kilo",
        "mega",
        "giga",
        "tera",
        "peta",
        "exa",
        "zetta",
        "yotta"
      ]
    }
  };
  function Yn(e, { bits: t = false, pad: n = false, base: r = -1, round: s = 2, locale: o = _e, localeOptions: i = {}, separator: l = _e, spacer: a = Kn, symbols: u = {}, standard: c = _e, output: d = Gn, fullform: m = false, fullforms: S = [], exponent: p = -1, roundingMethod: x = Hn, precision: y = 0 } = {}) {
    let C = p, U = Number(e), E = [], N = 0, B = _e;
    c === Jn ? (r = 10, c = Ke) : c === St || c === Ke ? r = 2 : r === 2 ? c = St : (r = 10, c = Ke);
    const h = r === 10 ? 1e3 : 1024, w = m === true, R = U < 0, _ = Math[x];
    if (typeof e != "bigint" && isNaN(e)) throw new TypeError(jn);
    if (typeof _ !== Ln) throw new TypeError(Mn);
    if (R && (U = -U), (C === -1 || isNaN(C)) && (C = Math.floor(Math.log(U) / Math.log(h)), C < 0 && (C = 0)), C > 8 && (y > 0 && (y += 8 - C), C = 8), d === In) return C;
    if (U === 0) E[0] = 0, B = E[1] = Ge.symbol[c][t ? _t : Et][C];
    else {
      N = U / (r === 2 ? Math.pow(2, C * 10) : Math.pow(1e3, C)), t && (N = N * 8, N >= h && C < 8 && (N = N / h, C++));
      const A = Math.pow(10, C > 0 ? s : 0);
      E[0] = _(N * A) / A, E[0] === h && C < 8 && p === -1 && (E[0] = 1, C++), B = E[1] = r === 10 && C === 1 ? t ? Vn : Wn : Ge.symbol[c][t ? _t : Et][C];
    }
    if (R && (E[0] = -E[0]), y > 0 && (E[0] = E[0].toPrecision(y)), E[1] = u[E[1]] || E[1], o === true ? E[0] = E[0].toLocaleString() : o.length > 0 ? E[0] = E[0].toLocaleString(o, i) : l.length > 0 && (E[0] = E[0].toString().replace(Ft, l)), n && s > 0) {
      const A = E[0].toString(), v = l || (A.match(/(\D)/g) || []).pop() || Ft, P = A.toString().split(v), b = P[1] || _e, T = b.length, j = s - T;
      E[0] = `${P[0]}${v}${b.padEnd(T + j, Xn)}`;
    }
    return w && (E[1] = S[C] ? S[C] : Ge.fullform[c][C] + (t ? $n : Un) + (E[0] === 1 ? _e : qn)), d === Dn ? E : d === zn ? {
      value: E[0],
      symbol: E[1],
      exponent: C,
      unit: B
    } : E.join(a);
  }
  const Zn = {
    class: "breadcrumb"
  }, Qn = [
    "onClick"
  ], er = pe({
    __name: "Breadcrumb",
    emits: [
      "navigate"
    ],
    setup(e, { emit: t }) {
      let n = Gt(), r = fe(() => decodeURI(n.path).substring(5).split("/").filter(Boolean)), s = t;
      const o = (l) => l < 0 ? "/" : "/" + r.value.slice(0, l + 1).join("/"), i = (l) => {
        s("navigate", l);
      };
      return (l, a) => ($(), L("nav", Zn, [
        F("span", {
          class: he([
            "breadcrumb-item",
            {
              active: !Y(r).length
            }
          ]),
          onClick: a[0] || (a[0] = (u) => i("/"))
        }, [
          ...a[1] || (a[1] = [
            F("span", {
              class: "breadcrumb-icon"
            }, "\u{1F3E0}", -1),
            Ce(" / ", -1)
          ])
        ], 2),
        ($(true), L(re, null, Me(Y(r), (u, c) => ($(), L(re, {
          key: c
        }, [
          a[2] || (a[2] = F("span", {
            class: "breadcrumb-separator"
          }, ">", -1)),
          F("span", {
            class: he([
              "breadcrumb-item",
              {
                active: c === Y(r).length - 1
              }
            ]),
            onClick: (d) => i(o(c))
          }, z(u), 11, Qn)
        ], 64))), 128))
      ]));
    }
  }), tr = {
    key: 0,
    class: "drag-overlay"
  }, nr = pe({
    __name: "FileDragContainer",
    emits: [
      "drop"
    ],
    setup(e, { emit: t }) {
      let n = 0;
      const r = le(false), s = t, o = (m) => {
        var _a;
        m.preventDefault(), m.stopPropagation(), ((_a = m.dataTransfer) == null ? void 0 : _a.types.includes("Files")) && (n++, console.log("dragCounter:", n, m.target), r.value = true);
      }, i = (m) => {
        m.preventDefault(), m.stopPropagation(), m.dataTransfer && (m.dataTransfer.dropEffect = "copy");
      }, l = (m) => {
        m.preventDefault(), m.stopPropagation(), n--, n <= 0 && (n = 0, r.value = false);
      }, a = async (m) => {
        var _a, _b, _c;
        if (m.preventDefault(), m.stopPropagation(), n = 0, r.value = false, !((_a = m.dataTransfer) == null ? void 0 : _a.items)) return;
        let S = [], p = ((_b = m.dataTransfer) == null ? void 0 : _b.files) || [];
        for (let x of (_c = m.dataTransfer) == null ? void 0 : _c.items) if (x.kind === "file") {
          let y = x.webkitGetAsEntry();
          y && S.push(y);
        }
        S.length > 0 && s("drop", {
          files: p,
          entries: S
        });
      }, u = (m) => {
        m.preventDefault(), m.stopPropagation();
      }, c = () => {
        [
          "dragenter",
          "dragover",
          "dragleave",
          "drop"
        ].forEach((S) => {
          document.addEventListener(S, u, false);
        });
      }, d = () => {
        [
          "dragenter",
          "dragover",
          "dragleave",
          "drop"
        ].forEach((S) => {
          document.removeEventListener(S, u, false);
        });
      };
      return Xt(() => {
        c();
      }), Yt(() => {
        d();
      }), (m, S) => ($(), L("div", {
        class: he([
          "file-view",
          {
            "drag-over": r.value
          }
        ]),
        onDragenter: ye(o, [
          "prevent"
        ]),
        onDragover: ye(i, [
          "prevent"
        ]),
        onDragleave: ye(l, [
          "prevent"
        ]),
        onDrop: ye(a, [
          "prevent"
        ])
      }, [
        r.value ? ($(), L("div", tr, [
          ...S[0] || (S[0] = [
            F("div", {
              class: "drag-content"
            }, [
              F("div", {
                class: "drag-icon"
              }, "\u{1F4C1}"),
              F("div", {
                class: "drag-text"
              }, "\u62D6\u653E\u6587\u4EF6\u5230\u6B64\u5904\u4E0A\u4F20"),
              F("div", {
                class: "drag-subtext"
              }, "\u652F\u6301\u591A\u6587\u4EF6\u540C\u65F6\u4E0A\u4F20")
            ], -1)
          ])
        ])) : G("", true),
        rt(m.$slots, "default")
      ], 34));
    }
  }), rr = () => {
    ze().$patch({
      account: "",
      token: ""
    }), localStorage.removeItem("user-token"), localStorage.removeItem("user-account");
  }, Rt = {
    logout: rr
  }, sr = {
    class: "bg-white border border-gray-300 rounded px-4 py-2 min-w-100 min-h-32 max-h-[80vh] flex flex-col"
  }, or = {
    class: "mb-2 h-8 items-center flex w-full justify-between"
  }, ir = {
    class: "overflow-y-auto flex-1"
  }, ar = pe({
    __name: "Dialog",
    props: {
      close: {
        type: Function
      },
      data: {},
      zIndex: {}
    },
    setup(e) {
      return (t, n) => ($(), ct(Zt, {
        to: "body"
      }, [
        ue(wt, {
          "enter-active-class": "animate__animated animate__fadeIn duration-500",
          appear: ""
        }, {
          default: ge(() => [
            F("div", {
              class: he([
                {
                  zIndex: e.zIndex ?? 1e3
                },
                "bg-[rgba(0,0,0,.35)] w-full h-full flex items-center justify-center fixed top-0 left-0 right-0 bottom-0"
              ])
            }, [
              ue(wt, {
                "enter-active-class": "animate__animated animate__slideInUp duration-250",
                appear: ""
              }, {
                default: ge(() => [
                  F("div", sr, [
                    F("div", or, [
                      F("div", null, [
                        rt(t.$slots, "title")
                      ]),
                      F("div", {
                        onClick: n[0] || (n[0] = (...r) => e.close && e.close(...r)),
                        class: "rounded-[50%] hover:bg-neutral-300 transition-all duration-350"
                      }, [
                        ...n[1] || (n[1] = [
                          F("svg", {
                            t: "1761792212856",
                            class: "icon",
                            viewBox: "0 0 1024 1024",
                            version: "1.1",
                            xmlns: "http://www.w3.org/2000/svg",
                            "p-id": "4743",
                            width: "24",
                            height: "24"
                          }, [
                            F("path", {
                              d: "M504.224 470.288l207.84-207.84a16 16 0 0 1 22.608 0l11.328 11.328a16 16 0 0 1 0 22.624l-207.84 207.824 207.84 207.84a16 16 0 0 1 0 22.608l-11.328 11.328a16 16 0 0 1-22.624 0l-207.824-207.84-207.84 207.84a16 16 0 0 1-22.608 0l-11.328-11.328a16 16 0 0 1 0-22.624l207.84-207.824-207.84-207.84a16 16 0 0 1 0-22.608l11.328-11.328a16 16 0 0 1 22.624 0l207.824 207.84z",
                              fill: "currentColor",
                              "p-id": "4744"
                            })
                          ], -1)
                        ])
                      ])
                    ]),
                    F("div", ir, [
                      rt(t.$slots, "default", {
                        data: e.data
                      })
                    ])
                  ])
                ]),
                _: 3
              })
            ], 2)
          ]),
          _: 3
        })
      ]));
    }
  });
  let Xe = [], lr = 1e3;
  const en = (e, t, n, r) => {
    let s = document.createElement("div");
    document.body.append(s), typeof e == "string" && (e = Oe("div", e)), typeof t == "string" && (t = Oe("div", t));
    const o = Qt({
      render: () => Oe(ar, {
        data: n,
        close: () => i.close(i),
        ...r,
        zIndex: lr++
      }, {
        title: (l) => typeof e == "function" ? e(l) : e,
        default: (l) => typeof t == "function" ? t(l) : t
      })
    });
    o.mount(s);
    const i = {
      title: e,
      content: t,
      app: o,
      container: s,
      close: (l) => {
        var _a;
        (_a = l.onClose) == null ? void 0 : _a.call(l, l), l.app.unmount(), l.container.remove(), Xe = Xe.filter((a) => a !== l);
      },
      ...r
    };
    return Xe.push(i), i;
  }, cr = {
    class: "flex w-full items-center"
  }, ur = {
    key: 0,
    class: "h-lh text-sm mr-1 font-[Helvetica,Tahoma,Arial]"
  }, fr = {
    class: "flex-1 rounded w-full h-2 bg-gray-200"
  }, dr = pe({
    __name: "Progress",
    props: {
      value: {},
      showProgressText: {
        type: Boolean,
        default: true
      }
    },
    setup(e) {
      return (t, n) => ($(), L("div", cr, [
        e.showProgressText ? ($(), L("div", ur, z(e.value.toFixed(0)) + "%", 1)) : G("", true),
        F("div", fr, [
          F("div", {
            class: "relative h-full rounded bg-blue-500 transition-[width] duration-500 ease-in-out",
            style: ut({
              width: `${e.value}%`
            })
          }, null, 4)
        ])
      ]));
    }
  }), tn = Kt("uploadHistory", {
    state: () => ({
      history: []
    }),
    getters: {
      uncompletedItemsPriority: (e) => e.history.sort((t, n) => n.progress < 100 && t.progress >= 100 ? 1 : n.progress >= 100 && t.progress < 100 ? -1 : 0)
    },
    actions: {
      add(e, t, n, r) {
        this.history.push({
          name: e,
          size: t,
          sha256: n,
          progress: r ?? 0
        });
      },
      addMany(e) {
        this.history.push(...e.map((t) => ({
          progress: 0,
          ...t
        })));
      },
      clear() {
        this.history = [];
      },
      remove(e) {
        this.history.splice(e, 1);
      },
      update(e, t) {
        const n = this.history.findIndex((r) => r.sha256 === e);
        console.log("@update", {
          sha256: e,
          progress: t
        }), n !== -1 && (this.history[n] = typeof t == "function" ? t(this.history[n]) : {
          ...this.history[n],
          progress: t
        });
      }
    }
  }), hr = {
    class: "flex flex-col gap-2"
  }, nn = pe({
    __name: "UploadHistory",
    setup(e) {
      const t = tn();
      return (n, r) => ($(), L("div", hr, [
        ue(On, {
          "move-class": "transition-all duration-300"
        }, {
          default: ge(() => [
            ($(true), L(re, null, Me(Y(t).uncompletedItemsPriority, (s) => ($(), L("div", {
              key: s.sha256,
              class: "flex flex-col"
            }, [
              F("div", null, z(s.name), 1),
              ue(dr, {
                value: s.progress
              }, null, 8, [
                "value"
              ])
            ]))), 128))
          ]),
          _: 1
        })
      ]));
    }
  }), pr = {
    class: "navbar"
  }, mr = {
    class: "navbar-menu"
  }, yr = pe({
    __name: "Header",
    setup(e) {
      const t = ze();
      function n() {
        en("\u4E0A\u4F20\u8BB0\u5F55", Oe(nn));
      }
      return (r, s) => {
        const o = Tn("router-link");
        return $(), L("header", pr, [
          ue(o, {
            to: "/",
            class: "navbar-brand"
          }, {
            default: ge(() => [
              ...s[1] || (s[1] = [
                Ce("MyFolder", -1)
              ])
            ]),
            _: 1
          }),
          F("ul", mr, [
            Y(t).isLogin ? ($(), L(re, {
              key: 0
            }, [
              F("li", null, [
                F("a", {
                  onClick: s[0] || (s[0] = (...i) => Y(Rt).logout && Y(Rt).logout(...i))
                }, "\u767B\u51FA")
              ]),
              F("li", null, [
                F("a", {
                  onClick: n
                }, "\u4E0A\u4F20\u8BB0\u5F55")
              ])
            ], 64)) : ($(), L(re, {
              key: 1
            }, [
              F("li", null, [
                ue(o, {
                  to: "/login"
                }, {
                  default: ge(() => [
                    ...s[2] || (s[2] = [
                      Ce("\u767B\u5F55", -1)
                    ])
                  ]),
                  _: 1
                })
              ]),
              F("li", null, [
                ue(o, {
                  to: "/register"
                }, {
                  default: ge(() => [
                    ...s[3] || (s[3] = [
                      Ce("\u6CE8\u518C", -1)
                    ])
                  ]),
                  _: 1
                })
              ])
            ], 64))
          ])
        ]);
      };
    }
  }), br = {}, gr = {
    class: "footer"
  };
  function xr(e, t) {
    return $(), L("footer", gr, [
      ...t[0] || (t[0] = [
        F("div", {
          class: "footer-links"
        }, [
          F("a", {
            href: "#"
          }, "\u5E2E\u52A9"),
          F("a", {
            href: "#"
          }, "\u5173\u4E8E"),
          F("a", {
            href: "#"
          }, "\u8054\u7CFB\u6211\u4EEC")
        ], -1),
        F("p", null, "\xA9 2024 MyFolder. All rights reserved.", -1)
      ])
    ]);
  }
  const wr = dt(br, [
    [
      "render",
      xr
    ]
  ]);
  var vr = typeof globalThis < "u" ? globalThis : typeof window < "u" ? window : typeof global < "u" ? global : typeof self < "u" ? self : {};
  function _r(e) {
    return e && e.__esModule && Object.prototype.hasOwnProperty.call(e, "default") ? e.default : e;
  }
  function Er(e) {
    if (Object.prototype.hasOwnProperty.call(e, "__esModule")) return e;
    var t = e.default;
    if (typeof t == "function") {
      var n = function r() {
        return this instanceof r ? Reflect.construct(t, arguments, this.constructor) : t.apply(this, arguments);
      };
      n.prototype = t.prototype;
    } else n = {};
    return Object.defineProperty(n, "__esModule", {
      value: true
    }), Object.keys(e).forEach(function(r) {
      var s = Object.getOwnPropertyDescriptor(e, r);
      Object.defineProperty(n, r, s.get ? s : {
        enumerable: true,
        get: function() {
          return e[r];
        }
      });
    }), n;
  }
  var Ye, At;
  function Sr() {
    if (At) return Ye;
    At = 1;
    function e(s) {
      if (typeof s != "string") throw new TypeError("Path must be a string. Received " + JSON.stringify(s));
    }
    function t(s, o) {
      for (var i = "", l = 0, a = -1, u = 0, c, d = 0; d <= s.length; ++d) {
        if (d < s.length) c = s.charCodeAt(d);
        else {
          if (c === 47) break;
          c = 47;
        }
        if (c === 47) {
          if (!(a === d - 1 || u === 1)) if (a !== d - 1 && u === 2) {
            if (i.length < 2 || l !== 2 || i.charCodeAt(i.length - 1) !== 46 || i.charCodeAt(i.length - 2) !== 46) {
              if (i.length > 2) {
                var m = i.lastIndexOf("/");
                if (m !== i.length - 1) {
                  m === -1 ? (i = "", l = 0) : (i = i.slice(0, m), l = i.length - 1 - i.lastIndexOf("/")), a = d, u = 0;
                  continue;
                }
              } else if (i.length === 2 || i.length === 1) {
                i = "", l = 0, a = d, u = 0;
                continue;
              }
            }
            o && (i.length > 0 ? i += "/.." : i = "..", l = 2);
          } else i.length > 0 ? i += "/" + s.slice(a + 1, d) : i = s.slice(a + 1, d), l = d - a - 1;
          a = d, u = 0;
        } else c === 46 && u !== -1 ? ++u : u = -1;
      }
      return i;
    }
    function n(s, o) {
      var i = o.dir || o.root, l = o.base || (o.name || "") + (o.ext || "");
      return i ? i === o.root ? i + l : i + s + l : l;
    }
    var r = {
      resolve: function() {
        for (var o = "", i = false, l, a = arguments.length - 1; a >= -1 && !i; a--) {
          var u;
          a >= 0 ? u = arguments[a] : (l === void 0 && (l = process.cwd()), u = l), e(u), u.length !== 0 && (o = u + "/" + o, i = u.charCodeAt(0) === 47);
        }
        return o = t(o, !i), i ? o.length > 0 ? "/" + o : "/" : o.length > 0 ? o : ".";
      },
      normalize: function(o) {
        if (e(o), o.length === 0) return ".";
        var i = o.charCodeAt(0) === 47, l = o.charCodeAt(o.length - 1) === 47;
        return o = t(o, !i), o.length === 0 && !i && (o = "."), o.length > 0 && l && (o += "/"), i ? "/" + o : o;
      },
      isAbsolute: function(o) {
        return e(o), o.length > 0 && o.charCodeAt(0) === 47;
      },
      join: function() {
        if (arguments.length === 0) return ".";
        for (var o, i = 0; i < arguments.length; ++i) {
          var l = arguments[i];
          e(l), l.length > 0 && (o === void 0 ? o = l : o += "/" + l);
        }
        return o === void 0 ? "." : r.normalize(o);
      },
      relative: function(o, i) {
        if (e(o), e(i), o === i || (o = r.resolve(o), i = r.resolve(i), o === i)) return "";
        for (var l = 1; l < o.length && o.charCodeAt(l) === 47; ++l) ;
        for (var a = o.length, u = a - l, c = 1; c < i.length && i.charCodeAt(c) === 47; ++c) ;
        for (var d = i.length, m = d - c, S = u < m ? u : m, p = -1, x = 0; x <= S; ++x) {
          if (x === S) {
            if (m > S) {
              if (i.charCodeAt(c + x) === 47) return i.slice(c + x + 1);
              if (x === 0) return i.slice(c + x);
            } else u > S && (o.charCodeAt(l + x) === 47 ? p = x : x === 0 && (p = 0));
            break;
          }
          var y = o.charCodeAt(l + x), C = i.charCodeAt(c + x);
          if (y !== C) break;
          y === 47 && (p = x);
        }
        var U = "";
        for (x = l + p + 1; x <= a; ++x) (x === a || o.charCodeAt(x) === 47) && (U.length === 0 ? U += ".." : U += "/..");
        return U.length > 0 ? U + i.slice(c + p) : (c += p, i.charCodeAt(c) === 47 && ++c, i.slice(c));
      },
      _makeLong: function(o) {
        return o;
      },
      dirname: function(o) {
        if (e(o), o.length === 0) return ".";
        for (var i = o.charCodeAt(0), l = i === 47, a = -1, u = true, c = o.length - 1; c >= 1; --c) if (i = o.charCodeAt(c), i === 47) {
          if (!u) {
            a = c;
            break;
          }
        } else u = false;
        return a === -1 ? l ? "/" : "." : l && a === 1 ? "//" : o.slice(0, a);
      },
      basename: function(o, i) {
        if (i !== void 0 && typeof i != "string") throw new TypeError('"ext" argument must be a string');
        e(o);
        var l = 0, a = -1, u = true, c;
        if (i !== void 0 && i.length > 0 && i.length <= o.length) {
          if (i.length === o.length && i === o) return "";
          var d = i.length - 1, m = -1;
          for (c = o.length - 1; c >= 0; --c) {
            var S = o.charCodeAt(c);
            if (S === 47) {
              if (!u) {
                l = c + 1;
                break;
              }
            } else m === -1 && (u = false, m = c + 1), d >= 0 && (S === i.charCodeAt(d) ? --d === -1 && (a = c) : (d = -1, a = m));
          }
          return l === a ? a = m : a === -1 && (a = o.length), o.slice(l, a);
        } else {
          for (c = o.length - 1; c >= 0; --c) if (o.charCodeAt(c) === 47) {
            if (!u) {
              l = c + 1;
              break;
            }
          } else a === -1 && (u = false, a = c + 1);
          return a === -1 ? "" : o.slice(l, a);
        }
      },
      extname: function(o) {
        e(o);
        for (var i = -1, l = 0, a = -1, u = true, c = 0, d = o.length - 1; d >= 0; --d) {
          var m = o.charCodeAt(d);
          if (m === 47) {
            if (!u) {
              l = d + 1;
              break;
            }
            continue;
          }
          a === -1 && (u = false, a = d + 1), m === 46 ? i === -1 ? i = d : c !== 1 && (c = 1) : i !== -1 && (c = -1);
        }
        return i === -1 || a === -1 || c === 0 || c === 1 && i === a - 1 && i === l + 1 ? "" : o.slice(i, a);
      },
      format: function(o) {
        if (o === null || typeof o != "object") throw new TypeError('The "pathObject" argument must be of type Object. Received type ' + typeof o);
        return n("/", o);
      },
      parse: function(o) {
        e(o);
        var i = {
          root: "",
          dir: "",
          base: "",
          ext: "",
          name: ""
        };
        if (o.length === 0) return i;
        var l = o.charCodeAt(0), a = l === 47, u;
        a ? (i.root = "/", u = 1) : u = 0;
        for (var c = -1, d = 0, m = -1, S = true, p = o.length - 1, x = 0; p >= u; --p) {
          if (l = o.charCodeAt(p), l === 47) {
            if (!S) {
              d = p + 1;
              break;
            }
            continue;
          }
          m === -1 && (S = false, m = p + 1), l === 46 ? c === -1 ? c = p : x !== 1 && (x = 1) : c !== -1 && (x = -1);
        }
        return c === -1 || m === -1 || x === 0 || x === 1 && c === m - 1 && c === d + 1 ? m !== -1 && (d === 0 && a ? i.base = i.name = o.slice(1, m) : i.base = i.name = o.slice(d, m)) : (d === 0 && a ? (i.name = o.slice(1, c), i.base = o.slice(1, m)) : (i.name = o.slice(d, c), i.base = o.slice(d, m)), i.ext = o.slice(c, m)), d > 0 ? i.dir = o.slice(0, d - 1) : a && (i.dir = "/"), i;
      },
      sep: "/",
      delimiter: ":",
      win32: null,
      posix: null
    };
    return r.posix = r, Ye = r, Ye;
  }
  var Fr = Sr();
  const Ct = _r(Fr);
  function rn(e, t) {
    return function() {
      return e.apply(t, arguments);
    };
  }
  const { toString: Rr } = Object.prototype, { getPrototypeOf: ht } = Object, { iterator: He, toStringTag: sn } = Symbol, qe = /* @__PURE__ */ ((e) => (t) => {
    const n = Rr.call(t);
    return e[n] || (e[n] = n.slice(8, -1).toLowerCase());
  })(/* @__PURE__ */ Object.create(null)), se = (e) => (e = e.toLowerCase(), (t) => qe(t) === e), Je = (e) => (t) => typeof t === e, { isArray: Se } = Array, Ee = Je("undefined");
  function Te(e) {
    return e !== null && !Ee(e) && e.constructor !== null && !Ee(e.constructor) && Q(e.constructor.isBuffer) && e.constructor.isBuffer(e);
  }
  const on = se("ArrayBuffer");
  function Ar(e) {
    let t;
    return typeof ArrayBuffer < "u" && ArrayBuffer.isView ? t = ArrayBuffer.isView(e) : t = e && e.buffer && on(e.buffer), t;
  }
  const Cr = Je("string"), Q = Je("function"), an = Je("number"), ke = (e) => e !== null && typeof e == "object", Or = (e) => e === true || e === false, Ue = (e) => {
    if (qe(e) !== "object") return false;
    const t = ht(e);
    return (t === null || t === Object.prototype || Object.getPrototypeOf(t) === null) && !(sn in e) && !(He in e);
  }, Tr = (e) => {
    if (!ke(e) || Te(e)) return false;
    try {
      return Object.keys(e).length === 0 && Object.getPrototypeOf(e) === Object.prototype;
    } catch {
      return false;
    }
  }, kr = se("Date"), Pr = se("File"), Nr = se("Blob"), Br = se("FileList"), Dr = (e) => ke(e) && Q(e.pipe), $r = (e) => {
    let t;
    return e && (typeof FormData == "function" && e instanceof FormData || Q(e.append) && ((t = qe(e)) === "formdata" || t === "object" && Q(e.toString) && e.toString() === "[object FormData]"));
  }, Ur = se("URLSearchParams"), [Ir, Lr, jr, Mr] = [
    "ReadableStream",
    "Request",
    "Response",
    "Headers"
  ].map(se), zr = (e) => e.trim ? e.trim() : e.replace(/^[\s\uFEFF\xA0]+|[\s\uFEFF\xA0]+$/g, "");
  function Pe(e, t, { allOwnKeys: n = false } = {}) {
    if (e === null || typeof e > "u") return;
    let r, s;
    if (typeof e != "object" && (e = [
      e
    ]), Se(e)) for (r = 0, s = e.length; r < s; r++) t.call(null, e[r], r, e);
    else {
      if (Te(e)) return;
      const o = n ? Object.getOwnPropertyNames(e) : Object.keys(e), i = o.length;
      let l;
      for (r = 0; r < i; r++) l = o[r], t.call(null, e[l], l, e);
    }
  }
  function ln(e, t) {
    if (Te(e)) return null;
    t = t.toLowerCase();
    const n = Object.keys(e);
    let r = n.length, s;
    for (; r-- > 0; ) if (s = n[r], t === s.toLowerCase()) return s;
    return null;
  }
  const be = typeof globalThis < "u" ? globalThis : typeof self < "u" ? self : typeof window < "u" ? window : global, cn = (e) => !Ee(e) && e !== be;
  function ot() {
    const { caseless: e, skipUndefined: t } = cn(this) && this || {}, n = {}, r = (s, o) => {
      if (o === "__proto__" || o === "constructor" || o === "prototype") return;
      const i = e && ln(n, o) || o;
      Ue(n[i]) && Ue(s) ? n[i] = ot(n[i], s) : Ue(s) ? n[i] = ot({}, s) : Se(s) ? n[i] = s.slice() : (!t || !Ee(s)) && (n[i] = s);
    };
    for (let s = 0, o = arguments.length; s < o; s++) arguments[s] && Pe(arguments[s], r);
    return n;
  }
  const Hr = (e, t, n, { allOwnKeys: r } = {}) => (Pe(t, (s, o) => {
    n && Q(s) ? Object.defineProperty(e, o, {
      value: rn(s, n),
      writable: true,
      enumerable: true,
      configurable: true
    }) : Object.defineProperty(e, o, {
      value: s,
      writable: true,
      enumerable: true,
      configurable: true
    });
  }, {
    allOwnKeys: r
  }), e), qr = (e) => (e.charCodeAt(0) === 65279 && (e = e.slice(1)), e), Jr = (e, t, n, r) => {
    e.prototype = Object.create(t.prototype, r), Object.defineProperty(e.prototype, "constructor", {
      value: e,
      writable: true,
      enumerable: false,
      configurable: true
    }), Object.defineProperty(e, "super", {
      value: t.prototype
    }), n && Object.assign(e.prototype, n);
  }, Vr = (e, t, n, r) => {
    let s, o, i;
    const l = {};
    if (t = t || {}, e == null) return t;
    do {
      for (s = Object.getOwnPropertyNames(e), o = s.length; o-- > 0; ) i = s[o], (!r || r(i, e, t)) && !l[i] && (t[i] = e[i], l[i] = true);
      e = n !== false && ht(e);
    } while (e && (!n || n(e, t)) && e !== Object.prototype);
    return t;
  }, Wr = (e, t, n) => {
    e = String(e), (n === void 0 || n > e.length) && (n = e.length), n -= t.length;
    const r = e.indexOf(t, n);
    return r !== -1 && r === n;
  }, Kr = (e) => {
    if (!e) return null;
    if (Se(e)) return e;
    let t = e.length;
    if (!an(t)) return null;
    const n = new Array(t);
    for (; t-- > 0; ) n[t] = e[t];
    return n;
  }, Gr = /* @__PURE__ */ ((e) => (t) => e && t instanceof e)(typeof Uint8Array < "u" && ht(Uint8Array)), Xr = (e, t) => {
    const r = (e && e[He]).call(e);
    let s;
    for (; (s = r.next()) && !s.done; ) {
      const o = s.value;
      t.call(e, o[0], o[1]);
    }
  }, Yr = (e, t) => {
    let n;
    const r = [];
    for (; (n = e.exec(t)) !== null; ) r.push(n);
    return r;
  }, Zr = se("HTMLFormElement"), Qr = (e) => e.toLowerCase().replace(/[-_\s]([a-z\d])(\w*)/g, function(n, r, s) {
    return r.toUpperCase() + s;
  }), Ot = (({ hasOwnProperty: e }) => (t, n) => e.call(t, n))(Object.prototype), es = se("RegExp"), un = (e, t) => {
    const n = Object.getOwnPropertyDescriptors(e), r = {};
    Pe(n, (s, o) => {
      let i;
      (i = t(s, o, e)) !== false && (r[o] = i || s);
    }), Object.defineProperties(e, r);
  }, ts = (e) => {
    un(e, (t, n) => {
      if (Q(e) && [
        "arguments",
        "caller",
        "callee"
      ].indexOf(n) !== -1) return false;
      const r = e[n];
      if (Q(r)) {
        if (t.enumerable = false, "writable" in t) {
          t.writable = false;
          return;
        }
        t.set || (t.set = () => {
          throw Error("Can not rewrite read-only method '" + n + "'");
        });
      }
    });
  }, ns = (e, t) => {
    const n = {}, r = (s) => {
      s.forEach((o) => {
        n[o] = true;
      });
    };
    return Se(e) ? r(e) : r(String(e).split(t)), n;
  }, rs = () => {
  }, ss = (e, t) => e != null && Number.isFinite(e = +e) ? e : t;
  function os(e) {
    return !!(e && Q(e.append) && e[sn] === "FormData" && e[He]);
  }
  const is = (e) => {
    const t = new Array(10), n = (r, s) => {
      if (ke(r)) {
        if (t.indexOf(r) >= 0) return;
        if (Te(r)) return r;
        if (!("toJSON" in r)) {
          t[s] = r;
          const o = Se(r) ? [] : {};
          return Pe(r, (i, l) => {
            const a = n(i, s + 1);
            !Ee(a) && (o[l] = a);
          }), t[s] = void 0, o;
        }
      }
      return r;
    };
    return n(e, 0);
  }, as = se("AsyncFunction"), ls = (e) => e && (ke(e) || Q(e)) && Q(e.then) && Q(e.catch), fn = ((e, t) => e ? setImmediate : t ? ((n, r) => (be.addEventListener("message", ({ source: s, data: o }) => {
    s === be && o === n && r.length && r.shift()();
  }, false), (s) => {
    r.push(s), be.postMessage(n, "*");
  }))(`axios@${Math.random()}`, []) : (n) => setTimeout(n))(typeof setImmediate == "function", Q(be.postMessage)), cs = typeof queueMicrotask < "u" ? queueMicrotask.bind(be) : typeof process < "u" && process.nextTick || fn, us = (e) => e != null && Q(e[He]), f = {
    isArray: Se,
    isArrayBuffer: on,
    isBuffer: Te,
    isFormData: $r,
    isArrayBufferView: Ar,
    isString: Cr,
    isNumber: an,
    isBoolean: Or,
    isObject: ke,
    isPlainObject: Ue,
    isEmptyObject: Tr,
    isReadableStream: Ir,
    isRequest: Lr,
    isResponse: jr,
    isHeaders: Mr,
    isUndefined: Ee,
    isDate: kr,
    isFile: Pr,
    isBlob: Nr,
    isRegExp: es,
    isFunction: Q,
    isStream: Dr,
    isURLSearchParams: Ur,
    isTypedArray: Gr,
    isFileList: Br,
    forEach: Pe,
    merge: ot,
    extend: Hr,
    trim: zr,
    stripBOM: qr,
    inherits: Jr,
    toFlatObject: Vr,
    kindOf: qe,
    kindOfTest: se,
    endsWith: Wr,
    toArray: Kr,
    forEachEntry: Xr,
    matchAll: Yr,
    isHTMLForm: Zr,
    hasOwnProperty: Ot,
    hasOwnProp: Ot,
    reduceDescriptors: un,
    freezeMethods: ts,
    toObjectSet: ns,
    toCamelCase: Qr,
    noop: rs,
    toFiniteNumber: ss,
    findKey: ln,
    global: be,
    isContextDefined: cn,
    isSpecCompliantForm: os,
    toJSONObject: is,
    isAsyncFn: as,
    isThenable: ls,
    setImmediate: fn,
    asap: cs,
    isIterable: us
  };
  let k = class dn extends Error {
    static from(t, n, r, s, o, i) {
      const l = new dn(t.message, n || t.code, r, s, o);
      return l.cause = t, l.name = t.name, i && Object.assign(l, i), l;
    }
    constructor(t, n, r, s, o) {
      super(t), this.name = "AxiosError", this.isAxiosError = true, n && (this.code = n), r && (this.config = r), s && (this.request = s), o && (this.response = o, this.status = o.status);
    }
    toJSON() {
      return {
        message: this.message,
        name: this.name,
        description: this.description,
        number: this.number,
        fileName: this.fileName,
        lineNumber: this.lineNumber,
        columnNumber: this.columnNumber,
        stack: this.stack,
        config: f.toJSONObject(this.config),
        code: this.code,
        status: this.status
      };
    }
  };
  k.ERR_BAD_OPTION_VALUE = "ERR_BAD_OPTION_VALUE";
  k.ERR_BAD_OPTION = "ERR_BAD_OPTION";
  k.ECONNABORTED = "ECONNABORTED";
  k.ETIMEDOUT = "ETIMEDOUT";
  k.ERR_NETWORK = "ERR_NETWORK";
  k.ERR_FR_TOO_MANY_REDIRECTS = "ERR_FR_TOO_MANY_REDIRECTS";
  k.ERR_DEPRECATED = "ERR_DEPRECATED";
  k.ERR_BAD_RESPONSE = "ERR_BAD_RESPONSE";
  k.ERR_BAD_REQUEST = "ERR_BAD_REQUEST";
  k.ERR_CANCELED = "ERR_CANCELED";
  k.ERR_NOT_SUPPORT = "ERR_NOT_SUPPORT";
  k.ERR_INVALID_URL = "ERR_INVALID_URL";
  const fs = null;
  function it(e) {
    return f.isPlainObject(e) || f.isArray(e);
  }
  function hn(e) {
    return f.endsWith(e, "[]") ? e.slice(0, -2) : e;
  }
  function Tt(e, t, n) {
    return e ? e.concat(t).map(function(s, o) {
      return s = hn(s), !n && o ? "[" + s + "]" : s;
    }).join(n ? "." : "") : t;
  }
  function ds(e) {
    return f.isArray(e) && !e.some(it);
  }
  const hs = f.toFlatObject(f, {}, null, function(t) {
    return /^is[A-Z]/.test(t);
  });
  function Ve(e, t, n) {
    if (!f.isObject(e)) throw new TypeError("target must be an object");
    t = t || new FormData(), n = f.toFlatObject(n, {
      metaTokens: true,
      dots: false,
      indexes: false
    }, false, function(x, y) {
      return !f.isUndefined(y[x]);
    });
    const r = n.metaTokens, s = n.visitor || c, o = n.dots, i = n.indexes, a = (n.Blob || typeof Blob < "u" && Blob) && f.isSpecCompliantForm(t);
    if (!f.isFunction(s)) throw new TypeError("visitor must be a function");
    function u(p) {
      if (p === null) return "";
      if (f.isDate(p)) return p.toISOString();
      if (f.isBoolean(p)) return p.toString();
      if (!a && f.isBlob(p)) throw new k("Blob is not supported. Use a Buffer instead.");
      return f.isArrayBuffer(p) || f.isTypedArray(p) ? a && typeof Blob == "function" ? new Blob([
        p
      ]) : Buffer.from(p) : p;
    }
    function c(p, x, y) {
      let C = p;
      if (p && !y && typeof p == "object") {
        if (f.endsWith(x, "{}")) x = r ? x : x.slice(0, -2), p = JSON.stringify(p);
        else if (f.isArray(p) && ds(p) || (f.isFileList(p) || f.endsWith(x, "[]")) && (C = f.toArray(p))) return x = hn(x), C.forEach(function(E, N) {
          !(f.isUndefined(E) || E === null) && t.append(i === true ? Tt([
            x
          ], N, o) : i === null ? x : x + "[]", u(E));
        }), false;
      }
      return it(p) ? true : (t.append(Tt(y, x, o), u(p)), false);
    }
    const d = [], m = Object.assign(hs, {
      defaultVisitor: c,
      convertValue: u,
      isVisitable: it
    });
    function S(p, x) {
      if (!f.isUndefined(p)) {
        if (d.indexOf(p) !== -1) throw Error("Circular reference detected in " + x.join("."));
        d.push(p), f.forEach(p, function(C, U) {
          (!(f.isUndefined(C) || C === null) && s.call(t, C, f.isString(U) ? U.trim() : U, x, m)) === true && S(C, x ? x.concat(U) : [
            U
          ]);
        }), d.pop();
      }
    }
    if (!f.isObject(e)) throw new TypeError("data must be an object");
    return S(e), t;
  }
  function kt(e) {
    const t = {
      "!": "%21",
      "'": "%27",
      "(": "%28",
      ")": "%29",
      "~": "%7E",
      "%20": "+",
      "%00": "\0"
    };
    return encodeURIComponent(e).replace(/[!'()~]|%20|%00/g, function(r) {
      return t[r];
    });
  }
  function pt(e, t) {
    this._pairs = [], e && Ve(e, this, t);
  }
  const pn = pt.prototype;
  pn.append = function(t, n) {
    this._pairs.push([
      t,
      n
    ]);
  };
  pn.toString = function(t) {
    const n = t ? function(r) {
      return t.call(this, r, kt);
    } : kt;
    return this._pairs.map(function(s) {
      return n(s[0]) + "=" + n(s[1]);
    }, "").join("&");
  };
  function ps(e) {
    return encodeURIComponent(e).replace(/%3A/gi, ":").replace(/%24/g, "$").replace(/%2C/gi, ",").replace(/%20/g, "+");
  }
  function mn(e, t, n) {
    if (!t) return e;
    const r = n && n.encode || ps, s = f.isFunction(n) ? {
      serialize: n
    } : n, o = s && s.serialize;
    let i;
    if (o ? i = o(t, s) : i = f.isURLSearchParams(t) ? t.toString() : new pt(t, s).toString(r), i) {
      const l = e.indexOf("#");
      l !== -1 && (e = e.slice(0, l)), e += (e.indexOf("?") === -1 ? "?" : "&") + i;
    }
    return e;
  }
  class Pt {
    constructor() {
      this.handlers = [];
    }
    use(t, n, r) {
      return this.handlers.push({
        fulfilled: t,
        rejected: n,
        synchronous: r ? r.synchronous : false,
        runWhen: r ? r.runWhen : null
      }), this.handlers.length - 1;
    }
    eject(t) {
      this.handlers[t] && (this.handlers[t] = null);
    }
    clear() {
      this.handlers && (this.handlers = []);
    }
    forEach(t) {
      f.forEach(this.handlers, function(r) {
        r !== null && t(r);
      });
    }
  }
  const mt = {
    silentJSONParsing: true,
    forcedJSONParsing: true,
    clarifyTimeoutError: false,
    legacyInterceptorReqResOrdering: true
  }, ms = typeof URLSearchParams < "u" ? URLSearchParams : pt, ys = typeof FormData < "u" ? FormData : null, bs = typeof Blob < "u" ? Blob : null, gs = {
    isBrowser: true,
    classes: {
      URLSearchParams: ms,
      FormData: ys,
      Blob: bs
    },
    protocols: [
      "http",
      "https",
      "file",
      "blob",
      "url",
      "data"
    ]
  }, yt = typeof window < "u" && typeof document < "u", at = typeof navigator == "object" && navigator || void 0, xs = yt && (!at || [
    "ReactNative",
    "NativeScript",
    "NS"
  ].indexOf(at.product) < 0), ws = typeof WorkerGlobalScope < "u" && self instanceof WorkerGlobalScope && typeof self.importScripts == "function", vs = yt && window.location.href || "http://localhost", _s = Object.freeze(Object.defineProperty({
    __proto__: null,
    hasBrowserEnv: yt,
    hasStandardBrowserEnv: xs,
    hasStandardBrowserWebWorkerEnv: ws,
    navigator: at,
    origin: vs
  }, Symbol.toStringTag, {
    value: "Module"
  })), X = {
    ..._s,
    ...gs
  };
  function Es(e, t) {
    return Ve(e, new X.classes.URLSearchParams(), {
      visitor: function(n, r, s, o) {
        return X.isNode && f.isBuffer(n) ? (this.append(r, n.toString("base64")), false) : o.defaultVisitor.apply(this, arguments);
      },
      ...t
    });
  }
  function Ss(e) {
    return f.matchAll(/\w+|\[(\w*)]/g, e).map((t) => t[0] === "[]" ? "" : t[1] || t[0]);
  }
  function Fs(e) {
    const t = {}, n = Object.keys(e);
    let r;
    const s = n.length;
    let o;
    for (r = 0; r < s; r++) o = n[r], t[o] = e[o];
    return t;
  }
  function yn(e) {
    function t(n, r, s, o) {
      let i = n[o++];
      if (i === "__proto__") return true;
      const l = Number.isFinite(+i), a = o >= n.length;
      return i = !i && f.isArray(s) ? s.length : i, a ? (f.hasOwnProp(s, i) ? s[i] = [
        s[i],
        r
      ] : s[i] = r, !l) : ((!s[i] || !f.isObject(s[i])) && (s[i] = []), t(n, r, s[i], o) && f.isArray(s[i]) && (s[i] = Fs(s[i])), !l);
    }
    if (f.isFormData(e) && f.isFunction(e.entries)) {
      const n = {};
      return f.forEachEntry(e, (r, s) => {
        t(Ss(r), s, n, 0);
      }), n;
    }
    return null;
  }
  function Rs(e, t, n) {
    if (f.isString(e)) try {
      return (t || JSON.parse)(e), f.trim(e);
    } catch (r) {
      if (r.name !== "SyntaxError") throw r;
    }
    return (n || JSON.stringify)(e);
  }
  const Ne = {
    transitional: mt,
    adapter: [
      "xhr",
      "http",
      "fetch"
    ],
    transformRequest: [
      function(t, n) {
        const r = n.getContentType() || "", s = r.indexOf("application/json") > -1, o = f.isObject(t);
        if (o && f.isHTMLForm(t) && (t = new FormData(t)), f.isFormData(t)) return s ? JSON.stringify(yn(t)) : t;
        if (f.isArrayBuffer(t) || f.isBuffer(t) || f.isStream(t) || f.isFile(t) || f.isBlob(t) || f.isReadableStream(t)) return t;
        if (f.isArrayBufferView(t)) return t.buffer;
        if (f.isURLSearchParams(t)) return n.setContentType("application/x-www-form-urlencoded;charset=utf-8", false), t.toString();
        let l;
        if (o) {
          if (r.indexOf("application/x-www-form-urlencoded") > -1) return Es(t, this.formSerializer).toString();
          if ((l = f.isFileList(t)) || r.indexOf("multipart/form-data") > -1) {
            const a = this.env && this.env.FormData;
            return Ve(l ? {
              "files[]": t
            } : t, a && new a(), this.formSerializer);
          }
        }
        return o || s ? (n.setContentType("application/json", false), Rs(t)) : t;
      }
    ],
    transformResponse: [
      function(t) {
        const n = this.transitional || Ne.transitional, r = n && n.forcedJSONParsing, s = this.responseType === "json";
        if (f.isResponse(t) || f.isReadableStream(t)) return t;
        if (t && f.isString(t) && (r && !this.responseType || s)) {
          const i = !(n && n.silentJSONParsing) && s;
          try {
            return JSON.parse(t, this.parseReviver);
          } catch (l) {
            if (i) throw l.name === "SyntaxError" ? k.from(l, k.ERR_BAD_RESPONSE, this, null, this.response) : l;
          }
        }
        return t;
      }
    ],
    timeout: 0,
    xsrfCookieName: "XSRF-TOKEN",
    xsrfHeaderName: "X-XSRF-TOKEN",
    maxContentLength: -1,
    maxBodyLength: -1,
    env: {
      FormData: X.classes.FormData,
      Blob: X.classes.Blob
    },
    validateStatus: function(t) {
      return t >= 200 && t < 300;
    },
    headers: {
      common: {
        Accept: "application/json, text/plain, */*",
        "Content-Type": void 0
      }
    }
  };
  f.forEach([
    "delete",
    "get",
    "head",
    "post",
    "put",
    "patch"
  ], (e) => {
    Ne.headers[e] = {};
  });
  const As = f.toObjectSet([
    "age",
    "authorization",
    "content-length",
    "content-type",
    "etag",
    "expires",
    "from",
    "host",
    "if-modified-since",
    "if-unmodified-since",
    "last-modified",
    "location",
    "max-forwards",
    "proxy-authorization",
    "referer",
    "retry-after",
    "user-agent"
  ]), Cs = (e) => {
    const t = {};
    let n, r, s;
    return e && e.split(`
`).forEach(function(i) {
      s = i.indexOf(":"), n = i.substring(0, s).trim().toLowerCase(), r = i.substring(s + 1).trim(), !(!n || t[n] && As[n]) && (n === "set-cookie" ? t[n] ? t[n].push(r) : t[n] = [
        r
      ] : t[n] = t[n] ? t[n] + ", " + r : r);
    }), t;
  }, Nt = Symbol("internals");
  function Fe(e) {
    return e && String(e).trim().toLowerCase();
  }
  function Ie(e) {
    return e === false || e == null ? e : f.isArray(e) ? e.map(Ie) : String(e);
  }
  function Os(e) {
    const t = /* @__PURE__ */ Object.create(null), n = /([^\s,;=]+)\s*(?:=\s*([^,;]+))?/g;
    let r;
    for (; r = n.exec(e); ) t[r[1]] = r[2];
    return t;
  }
  const Ts = (e) => /^[-_a-zA-Z0-9^`|~,!#$%&'*+.]+$/.test(e.trim());
  function Ze(e, t, n, r, s) {
    if (f.isFunction(r)) return r.call(this, t, n);
    if (s && (t = n), !!f.isString(t)) {
      if (f.isString(r)) return t.indexOf(r) !== -1;
      if (f.isRegExp(r)) return r.test(t);
    }
  }
  function ks(e) {
    return e.trim().toLowerCase().replace(/([a-z\d])(\w*)/g, (t, n, r) => n.toUpperCase() + r);
  }
  function Ps(e, t) {
    const n = f.toCamelCase(" " + t);
    [
      "get",
      "set",
      "has"
    ].forEach((r) => {
      Object.defineProperty(e, r + n, {
        value: function(s, o, i) {
          return this[r].call(this, t, s, o, i);
        },
        configurable: true
      });
    });
  }
  let ee = class {
    constructor(t) {
      t && this.set(t);
    }
    set(t, n, r) {
      const s = this;
      function o(l, a, u) {
        const c = Fe(a);
        if (!c) throw new Error("header name must be a non-empty string");
        const d = f.findKey(s, c);
        (!d || s[d] === void 0 || u === true || u === void 0 && s[d] !== false) && (s[d || a] = Ie(l));
      }
      const i = (l, a) => f.forEach(l, (u, c) => o(u, c, a));
      if (f.isPlainObject(t) || t instanceof this.constructor) i(t, n);
      else if (f.isString(t) && (t = t.trim()) && !Ts(t)) i(Cs(t), n);
      else if (f.isObject(t) && f.isIterable(t)) {
        let l = {}, a, u;
        for (const c of t) {
          if (!f.isArray(c)) throw TypeError("Object iterator must return a key-value pair");
          l[u = c[0]] = (a = l[u]) ? f.isArray(a) ? [
            ...a,
            c[1]
          ] : [
            a,
            c[1]
          ] : c[1];
        }
        i(l, n);
      } else t != null && o(n, t, r);
      return this;
    }
    get(t, n) {
      if (t = Fe(t), t) {
        const r = f.findKey(this, t);
        if (r) {
          const s = this[r];
          if (!n) return s;
          if (n === true) return Os(s);
          if (f.isFunction(n)) return n.call(this, s, r);
          if (f.isRegExp(n)) return n.exec(s);
          throw new TypeError("parser must be boolean|regexp|function");
        }
      }
    }
    has(t, n) {
      if (t = Fe(t), t) {
        const r = f.findKey(this, t);
        return !!(r && this[r] !== void 0 && (!n || Ze(this, this[r], r, n)));
      }
      return false;
    }
    delete(t, n) {
      const r = this;
      let s = false;
      function o(i) {
        if (i = Fe(i), i) {
          const l = f.findKey(r, i);
          l && (!n || Ze(r, r[l], l, n)) && (delete r[l], s = true);
        }
      }
      return f.isArray(t) ? t.forEach(o) : o(t), s;
    }
    clear(t) {
      const n = Object.keys(this);
      let r = n.length, s = false;
      for (; r--; ) {
        const o = n[r];
        (!t || Ze(this, this[o], o, t, true)) && (delete this[o], s = true);
      }
      return s;
    }
    normalize(t) {
      const n = this, r = {};
      return f.forEach(this, (s, o) => {
        const i = f.findKey(r, o);
        if (i) {
          n[i] = Ie(s), delete n[o];
          return;
        }
        const l = t ? ks(o) : String(o).trim();
        l !== o && delete n[o], n[l] = Ie(s), r[l] = true;
      }), this;
    }
    concat(...t) {
      return this.constructor.concat(this, ...t);
    }
    toJSON(t) {
      const n = /* @__PURE__ */ Object.create(null);
      return f.forEach(this, (r, s) => {
        r != null && r !== false && (n[s] = t && f.isArray(r) ? r.join(", ") : r);
      }), n;
    }
    [Symbol.iterator]() {
      return Object.entries(this.toJSON())[Symbol.iterator]();
    }
    toString() {
      return Object.entries(this.toJSON()).map(([t, n]) => t + ": " + n).join(`
`);
    }
    getSetCookie() {
      return this.get("set-cookie") || [];
    }
    get [Symbol.toStringTag]() {
      return "AxiosHeaders";
    }
    static from(t) {
      return t instanceof this ? t : new this(t);
    }
    static concat(t, ...n) {
      const r = new this(t);
      return n.forEach((s) => r.set(s)), r;
    }
    static accessor(t) {
      const r = (this[Nt] = this[Nt] = {
        accessors: {}
      }).accessors, s = this.prototype;
      function o(i) {
        const l = Fe(i);
        r[l] || (Ps(s, i), r[l] = true);
      }
      return f.isArray(t) ? t.forEach(o) : o(t), this;
    }
  };
  ee.accessor([
    "Content-Type",
    "Content-Length",
    "Accept",
    "Accept-Encoding",
    "User-Agent",
    "Authorization"
  ]);
  f.reduceDescriptors(ee.prototype, ({ value: e }, t) => {
    let n = t[0].toUpperCase() + t.slice(1);
    return {
      get: () => e,
      set(r) {
        this[n] = r;
      }
    };
  });
  f.freezeMethods(ee);
  function Qe(e, t) {
    const n = this || Ne, r = t || n, s = ee.from(r.headers);
    let o = r.data;
    return f.forEach(e, function(l) {
      o = l.call(n, o, s.normalize(), t ? t.status : void 0);
    }), s.normalize(), o;
  }
  function bn(e) {
    return !!(e && e.__CANCEL__);
  }
  let Be = class extends k {
    constructor(t, n, r) {
      super(t ?? "canceled", k.ERR_CANCELED, n, r), this.name = "CanceledError", this.__CANCEL__ = true;
    }
  };
  function gn(e, t, n) {
    const r = n.config.validateStatus;
    !n.status || !r || r(n.status) ? e(n) : t(new k("Request failed with status code " + n.status, [
      k.ERR_BAD_REQUEST,
      k.ERR_BAD_RESPONSE
    ][Math.floor(n.status / 100) - 4], n.config, n.request, n));
  }
  function Ns(e) {
    const t = /^([-+\w]{1,25})(:?\/\/|:)/.exec(e);
    return t && t[1] || "";
  }
  function Bs(e, t) {
    e = e || 10;
    const n = new Array(e), r = new Array(e);
    let s = 0, o = 0, i;
    return t = t !== void 0 ? t : 1e3, function(a) {
      const u = Date.now(), c = r[o];
      i || (i = u), n[s] = a, r[s] = u;
      let d = o, m = 0;
      for (; d !== s; ) m += n[d++], d = d % e;
      if (s = (s + 1) % e, s === o && (o = (o + 1) % e), u - i < t) return;
      const S = c && u - c;
      return S ? Math.round(m * 1e3 / S) : void 0;
    };
  }
  function Ds(e, t) {
    let n = 0, r = 1e3 / t, s, o;
    const i = (u, c = Date.now()) => {
      n = c, s = null, o && (clearTimeout(o), o = null), e(...u);
    };
    return [
      (...u) => {
        const c = Date.now(), d = c - n;
        d >= r ? i(u, c) : (s = u, o || (o = setTimeout(() => {
          o = null, i(s);
        }, r - d)));
      },
      () => s && i(s)
    ];
  }
  const je = (e, t, n = 3) => {
    let r = 0;
    const s = Bs(50, 250);
    return Ds((o) => {
      const i = o.loaded, l = o.lengthComputable ? o.total : void 0, a = i - r, u = s(a), c = i <= l;
      r = i;
      const d = {
        loaded: i,
        total: l,
        progress: l ? i / l : void 0,
        bytes: a,
        rate: u || void 0,
        estimated: u && l && c ? (l - i) / u : void 0,
        event: o,
        lengthComputable: l != null,
        [t ? "download" : "upload"]: true
      };
      e(d);
    }, n);
  }, Bt = (e, t) => {
    const n = e != null;
    return [
      (r) => t[0]({
        lengthComputable: n,
        total: e,
        loaded: r
      }),
      t[1]
    ];
  }, Dt = (e) => (...t) => f.asap(() => e(...t)), $s = X.hasStandardBrowserEnv ? /* @__PURE__ */ ((e, t) => (n) => (n = new URL(n, X.origin), e.protocol === n.protocol && e.host === n.host && (t || e.port === n.port)))(new URL(X.origin), X.navigator && /(msie|trident)/i.test(X.navigator.userAgent)) : () => true, Us = X.hasStandardBrowserEnv ? {
    write(e, t, n, r, s, o, i) {
      if (typeof document > "u") return;
      const l = [
        `${e}=${encodeURIComponent(t)}`
      ];
      f.isNumber(n) && l.push(`expires=${new Date(n).toUTCString()}`), f.isString(r) && l.push(`path=${r}`), f.isString(s) && l.push(`domain=${s}`), o === true && l.push("secure"), f.isString(i) && l.push(`SameSite=${i}`), document.cookie = l.join("; ");
    },
    read(e) {
      if (typeof document > "u") return null;
      const t = document.cookie.match(new RegExp("(?:^|; )" + e + "=([^;]*)"));
      return t ? decodeURIComponent(t[1]) : null;
    },
    remove(e) {
      this.write(e, "", Date.now() - 864e5, "/");
    }
  } : {
    write() {
    },
    read() {
      return null;
    },
    remove() {
    }
  };
  function Is(e) {
    return typeof e != "string" ? false : /^([a-z][a-z\d+\-.]*:)?\/\//i.test(e);
  }
  function Ls(e, t) {
    return t ? e.replace(/\/?\/$/, "") + "/" + t.replace(/^\/+/, "") : e;
  }
  function xn(e, t, n) {
    let r = !Is(t);
    return e && (r || n == false) ? Ls(e, t) : t;
  }
  const $t = (e) => e instanceof ee ? {
    ...e
  } : e;
  function we(e, t) {
    t = t || {};
    const n = {};
    function r(u, c, d, m) {
      return f.isPlainObject(u) && f.isPlainObject(c) ? f.merge.call({
        caseless: m
      }, u, c) : f.isPlainObject(c) ? f.merge({}, c) : f.isArray(c) ? c.slice() : c;
    }
    function s(u, c, d, m) {
      if (f.isUndefined(c)) {
        if (!f.isUndefined(u)) return r(void 0, u, d, m);
      } else return r(u, c, d, m);
    }
    function o(u, c) {
      if (!f.isUndefined(c)) return r(void 0, c);
    }
    function i(u, c) {
      if (f.isUndefined(c)) {
        if (!f.isUndefined(u)) return r(void 0, u);
      } else return r(void 0, c);
    }
    function l(u, c, d) {
      if (d in t) return r(u, c);
      if (d in e) return r(void 0, u);
    }
    const a = {
      url: o,
      method: o,
      data: o,
      baseURL: i,
      transformRequest: i,
      transformResponse: i,
      paramsSerializer: i,
      timeout: i,
      timeoutMessage: i,
      withCredentials: i,
      withXSRFToken: i,
      adapter: i,
      responseType: i,
      xsrfCookieName: i,
      xsrfHeaderName: i,
      onUploadProgress: i,
      onDownloadProgress: i,
      decompress: i,
      maxContentLength: i,
      maxBodyLength: i,
      beforeRedirect: i,
      transport: i,
      httpAgent: i,
      httpsAgent: i,
      cancelToken: i,
      socketPath: i,
      responseEncoding: i,
      validateStatus: l,
      headers: (u, c, d) => s($t(u), $t(c), d, true)
    };
    return f.forEach(Object.keys({
      ...e,
      ...t
    }), function(c) {
      if (c === "__proto__" || c === "constructor" || c === "prototype") return;
      const d = f.hasOwnProp(a, c) ? a[c] : s, m = d(e[c], t[c], c);
      f.isUndefined(m) && d !== l || (n[c] = m);
    }), n;
  }
  const wn = (e) => {
    const t = we({}, e);
    let { data: n, withXSRFToken: r, xsrfHeaderName: s, xsrfCookieName: o, headers: i, auth: l } = t;
    if (t.headers = i = ee.from(i), t.url = mn(xn(t.baseURL, t.url, t.allowAbsoluteUrls), e.params, e.paramsSerializer), l && i.set("Authorization", "Basic " + btoa((l.username || "") + ":" + (l.password ? unescape(encodeURIComponent(l.password)) : ""))), f.isFormData(n)) {
      if (X.hasStandardBrowserEnv || X.hasStandardBrowserWebWorkerEnv) i.setContentType(void 0);
      else if (f.isFunction(n.getHeaders)) {
        const a = n.getHeaders(), u = [
          "content-type",
          "content-length"
        ];
        Object.entries(a).forEach(([c, d]) => {
          u.includes(c.toLowerCase()) && i.set(c, d);
        });
      }
    }
    if (X.hasStandardBrowserEnv && (r && f.isFunction(r) && (r = r(t)), r || r !== false && $s(t.url))) {
      const a = s && o && Us.read(o);
      a && i.set(s, a);
    }
    return t;
  }, js = typeof XMLHttpRequest < "u", Ms = js && function(e) {
    return new Promise(function(n, r) {
      const s = wn(e);
      let o = s.data;
      const i = ee.from(s.headers).normalize();
      let { responseType: l, onUploadProgress: a, onDownloadProgress: u } = s, c, d, m, S, p;
      function x() {
        S && S(), p && p(), s.cancelToken && s.cancelToken.unsubscribe(c), s.signal && s.signal.removeEventListener("abort", c);
      }
      let y = new XMLHttpRequest();
      y.open(s.method.toUpperCase(), s.url, true), y.timeout = s.timeout;
      function C() {
        if (!y) return;
        const E = ee.from("getAllResponseHeaders" in y && y.getAllResponseHeaders()), B = {
          data: !l || l === "text" || l === "json" ? y.responseText : y.response,
          status: y.status,
          statusText: y.statusText,
          headers: E,
          config: e,
          request: y
        };
        gn(function(w) {
          n(w), x();
        }, function(w) {
          r(w), x();
        }, B), y = null;
      }
      "onloadend" in y ? y.onloadend = C : y.onreadystatechange = function() {
        !y || y.readyState !== 4 || y.status === 0 && !(y.responseURL && y.responseURL.indexOf("file:") === 0) || setTimeout(C);
      }, y.onabort = function() {
        y && (r(new k("Request aborted", k.ECONNABORTED, e, y)), y = null);
      }, y.onerror = function(N) {
        const B = N && N.message ? N.message : "Network Error", h = new k(B, k.ERR_NETWORK, e, y);
        h.event = N || null, r(h), y = null;
      }, y.ontimeout = function() {
        let N = s.timeout ? "timeout of " + s.timeout + "ms exceeded" : "timeout exceeded";
        const B = s.transitional || mt;
        s.timeoutErrorMessage && (N = s.timeoutErrorMessage), r(new k(N, B.clarifyTimeoutError ? k.ETIMEDOUT : k.ECONNABORTED, e, y)), y = null;
      }, o === void 0 && i.setContentType(null), "setRequestHeader" in y && f.forEach(i.toJSON(), function(N, B) {
        y.setRequestHeader(B, N);
      }), f.isUndefined(s.withCredentials) || (y.withCredentials = !!s.withCredentials), l && l !== "json" && (y.responseType = s.responseType), u && ([m, p] = je(u, true), y.addEventListener("progress", m)), a && y.upload && ([d, S] = je(a), y.upload.addEventListener("progress", d), y.upload.addEventListener("loadend", S)), (s.cancelToken || s.signal) && (c = (E) => {
        y && (r(!E || E.type ? new Be(null, e, y) : E), y.abort(), y = null);
      }, s.cancelToken && s.cancelToken.subscribe(c), s.signal && (s.signal.aborted ? c() : s.signal.addEventListener("abort", c)));
      const U = Ns(s.url);
      if (U && X.protocols.indexOf(U) === -1) {
        r(new k("Unsupported protocol " + U + ":", k.ERR_BAD_REQUEST, e));
        return;
      }
      y.send(o || null);
    });
  }, zs = (e, t) => {
    const { length: n } = e = e ? e.filter(Boolean) : [];
    if (t || n) {
      let r = new AbortController(), s;
      const o = function(u) {
        if (!s) {
          s = true, l();
          const c = u instanceof Error ? u : this.reason;
          r.abort(c instanceof k ? c : new Be(c instanceof Error ? c.message : c));
        }
      };
      let i = t && setTimeout(() => {
        i = null, o(new k(`timeout of ${t}ms exceeded`, k.ETIMEDOUT));
      }, t);
      const l = () => {
        e && (i && clearTimeout(i), i = null, e.forEach((u) => {
          u.unsubscribe ? u.unsubscribe(o) : u.removeEventListener("abort", o);
        }), e = null);
      };
      e.forEach((u) => u.addEventListener("abort", o));
      const { signal: a } = r;
      return a.unsubscribe = () => f.asap(l), a;
    }
  }, Hs = function* (e, t) {
    let n = e.byteLength;
    if (n < t) {
      yield e;
      return;
    }
    let r = 0, s;
    for (; r < n; ) s = r + t, yield e.slice(r, s), r = s;
  }, qs = async function* (e, t) {
    for await (const n of Js(e)) yield* Hs(n, t);
  }, Js = async function* (e) {
    if (e[Symbol.asyncIterator]) {
      yield* e;
      return;
    }
    const t = e.getReader();
    try {
      for (; ; ) {
        const { done: n, value: r } = await t.read();
        if (n) break;
        yield r;
      }
    } finally {
      await t.cancel();
    }
  }, Ut = (e, t, n, r) => {
    const s = qs(e, t);
    let o = 0, i, l = (a) => {
      i || (i = true, r && r(a));
    };
    return new ReadableStream({
      async pull(a) {
        try {
          const { done: u, value: c } = await s.next();
          if (u) {
            l(), a.close();
            return;
          }
          let d = c.byteLength;
          if (n) {
            let m = o += d;
            n(m);
          }
          a.enqueue(new Uint8Array(c));
        } catch (u) {
          throw l(u), u;
        }
      },
      cancel(a) {
        return l(a), s.return();
      }
    }, {
      highWaterMark: 2
    });
  }, It = 64 * 1024, { isFunction: $e } = f, Vs = (({ Request: e, Response: t }) => ({
    Request: e,
    Response: t
  }))(f.global), { ReadableStream: Lt, TextEncoder: jt } = f.global, Mt = (e, ...t) => {
    try {
      return !!e(...t);
    } catch {
      return false;
    }
  }, Ws = (e) => {
    e = f.merge.call({
      skipUndefined: true
    }, Vs, e);
    const { fetch: t, Request: n, Response: r } = e, s = t ? $e(t) : typeof fetch == "function", o = $e(n), i = $e(r);
    if (!s) return false;
    const l = s && $e(Lt), a = s && (typeof jt == "function" ? /* @__PURE__ */ ((p) => (x) => p.encode(x))(new jt()) : async (p) => new Uint8Array(await new n(p).arrayBuffer())), u = o && l && Mt(() => {
      let p = false;
      const x = new n(X.origin, {
        body: new Lt(),
        method: "POST",
        get duplex() {
          return p = true, "half";
        }
      }).headers.has("Content-Type");
      return p && !x;
    }), c = i && l && Mt(() => f.isReadableStream(new r("").body)), d = {
      stream: c && ((p) => p.body)
    };
    s && [
      "text",
      "arrayBuffer",
      "blob",
      "formData",
      "stream"
    ].forEach((p) => {
      !d[p] && (d[p] = (x, y) => {
        let C = x && x[p];
        if (C) return C.call(x);
        throw new k(`Response type '${p}' is not supported`, k.ERR_NOT_SUPPORT, y);
      });
    });
    const m = async (p) => {
      if (p == null) return 0;
      if (f.isBlob(p)) return p.size;
      if (f.isSpecCompliantForm(p)) return (await new n(X.origin, {
        method: "POST",
        body: p
      }).arrayBuffer()).byteLength;
      if (f.isArrayBufferView(p) || f.isArrayBuffer(p)) return p.byteLength;
      if (f.isURLSearchParams(p) && (p = p + ""), f.isString(p)) return (await a(p)).byteLength;
    }, S = async (p, x) => {
      const y = f.toFiniteNumber(p.getContentLength());
      return y ?? m(x);
    };
    return async (p) => {
      let { url: x, method: y, data: C, signal: U, cancelToken: E, timeout: N, onDownloadProgress: B, onUploadProgress: h, responseType: w, headers: R, withCredentials: _ = "same-origin", fetchOptions: A } = wn(p), v = t || fetch;
      w = w ? (w + "").toLowerCase() : "text";
      let P = zs([
        U,
        E && E.toAbortSignal()
      ], N), b = null;
      const T = P && P.unsubscribe && (() => {
        P.unsubscribe();
      });
      let j;
      try {
        if (h && u && y !== "get" && y !== "head" && (j = await S(R, C)) !== 0) {
          let H = new n(x, {
            method: "POST",
            body: C,
            duplex: "half"
          }), Z;
          if (f.isFormData(C) && (Z = H.headers.get("content-type")) && R.setContentType(Z), H.body) {
            const [oe, ne] = Bt(j, je(Dt(h)));
            C = Ut(H.body, It, oe, ne);
          }
        }
        f.isString(_) || (_ = _ ? "include" : "omit");
        const M = o && "credentials" in n.prototype, W = {
          ...A,
          signal: P,
          method: y.toUpperCase(),
          headers: R.normalize().toJSON(),
          body: C,
          duplex: "half",
          credentials: M ? _ : void 0
        };
        b = o && new n(x, W);
        let V = await (o ? v(b, A) : v(x, W));
        const D = c && (w === "stream" || w === "response");
        if (c && (B || D && T)) {
          const H = {};
          [
            "status",
            "statusText",
            "headers"
          ].forEach((ie) => {
            H[ie] = V[ie];
          });
          const Z = f.toFiniteNumber(V.headers.get("content-length")), [oe, ne] = B && Bt(Z, je(Dt(B), true)) || [];
          V = new r(Ut(V.body, It, oe, () => {
            ne && ne(), T && T();
          }), H);
        }
        w = w || "text";
        let J = await d[f.findKey(d, w) || "text"](V, p);
        return !D && T && T(), await new Promise((H, Z) => {
          gn(H, Z, {
            data: J,
            headers: ee.from(V.headers),
            status: V.status,
            statusText: V.statusText,
            config: p,
            request: b
          });
        });
      } catch (M) {
        throw T && T(), M && M.name === "TypeError" && /Load failed|fetch/i.test(M.message) ? Object.assign(new k("Network Error", k.ERR_NETWORK, p, b, M && M.response), {
          cause: M.cause || M
        }) : k.from(M, M && M.code, p, b, M && M.response);
      }
    };
  }, Ks = /* @__PURE__ */ new Map(), vn = (e) => {
    let t = e && e.env || {};
    const { fetch: n, Request: r, Response: s } = t, o = [
      r,
      s,
      n
    ];
    let i = o.length, l = i, a, u, c = Ks;
    for (; l--; ) a = o[l], u = c.get(a), u === void 0 && c.set(a, u = l ? /* @__PURE__ */ new Map() : Ws(t)), c = u;
    return u;
  };
  vn();
  const bt = {
    http: fs,
    xhr: Ms,
    fetch: {
      get: vn
    }
  };
  f.forEach(bt, (e, t) => {
    if (e) {
      try {
        Object.defineProperty(e, "name", {
          value: t
        });
      } catch {
      }
      Object.defineProperty(e, "adapterName", {
        value: t
      });
    }
  });
  const zt = (e) => `- ${e}`, Gs = (e) => f.isFunction(e) || e === null || e === false;
  function Xs(e, t) {
    e = f.isArray(e) ? e : [
      e
    ];
    const { length: n } = e;
    let r, s;
    const o = {};
    for (let i = 0; i < n; i++) {
      r = e[i];
      let l;
      if (s = r, !Gs(r) && (s = bt[(l = String(r)).toLowerCase()], s === void 0)) throw new k(`Unknown adapter '${l}'`);
      if (s && (f.isFunction(s) || (s = s.get(t)))) break;
      o[l || "#" + i] = s;
    }
    if (!s) {
      const i = Object.entries(o).map(([a, u]) => `adapter ${a} ` + (u === false ? "is not supported by the environment" : "is not available in the build"));
      let l = n ? i.length > 1 ? `since :
` + i.map(zt).join(`
`) : " " + zt(i[0]) : "as no adapter specified";
      throw new k("There is no suitable adapter to dispatch the request " + l, "ERR_NOT_SUPPORT");
    }
    return s;
  }
  const _n = {
    getAdapter: Xs,
    adapters: bt
  };
  function et(e) {
    if (e.cancelToken && e.cancelToken.throwIfRequested(), e.signal && e.signal.aborted) throw new Be(null, e);
  }
  function Ht(e) {
    return et(e), e.headers = ee.from(e.headers), e.data = Qe.call(e, e.transformRequest), [
      "post",
      "put",
      "patch"
    ].indexOf(e.method) !== -1 && e.headers.setContentType("application/x-www-form-urlencoded", false), _n.getAdapter(e.adapter || Ne.adapter, e)(e).then(function(r) {
      return et(e), r.data = Qe.call(e, e.transformResponse, r), r.headers = ee.from(r.headers), r;
    }, function(r) {
      return bn(r) || (et(e), r && r.response && (r.response.data = Qe.call(e, e.transformResponse, r.response), r.response.headers = ee.from(r.response.headers))), Promise.reject(r);
    });
  }
  const En = "1.13.5", We = {};
  [
    "object",
    "boolean",
    "number",
    "function",
    "string",
    "symbol"
  ].forEach((e, t) => {
    We[e] = function(r) {
      return typeof r === e || "a" + (t < 1 ? "n " : " ") + e;
    };
  });
  const qt = {};
  We.transitional = function(t, n, r) {
    function s(o, i) {
      return "[Axios v" + En + "] Transitional option '" + o + "'" + i + (r ? ". " + r : "");
    }
    return (o, i, l) => {
      if (t === false) throw new k(s(i, " has been removed" + (n ? " in " + n : "")), k.ERR_DEPRECATED);
      return n && !qt[i] && (qt[i] = true, console.warn(s(i, " has been deprecated since v" + n + " and will be removed in the near future"))), t ? t(o, i, l) : true;
    };
  };
  We.spelling = function(t) {
    return (n, r) => (console.warn(`${r} is likely a misspelling of ${t}`), true);
  };
  function Ys(e, t, n) {
    if (typeof e != "object") throw new k("options must be an object", k.ERR_BAD_OPTION_VALUE);
    const r = Object.keys(e);
    let s = r.length;
    for (; s-- > 0; ) {
      const o = r[s], i = t[o];
      if (i) {
        const l = e[o], a = l === void 0 || i(l, o, e);
        if (a !== true) throw new k("option " + o + " must be " + a, k.ERR_BAD_OPTION_VALUE);
        continue;
      }
      if (n !== true) throw new k("Unknown option " + o, k.ERR_BAD_OPTION);
    }
  }
  const Le = {
    assertOptions: Ys,
    validators: We
  }, te = Le.validators;
  let xe = class {
    constructor(t) {
      this.defaults = t || {}, this.interceptors = {
        request: new Pt(),
        response: new Pt()
      };
    }
    async request(t, n) {
      try {
        return await this._request(t, n);
      } catch (r) {
        if (r instanceof Error) {
          let s = {};
          Error.captureStackTrace ? Error.captureStackTrace(s) : s = new Error();
          const o = s.stack ? s.stack.replace(/^.+\n/, "") : "";
          try {
            r.stack ? o && !String(r.stack).endsWith(o.replace(/^.+\n.+\n/, "")) && (r.stack += `
` + o) : r.stack = o;
          } catch {
          }
        }
        throw r;
      }
    }
    _request(t, n) {
      typeof t == "string" ? (n = n || {}, n.url = t) : n = t || {}, n = we(this.defaults, n);
      const { transitional: r, paramsSerializer: s, headers: o } = n;
      r !== void 0 && Le.assertOptions(r, {
        silentJSONParsing: te.transitional(te.boolean),
        forcedJSONParsing: te.transitional(te.boolean),
        clarifyTimeoutError: te.transitional(te.boolean),
        legacyInterceptorReqResOrdering: te.transitional(te.boolean)
      }, false), s != null && (f.isFunction(s) ? n.paramsSerializer = {
        serialize: s
      } : Le.assertOptions(s, {
        encode: te.function,
        serialize: te.function
      }, true)), n.allowAbsoluteUrls !== void 0 || (this.defaults.allowAbsoluteUrls !== void 0 ? n.allowAbsoluteUrls = this.defaults.allowAbsoluteUrls : n.allowAbsoluteUrls = true), Le.assertOptions(n, {
        baseUrl: te.spelling("baseURL"),
        withXsrfToken: te.spelling("withXSRFToken")
      }, true), n.method = (n.method || this.defaults.method || "get").toLowerCase();
      let i = o && f.merge(o.common, o[n.method]);
      o && f.forEach([
        "delete",
        "get",
        "head",
        "post",
        "put",
        "patch",
        "common"
      ], (p) => {
        delete o[p];
      }), n.headers = ee.concat(i, o);
      const l = [];
      let a = true;
      this.interceptors.request.forEach(function(x) {
        if (typeof x.runWhen == "function" && x.runWhen(n) === false) return;
        a = a && x.synchronous;
        const y = n.transitional || mt;
        y && y.legacyInterceptorReqResOrdering ? l.unshift(x.fulfilled, x.rejected) : l.push(x.fulfilled, x.rejected);
      });
      const u = [];
      this.interceptors.response.forEach(function(x) {
        u.push(x.fulfilled, x.rejected);
      });
      let c, d = 0, m;
      if (!a) {
        const p = [
          Ht.bind(this),
          void 0
        ];
        for (p.unshift(...l), p.push(...u), m = p.length, c = Promise.resolve(n); d < m; ) c = c.then(p[d++], p[d++]);
        return c;
      }
      m = l.length;
      let S = n;
      for (; d < m; ) {
        const p = l[d++], x = l[d++];
        try {
          S = p(S);
        } catch (y) {
          x.call(this, y);
          break;
        }
      }
      try {
        c = Ht.call(this, S);
      } catch (p) {
        return Promise.reject(p);
      }
      for (d = 0, m = u.length; d < m; ) c = c.then(u[d++], u[d++]);
      return c;
    }
    getUri(t) {
      t = we(this.defaults, t);
      const n = xn(t.baseURL, t.url, t.allowAbsoluteUrls);
      return mn(n, t.params, t.paramsSerializer);
    }
  };
  f.forEach([
    "delete",
    "get",
    "head",
    "options"
  ], function(t) {
    xe.prototype[t] = function(n, r) {
      return this.request(we(r || {}, {
        method: t,
        url: n,
        data: (r || {}).data
      }));
    };
  });
  f.forEach([
    "post",
    "put",
    "patch"
  ], function(t) {
    function n(r) {
      return function(o, i, l) {
        return this.request(we(l || {}, {
          method: t,
          headers: r ? {
            "Content-Type": "multipart/form-data"
          } : {},
          url: o,
          data: i
        }));
      };
    }
    xe.prototype[t] = n(), xe.prototype[t + "Form"] = n(true);
  });
  let Zs = class Sn {
    constructor(t) {
      if (typeof t != "function") throw new TypeError("executor must be a function.");
      let n;
      this.promise = new Promise(function(o) {
        n = o;
      });
      const r = this;
      this.promise.then((s) => {
        if (!r._listeners) return;
        let o = r._listeners.length;
        for (; o-- > 0; ) r._listeners[o](s);
        r._listeners = null;
      }), this.promise.then = (s) => {
        let o;
        const i = new Promise((l) => {
          r.subscribe(l), o = l;
        }).then(s);
        return i.cancel = function() {
          r.unsubscribe(o);
        }, i;
      }, t(function(o, i, l) {
        r.reason || (r.reason = new Be(o, i, l), n(r.reason));
      });
    }
    throwIfRequested() {
      if (this.reason) throw this.reason;
    }
    subscribe(t) {
      if (this.reason) {
        t(this.reason);
        return;
      }
      this._listeners ? this._listeners.push(t) : this._listeners = [
        t
      ];
    }
    unsubscribe(t) {
      if (!this._listeners) return;
      const n = this._listeners.indexOf(t);
      n !== -1 && this._listeners.splice(n, 1);
    }
    toAbortSignal() {
      const t = new AbortController(), n = (r) => {
        t.abort(r);
      };
      return this.subscribe(n), t.signal.unsubscribe = () => this.unsubscribe(n), t.signal;
    }
    static source() {
      let t;
      return {
        token: new Sn(function(s) {
          t = s;
        }),
        cancel: t
      };
    }
  };
  function Qs(e) {
    return function(n) {
      return e.apply(null, n);
    };
  }
  function eo(e) {
    return f.isObject(e) && e.isAxiosError === true;
  }
  const lt = {
    Continue: 100,
    SwitchingProtocols: 101,
    Processing: 102,
    EarlyHints: 103,
    Ok: 200,
    Created: 201,
    Accepted: 202,
    NonAuthoritativeInformation: 203,
    NoContent: 204,
    ResetContent: 205,
    PartialContent: 206,
    MultiStatus: 207,
    AlreadyReported: 208,
    ImUsed: 226,
    MultipleChoices: 300,
    MovedPermanently: 301,
    Found: 302,
    SeeOther: 303,
    NotModified: 304,
    UseProxy: 305,
    Unused: 306,
    TemporaryRedirect: 307,
    PermanentRedirect: 308,
    BadRequest: 400,
    Unauthorized: 401,
    PaymentRequired: 402,
    Forbidden: 403,
    NotFound: 404,
    MethodNotAllowed: 405,
    NotAcceptable: 406,
    ProxyAuthenticationRequired: 407,
    RequestTimeout: 408,
    Conflict: 409,
    Gone: 410,
    LengthRequired: 411,
    PreconditionFailed: 412,
    PayloadTooLarge: 413,
    UriTooLong: 414,
    UnsupportedMediaType: 415,
    RangeNotSatisfiable: 416,
    ExpectationFailed: 417,
    ImATeapot: 418,
    MisdirectedRequest: 421,
    UnprocessableEntity: 422,
    Locked: 423,
    FailedDependency: 424,
    TooEarly: 425,
    UpgradeRequired: 426,
    PreconditionRequired: 428,
    TooManyRequests: 429,
    RequestHeaderFieldsTooLarge: 431,
    UnavailableForLegalReasons: 451,
    InternalServerError: 500,
    NotImplemented: 501,
    BadGateway: 502,
    ServiceUnavailable: 503,
    GatewayTimeout: 504,
    HttpVersionNotSupported: 505,
    VariantAlsoNegotiates: 506,
    InsufficientStorage: 507,
    LoopDetected: 508,
    NotExtended: 510,
    NetworkAuthenticationRequired: 511,
    WebServerIsDown: 521,
    ConnectionTimedOut: 522,
    OriginIsUnreachable: 523,
    TimeoutOccurred: 524,
    SslHandshakeFailed: 525,
    InvalidSslCertificate: 526
  };
  Object.entries(lt).forEach(([e, t]) => {
    lt[t] = e;
  });
  function Fn(e) {
    const t = new xe(e), n = rn(xe.prototype.request, t);
    return f.extend(n, xe.prototype, t, {
      allOwnKeys: true
    }), f.extend(n, t, null, {
      allOwnKeys: true
    }), n.create = function(s) {
      return Fn(we(e, s));
    }, n;
  }
  const q = Fn(Ne);
  q.Axios = xe;
  q.CanceledError = Be;
  q.CancelToken = Zs;
  q.isCancel = bn;
  q.VERSION = En;
  q.toFormData = Ve;
  q.AxiosError = k;
  q.Cancel = q.CanceledError;
  q.all = function(t) {
    return Promise.all(t);
  };
  q.spread = Qs;
  q.isAxiosError = eo;
  q.mergeConfig = we;
  q.AxiosHeaders = ee;
  q.formToJSON = (e) => yn(f.isHTMLForm(e) ? new FormData(e) : e);
  q.getAdapter = _n.getAdapter;
  q.HttpStatusCode = lt;
  q.default = q;
  const { Axios: hi, AxiosError: pi, CanceledError: mi, isCancel: yi, CancelToken: bi, VERSION: gi, all: xi, Cancel: wi, isAxiosError: vi, spread: _i, toFormData: Ei, AxiosHeaders: Si, HttpStatusCode: Fi, formToJSON: Ri, getAdapter: Ai, mergeConfig: Ci } = q;
  let tt = null;
  const ce = async () => {
    if (tt) return tt;
    const { useToast: e } = await kn(async () => {
      const { useToast: t } = await import("./index-DbbiUtAi.js").then(async (m) => {
        await m.__tla;
        return m;
      }).then((n) => n.I);
      return {
        useToast: t
      };
    }, __vite__mapDeps([0,1]));
    return tt = e();
  };
  let Re = null;
  const de = () => (Re || (Re = q.create({
    baseURL: st.apiUrl
  }), Re.interceptors.request.use((e) => {
    const t = ze();
    return t.token && (e.headers.Authorization = t.token), e;
  }), Re.interceptors.response.use((e) => e, (e) => (ce().then((t) => {
    var _a, _b;
    let n = "\u64CD\u4F5C\u5931\u8D25";
    if (e.response) {
      const r = e.response.status;
      switch (r) {
        case 401:
          n = "\u672A\u6388\u6743\u8BBF\u95EE\uFF0C\u8BF7\u91CD\u65B0\u767B\u5F55";
          break;
        case 500:
          n = "\u670D\u52A1\u5668\u9519\u8BEF\uFF0C\u64CD\u4F5C\u5931\u8D25";
          break;
        default:
          n = ((_b = (_a = e.response) == null ? void 0 : _a.data) == null ? void 0 : _b.message) || `\u64CD\u4F5C\u5931\u8D25 (${r})`;
      }
    } else e.message && (n = e.message);
    t.error(n);
  }), Promise.reject(e)))), Re), to = (e) => de().post("/file/createfolder", {
    path: e
  }), no = (...e) => de().post("/file/delete", e.map((t) => ({
    deletePath: t
  }))), ro = (e, t) => de().post("/file/move", [
    {
      targetPath: e,
      newPath: Ct.join(Ct.dirname(e), t)
    }
  ]), so = (e, t, ...n) => {
    const r = new FormData();
    for (const s of n) r.append("files", s);
    return r.append("path", e), de().post("/file/uploadfile", r, {
      onUploadProgress: (s) => {
        if (t && s.total) {
          const o = Math.round(s.loaded * 100 / s.total);
          t(o);
        }
      }
    });
  }, oo = (e, t) => de().post("/file/copy", t.map((n) => ({
    targetPath: n.path,
    newPath: e + `/${n.name}`
  }))), io = (e, t) => de().post("/file/move", t.map((n) => ({
    targetPath: n.path,
    newPath: e + `/${n.name}`
  }))), me = {
    createFolder: to,
    deleteItem: no,
    renameItem: ro,
    uploadFile: so,
    copyFiles: oo,
    moveFiles: io
  };
  async function ao(e, t, n, r, s) {
    const o = [];
    for (const c in e) {
      const d = e[c];
      for (const m in d) {
        const S = d[m];
        o.push({
          fn: async () => {
            try {
              const p = await S();
              return s == null ? void 0 : s(Number(c), Number(m), {
                status: "fulfilled",
                value: p
              }), p;
            } catch (p) {
              throw s == null ? void 0 : s(Number(c), Number(m), {
                status: "rejected",
                reason: p
              }), p;
            }
          },
          group: Number(c)
        });
      }
    }
    const i = e.map(() => []);
    let l = 0;
    async function a() {
      for (; l < o.length; ) {
        const c = l++, { fn: d, group: m } = o[c];
        try {
          const S = await d();
          i[m].push({
            status: "fulfilled",
            value: S
          });
        } catch (S) {
          i[m].push({
            status: "rejected",
            reason: S
          });
        }
        i[m].length >= e[m].length && n(m, i[m]);
      }
    }
    const u = Array.from({
      length: t
    }, () => a());
    await Promise.all(u), r == null ? void 0 : r(i);
  }
  var nt = {
    exports: {}
  };
  const lo = {}, co = Object.freeze(Object.defineProperty({
    __proto__: null,
    default: lo
  }, Symbol.toStringTag, {
    value: "Module"
  })), Jt = Er(co);
  var Vt;
  function uo() {
    return Vt || (Vt = 1, (function(e) {
      (function() {
        var t = "input is invalid type", n = typeof window == "object", r = n ? window : {};
        r.JS_SHA256_NO_WINDOW && (n = false);
        var s = !n && typeof self == "object", o = !r.JS_SHA256_NO_NODE_JS && typeof process == "object" && process.versions && process.versions.node && process.type != "renderer";
        o ? r = vr : s && (r = self);
        var i = !r.JS_SHA256_NO_COMMON_JS && true && e.exports, l = !r.JS_SHA256_NO_ARRAY_BUFFER && typeof ArrayBuffer < "u", a = "0123456789abcdef".split(""), u = [
          -2147483648,
          8388608,
          32768,
          128
        ], c = [
          24,
          16,
          8,
          0
        ], d = [
          1116352408,
          1899447441,
          3049323471,
          3921009573,
          961987163,
          1508970993,
          2453635748,
          2870763221,
          3624381080,
          310598401,
          607225278,
          1426881987,
          1925078388,
          2162078206,
          2614888103,
          3248222580,
          3835390401,
          4022224774,
          264347078,
          604807628,
          770255983,
          1249150122,
          1555081692,
          1996064986,
          2554220882,
          2821834349,
          2952996808,
          3210313671,
          3336571891,
          3584528711,
          113926993,
          338241895,
          666307205,
          773529912,
          1294757372,
          1396182291,
          1695183700,
          1986661051,
          2177026350,
          2456956037,
          2730485921,
          2820302411,
          3259730800,
          3345764771,
          3516065817,
          3600352804,
          4094571909,
          275423344,
          430227734,
          506948616,
          659060556,
          883997877,
          958139571,
          1322822218,
          1537002063,
          1747873779,
          1955562222,
          2024104815,
          2227730452,
          2361852424,
          2428436474,
          2756734187,
          3204031479,
          3329325298
        ], m = [
          "hex",
          "array",
          "digest",
          "arrayBuffer"
        ], S = [];
        (r.JS_SHA256_NO_NODE_JS || !Array.isArray) && (Array.isArray = function(h) {
          return Object.prototype.toString.call(h) === "[object Array]";
        }), l && (r.JS_SHA256_NO_ARRAY_BUFFER_IS_VIEW || !ArrayBuffer.isView) && (ArrayBuffer.isView = function(h) {
          return typeof h == "object" && h.buffer && h.buffer.constructor === ArrayBuffer;
        });
        var p = function(h, w) {
          return function(R) {
            return new E(w, true).update(R)[h]();
          };
        }, x = function(h) {
          var w = p("hex", h);
          o && (w = y(w, h)), w.create = function() {
            return new E(h);
          }, w.update = function(A) {
            return w.create().update(A);
          };
          for (var R = 0; R < m.length; ++R) {
            var _ = m[R];
            w[_] = p(_, h);
          }
          return w;
        }, y = function(h, w) {
          var R = Jt, _ = Jt.Buffer, A = w ? "sha224" : "sha256", v;
          _.from && !r.JS_SHA256_NO_BUFFER_FROM ? v = _.from : v = function(b) {
            return new _(b);
          };
          var P = function(b) {
            if (typeof b == "string") return R.createHash(A).update(b, "utf8").digest("hex");
            if (b == null) throw new Error(t);
            return b.constructor === ArrayBuffer && (b = new Uint8Array(b)), Array.isArray(b) || ArrayBuffer.isView(b) || b.constructor === _ ? R.createHash(A).update(v(b)).digest("hex") : h(b);
          };
          return P;
        }, C = function(h, w) {
          return function(R, _) {
            return new N(R, w, true).update(_)[h]();
          };
        }, U = function(h) {
          var w = C("hex", h);
          w.create = function(A) {
            return new N(A, h);
          }, w.update = function(A, v) {
            return w.create(A).update(v);
          };
          for (var R = 0; R < m.length; ++R) {
            var _ = m[R];
            w[_] = C(_, h);
          }
          return w;
        };
        function E(h, w) {
          w ? (S[0] = S[16] = S[1] = S[2] = S[3] = S[4] = S[5] = S[6] = S[7] = S[8] = S[9] = S[10] = S[11] = S[12] = S[13] = S[14] = S[15] = 0, this.blocks = S) : this.blocks = [
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0
          ], h ? (this.h0 = 3238371032, this.h1 = 914150663, this.h2 = 812702999, this.h3 = 4144912697, this.h4 = 4290775857, this.h5 = 1750603025, this.h6 = 1694076839, this.h7 = 3204075428) : (this.h0 = 1779033703, this.h1 = 3144134277, this.h2 = 1013904242, this.h3 = 2773480762, this.h4 = 1359893119, this.h5 = 2600822924, this.h6 = 528734635, this.h7 = 1541459225), this.block = this.start = this.bytes = this.hBytes = 0, this.finalized = this.hashed = false, this.first = true, this.is224 = h;
        }
        E.prototype.update = function(h) {
          if (!this.finalized) {
            var w, R = typeof h;
            if (R !== "string") {
              if (R === "object") {
                if (h === null) throw new Error(t);
                if (l && h.constructor === ArrayBuffer) h = new Uint8Array(h);
                else if (!Array.isArray(h) && (!l || !ArrayBuffer.isView(h))) throw new Error(t);
              } else throw new Error(t);
              w = true;
            }
            for (var _, A = 0, v, P = h.length, b = this.blocks; A < P; ) {
              if (this.hashed && (this.hashed = false, b[0] = this.block, this.block = b[16] = b[1] = b[2] = b[3] = b[4] = b[5] = b[6] = b[7] = b[8] = b[9] = b[10] = b[11] = b[12] = b[13] = b[14] = b[15] = 0), w) for (v = this.start; A < P && v < 64; ++A) b[v >>> 2] |= h[A] << c[v++ & 3];
              else for (v = this.start; A < P && v < 64; ++A) _ = h.charCodeAt(A), _ < 128 ? b[v >>> 2] |= _ << c[v++ & 3] : _ < 2048 ? (b[v >>> 2] |= (192 | _ >>> 6) << c[v++ & 3], b[v >>> 2] |= (128 | _ & 63) << c[v++ & 3]) : _ < 55296 || _ >= 57344 ? (b[v >>> 2] |= (224 | _ >>> 12) << c[v++ & 3], b[v >>> 2] |= (128 | _ >>> 6 & 63) << c[v++ & 3], b[v >>> 2] |= (128 | _ & 63) << c[v++ & 3]) : (_ = 65536 + ((_ & 1023) << 10 | h.charCodeAt(++A) & 1023), b[v >>> 2] |= (240 | _ >>> 18) << c[v++ & 3], b[v >>> 2] |= (128 | _ >>> 12 & 63) << c[v++ & 3], b[v >>> 2] |= (128 | _ >>> 6 & 63) << c[v++ & 3], b[v >>> 2] |= (128 | _ & 63) << c[v++ & 3]);
              this.lastByteIndex = v, this.bytes += v - this.start, v >= 64 ? (this.block = b[16], this.start = v - 64, this.hash(), this.hashed = true) : this.start = v;
            }
            return this.bytes > 4294967295 && (this.hBytes += this.bytes / 4294967296 << 0, this.bytes = this.bytes % 4294967296), this;
          }
        }, E.prototype.finalize = function() {
          if (!this.finalized) {
            this.finalized = true;
            var h = this.blocks, w = this.lastByteIndex;
            h[16] = this.block, h[w >>> 2] |= u[w & 3], this.block = h[16], w >= 56 && (this.hashed || this.hash(), h[0] = this.block, h[16] = h[1] = h[2] = h[3] = h[4] = h[5] = h[6] = h[7] = h[8] = h[9] = h[10] = h[11] = h[12] = h[13] = h[14] = h[15] = 0), h[14] = this.hBytes << 3 | this.bytes >>> 29, h[15] = this.bytes << 3, this.hash();
          }
        }, E.prototype.hash = function() {
          var h = this.h0, w = this.h1, R = this.h2, _ = this.h3, A = this.h4, v = this.h5, P = this.h6, b = this.h7, T = this.blocks, j, M, W, V, D, J, H, Z, oe, ne, ie;
          for (j = 16; j < 64; ++j) D = T[j - 15], M = (D >>> 7 | D << 25) ^ (D >>> 18 | D << 14) ^ D >>> 3, D = T[j - 2], W = (D >>> 17 | D << 15) ^ (D >>> 19 | D << 13) ^ D >>> 10, T[j] = T[j - 16] + M + T[j - 7] + W << 0;
          for (ie = w & R, j = 0; j < 64; j += 4) this.first ? (this.is224 ? (Z = 300032, D = T[0] - 1413257819, b = D - 150054599 << 0, _ = D + 24177077 << 0) : (Z = 704751109, D = T[0] - 210244248, b = D - 1521486534 << 0, _ = D + 143694565 << 0), this.first = false) : (M = (h >>> 2 | h << 30) ^ (h >>> 13 | h << 19) ^ (h >>> 22 | h << 10), W = (A >>> 6 | A << 26) ^ (A >>> 11 | A << 21) ^ (A >>> 25 | A << 7), Z = h & w, V = Z ^ h & R ^ ie, H = A & v ^ ~A & P, D = b + W + H + d[j] + T[j], J = M + V, b = _ + D << 0, _ = D + J << 0), M = (_ >>> 2 | _ << 30) ^ (_ >>> 13 | _ << 19) ^ (_ >>> 22 | _ << 10), W = (b >>> 6 | b << 26) ^ (b >>> 11 | b << 21) ^ (b >>> 25 | b << 7), oe = _ & h, V = oe ^ _ & w ^ Z, H = b & A ^ ~b & v, D = P + W + H + d[j + 1] + T[j + 1], J = M + V, P = R + D << 0, R = D + J << 0, M = (R >>> 2 | R << 30) ^ (R >>> 13 | R << 19) ^ (R >>> 22 | R << 10), W = (P >>> 6 | P << 26) ^ (P >>> 11 | P << 21) ^ (P >>> 25 | P << 7), ne = R & _, V = ne ^ R & h ^ oe, H = P & b ^ ~P & A, D = v + W + H + d[j + 2] + T[j + 2], J = M + V, v = w + D << 0, w = D + J << 0, M = (w >>> 2 | w << 30) ^ (w >>> 13 | w << 19) ^ (w >>> 22 | w << 10), W = (v >>> 6 | v << 26) ^ (v >>> 11 | v << 21) ^ (v >>> 25 | v << 7), ie = w & R, V = ie ^ w & _ ^ ne, H = v & P ^ ~v & b, D = A + W + H + d[j + 3] + T[j + 3], J = M + V, A = h + D << 0, h = D + J << 0, this.chromeBugWorkAround = true;
          this.h0 = this.h0 + h << 0, this.h1 = this.h1 + w << 0, this.h2 = this.h2 + R << 0, this.h3 = this.h3 + _ << 0, this.h4 = this.h4 + A << 0, this.h5 = this.h5 + v << 0, this.h6 = this.h6 + P << 0, this.h7 = this.h7 + b << 0;
        }, E.prototype.hex = function() {
          this.finalize();
          var h = this.h0, w = this.h1, R = this.h2, _ = this.h3, A = this.h4, v = this.h5, P = this.h6, b = this.h7, T = a[h >>> 28 & 15] + a[h >>> 24 & 15] + a[h >>> 20 & 15] + a[h >>> 16 & 15] + a[h >>> 12 & 15] + a[h >>> 8 & 15] + a[h >>> 4 & 15] + a[h & 15] + a[w >>> 28 & 15] + a[w >>> 24 & 15] + a[w >>> 20 & 15] + a[w >>> 16 & 15] + a[w >>> 12 & 15] + a[w >>> 8 & 15] + a[w >>> 4 & 15] + a[w & 15] + a[R >>> 28 & 15] + a[R >>> 24 & 15] + a[R >>> 20 & 15] + a[R >>> 16 & 15] + a[R >>> 12 & 15] + a[R >>> 8 & 15] + a[R >>> 4 & 15] + a[R & 15] + a[_ >>> 28 & 15] + a[_ >>> 24 & 15] + a[_ >>> 20 & 15] + a[_ >>> 16 & 15] + a[_ >>> 12 & 15] + a[_ >>> 8 & 15] + a[_ >>> 4 & 15] + a[_ & 15] + a[A >>> 28 & 15] + a[A >>> 24 & 15] + a[A >>> 20 & 15] + a[A >>> 16 & 15] + a[A >>> 12 & 15] + a[A >>> 8 & 15] + a[A >>> 4 & 15] + a[A & 15] + a[v >>> 28 & 15] + a[v >>> 24 & 15] + a[v >>> 20 & 15] + a[v >>> 16 & 15] + a[v >>> 12 & 15] + a[v >>> 8 & 15] + a[v >>> 4 & 15] + a[v & 15] + a[P >>> 28 & 15] + a[P >>> 24 & 15] + a[P >>> 20 & 15] + a[P >>> 16 & 15] + a[P >>> 12 & 15] + a[P >>> 8 & 15] + a[P >>> 4 & 15] + a[P & 15];
          return this.is224 || (T += a[b >>> 28 & 15] + a[b >>> 24 & 15] + a[b >>> 20 & 15] + a[b >>> 16 & 15] + a[b >>> 12 & 15] + a[b >>> 8 & 15] + a[b >>> 4 & 15] + a[b & 15]), T;
        }, E.prototype.toString = E.prototype.hex, E.prototype.digest = function() {
          this.finalize();
          var h = this.h0, w = this.h1, R = this.h2, _ = this.h3, A = this.h4, v = this.h5, P = this.h6, b = this.h7, T = [
            h >>> 24 & 255,
            h >>> 16 & 255,
            h >>> 8 & 255,
            h & 255,
            w >>> 24 & 255,
            w >>> 16 & 255,
            w >>> 8 & 255,
            w & 255,
            R >>> 24 & 255,
            R >>> 16 & 255,
            R >>> 8 & 255,
            R & 255,
            _ >>> 24 & 255,
            _ >>> 16 & 255,
            _ >>> 8 & 255,
            _ & 255,
            A >>> 24 & 255,
            A >>> 16 & 255,
            A >>> 8 & 255,
            A & 255,
            v >>> 24 & 255,
            v >>> 16 & 255,
            v >>> 8 & 255,
            v & 255,
            P >>> 24 & 255,
            P >>> 16 & 255,
            P >>> 8 & 255,
            P & 255
          ];
          return this.is224 || T.push(b >>> 24 & 255, b >>> 16 & 255, b >>> 8 & 255, b & 255), T;
        }, E.prototype.array = E.prototype.digest, E.prototype.arrayBuffer = function() {
          this.finalize();
          var h = new ArrayBuffer(this.is224 ? 28 : 32), w = new DataView(h);
          return w.setUint32(0, this.h0), w.setUint32(4, this.h1), w.setUint32(8, this.h2), w.setUint32(12, this.h3), w.setUint32(16, this.h4), w.setUint32(20, this.h5), w.setUint32(24, this.h6), this.is224 || w.setUint32(28, this.h7), h;
        };
        function N(h, w, R) {
          var _, A = typeof h;
          if (A === "string") {
            var v = [], P = h.length, b = 0, T;
            for (_ = 0; _ < P; ++_) T = h.charCodeAt(_), T < 128 ? v[b++] = T : T < 2048 ? (v[b++] = 192 | T >>> 6, v[b++] = 128 | T & 63) : T < 55296 || T >= 57344 ? (v[b++] = 224 | T >>> 12, v[b++] = 128 | T >>> 6 & 63, v[b++] = 128 | T & 63) : (T = 65536 + ((T & 1023) << 10 | h.charCodeAt(++_) & 1023), v[b++] = 240 | T >>> 18, v[b++] = 128 | T >>> 12 & 63, v[b++] = 128 | T >>> 6 & 63, v[b++] = 128 | T & 63);
            h = v;
          } else if (A === "object") {
            if (h === null) throw new Error(t);
            if (l && h.constructor === ArrayBuffer) h = new Uint8Array(h);
            else if (!Array.isArray(h) && (!l || !ArrayBuffer.isView(h))) throw new Error(t);
          } else throw new Error(t);
          h.length > 64 && (h = new E(w, true).update(h).array());
          var j = [], M = [];
          for (_ = 0; _ < 64; ++_) {
            var W = h[_] || 0;
            j[_] = 92 ^ W, M[_] = 54 ^ W;
          }
          E.call(this, w, R), this.update(M), this.oKeyPad = j, this.inner = true, this.sharedMemory = R;
        }
        N.prototype = new E(), N.prototype.finalize = function() {
          if (E.prototype.finalize.call(this), this.inner) {
            this.inner = false;
            var h = this.array();
            E.call(this, this.is224, this.sharedMemory), this.update(this.oKeyPad), this.update(h), E.prototype.finalize.call(this);
          }
        };
        var B = x();
        B.sha256 = B, B.sha224 = x(true), B.sha256.hmac = U(), B.sha224.hmac = U(true), i ? e.exports = B : (r.sha256 = B.sha256, r.sha224 = B.sha224);
      })();
    })(nt)), nt.exports;
  }
  var fo = uo();
  async function Wt(e) {
    var _a;
    const t = (_a = globalThis.crypto) == null ? void 0 : _a.subtle;
    if (t && typeof t.digest == "function") {
      const n = await t.digest("SHA-256", e);
      return Array.from(new Uint8Array(n)).map((r) => r.toString(16).padStart(2, "0")).join("");
    }
    return fo.sha256(e);
  }
  const ho = async (e, t, n) => {
    const r = tn(), s = 8 * 1024 * 1024;
    if (en("\u4E0A\u4F20\u8BB0\u5F55", Oe(nn)), !t) return;
    let o = [];
    for (let y of t) o.push({
      file: y,
      totalChunks: Math.ceil(y.size / s),
      sha256: await Wt(await mo(y))
    });
    let i = 0;
    console.log("@files", o), r.addMany(o.map((y) => ({
      name: y.file.name,
      size: y.file.size,
      sha256: y.totalChunks === 0 ? y.sha256 + i++ : y.sha256,
      data: {
        chunkCount: 0,
        sha256: y.sha256
      },
      progress: y.totalChunks === 0 ? 100 : 0
    })));
    const l = await po(e, o, s);
    if (!l) return null;
    const [a, u] = l;
    if (!a || !u) return;
    const { uploadId: c, maxConcurrency: d, dedup: m } = a, S = await yo(c, o);
    if (!S) return;
    let p = [], x = /* @__PURE__ */ new Map();
    for (let y of S) {
      if (y.exists) continue;
      let C = o.find((E) => E.file.webkitRelativePath === y.path) ?? null;
      if (!C) continue;
      let U = [];
      for (let E = 0; E < C.totalChunks; E++) {
        let N = x.get(y.path) ?? 0, B = (C == null ? void 0 : C.file.slice(N * s, (N + 1) * s)) ?? null;
        U.push({
          path: y.path,
          chunkIndex: N,
          chunk: B,
          sha256: B ? await Wt(new Uint8Array(await B.arrayBuffer())) : "",
          orig: C,
          range: `${N * s}-${Math.min((N + 1) * s - 1, C.file.size - 1)}`
        }), x.set(y.path, N + 1);
      }
      p.push(U);
    }
    await bo(c, p, d, u, {
      onEachCompleted: (y, C, U, E) => {
        U.status === "fulfilled" && E && E.orig && E.orig.totalChunks && r.update(E.orig.sha256, (N) => (N.data.chunkCount ? N.data.chunkCount += 1 : N.data.chunkCount = 1, {
          ...N,
          progress: N.data.chunkCount / E.orig.totalChunks * 100
        }));
      },
      onAllComplete: async () => {
        o.forEach((y) => {
          r.update(y.sha256, 100);
        });
      }
    });
  }, po = async (e, t, n) => {
    const r = t.reduce((l, a) => l + a.file.size, 0), s = {
      directoryName: t[0].file.webkitRelativePath.split("/")[0],
      totalFiles: t.length,
      chunkSize: n,
      totalChunks: t.reduce((l, a) => l + a.totalChunks, 0),
      totalBytes: r,
      parentPath: e
    }, o = {
      parentPath: e,
      totalFiles: t.length,
      chunkSize: n,
      totalChunks: t.reduce((l, a) => l + a.totalChunks, 0),
      totalBytes: r
    }, i = de();
    try {
      const l = await i.post("/directory/directoryInfo", s);
      return l.data.status !== 200 ? ((await ce()).error(l.data.message), null) : [
        l.data.data,
        o
      ];
    } catch (l) {
      (await ce()).error(l.message);
    }
  };
  async function mo(e) {
    const t = await e.arrayBuffer();
    return new Uint8Array(t);
  }
  async function yo(e, t) {
    const n = de();
    try {
      const r = await n.post("/directory/filesInfo?uploadId=" + e, {
        entries: t.map((s) => ({
          path: s.file.webkitRelativePath,
          totalChunks: s.totalChunks,
          sha256: s.sha256,
          size: s.file.size,
          mtime: s.file.lastModified
        }))
      });
      return r.data.status !== 200 ? null : r.data.data;
    } catch (r) {
      return (await ce()).error(r.message), null;
    }
  }
  async function bo(e, t, n, r, s) {
    async function o(a) {
      if (!a.chunk) return;
      const u = {
        "Content-Type": "application/octet-stream",
        "X-File-Path": encodeURIComponent(a.path),
        "Content-sha256": a.sha256,
        "X-Total-Parts": a.orig.totalChunks,
        "Content-Range": a.range
      };
      return l.put(`/directory/${e}/chunks/${a.chunkIndex}`, a.chunk, {
        headers: u
      });
    }
    async function i(a, u = 5) {
      let c;
      for (let d = 0; d < u; d++) try {
        return await a();
      } catch (m) {
        c = m, console.warn(`Chunk upload failed (try ${d + 1}/${u})`, m);
      }
      throw c;
    }
    const l = de();
    try {
      await ao(t.map((a) => a.map((u) => () => i(() => o(u)))), n, async (a, u) => {
        (s == null ? void 0 : s.onChunkUploadProgress) && s.onChunkUploadProgress(a, u);
        let c = t[a][0].orig;
        if (u.every((d) => d.status === "fulfilled")) try {
          const d = await l.post(`/directory/${e}/chunks/complete`, {
            filePath: c.file.webkitRelativePath,
            totalParts: c.totalChunks,
            fileSize: c.file.size
          });
          d.data.status !== 200 && ce().then((m) => m.error(d.data.message));
        } catch (d) {
          ce().then((m) => m.error(d.message));
        }
      }, async (a) => {
        if ((s == null ? void 0 : s.onAllComplete) && s.onAllComplete(a), a.every((u) => u.every((c) => c.status === "fulfilled"))) try {
          const u = await l.post(`/directory/${e}/complete`, r);
          u.data.status !== 200 ? ce().then((c) => c.error(u.data.message)) : ce().then((c) => c.success("\u76EE\u5F55\u4E0A\u4F20\u5B8C\u6210"));
        } catch (u) {
          ce().then((c) => c.error(u.message));
        }
      }, async (a, u, c) => {
        (s == null ? void 0 : s.onEachCompleted) && s.onEachCompleted(a, u, c, {
          chunkIndex: t[a][u].chunkIndex,
          path: t[a][u].path,
          orig: t[a][u].orig,
          range: t[a][u].range,
          sha256: t[a][u].sha256
        });
      });
    } catch (a) {
      return (await ce()).error(a.message), null;
    }
  }
  const go = (e) => {
    const t = ft();
    if (console.log(e), e.data.status === 200) return e.data.message && t.success(e.data.message), e.data;
    throw t.error(e.data.message || "\u8BF7\u6C42\u5931\u8D25"), new Error(e.data.message || "\u8BF7\u6C42\u5931\u8D25");
  }, Ae = (e) => {
    var _a, _b;
    try {
      return go(e);
    } catch (t) {
      const n = ft();
      let r = "\u7F51\u7EDC\u9519\u8BEF";
      if (t.response) {
        const s = t.response.status;
        switch (s) {
          case 401:
            r = "\u672A\u6388\u6743\u8BBF\u95EE\uFF0C\u8BF7\u91CD\u65B0\u767B\u5F55";
            break;
          case 403:
            r = "\u6743\u9650\u4E0D\u8DB3\uFF0C\u65E0\u6CD5\u6267\u884C\u6B64\u64CD\u4F5C";
            break;
          case 404:
            r = "\u8BF7\u6C42\u7684\u8D44\u6E90\u4E0D\u5B58\u5728";
            break;
          case 500:
            r = "\u670D\u52A1\u5668\u5185\u90E8\u9519\u8BEF";
            break;
          default:
            r = ((_b = (_a = t.response) == null ? void 0 : _a.data) == null ? void 0 : _b.message) || `\u8BF7\u6C42\u5931\u8D25 (${s})`;
        }
      } else t.message && (r = t.message);
      throw n.error(r), t;
    }
  }, xo = [
    "onClick"
  ], wo = {
    key: 0,
    class: "context-menu-item__icon"
  }, vo = {
    class: "context-menu-item__label"
  }, _o = {
    key: 1,
    class: "context-menu-item__shortcut"
  }, Eo = pe({
    __name: "ContextMenu",
    props: {
      items: {},
      x: {},
      y: {},
      visible: {
        type: Boolean
      }
    },
    emits: [
      "close"
    ],
    setup(e, { emit: t }) {
      const n = e, r = t, s = le(), o = fe(() => ({
        left: `${n.x}px`,
        top: `${n.y}px`,
        position: "fixed",
        zIndex: 9999
      })), i = fe(() => n.items), l = (u) => {
        u.disabled || u.divider || (u.action && u.action(), r("close"));
      }, a = (u) => {
        s.value && !s.value.contains(u.target) && r("close");
      };
      return Xt(() => {
        n.visible && Pn(() => {
          if (document.addEventListener("click", a), s.value) {
            const u = s.value.getBoundingClientRect(), c = window.innerWidth, d = window.innerHeight;
            let m = n.x, S = n.y;
            u.right > c && (m = c - u.width - 10), u.bottom > d && (S = d - u.height - 10), (m !== n.x || S !== n.y) && (s.value.style.left = `${Math.max(0, m)}px`, s.value.style.top = `${Math.max(0, S)}px`);
          }
        });
      }), Yt(() => {
        document.removeEventListener("click", a);
      }), (u, c) => ($(), ct(Zt, {
        to: "body"
      }, [
        e.visible ? ($(), L("div", {
          key: 0,
          ref_key: "menuRef",
          ref: s,
          class: "context-menu",
          style: ut(o.value),
          onClick: c[0] || (c[0] = ye(() => {
          }, [
            "stop"
          ])),
          onContextmenu: c[1] || (c[1] = ye(() => {
          }, [
            "prevent"
          ]))
        }, [
          ($(true), L(re, null, Me(i.value, (d, m) => ($(), L("div", {
            key: m,
            class: he([
              "context-menu-item",
              {
                "context-menu-item--disabled": d.disabled,
                "context-menu-item--divider": d.divider
              }
            ]),
            onClick: (S) => l(d)
          }, [
            d.divider ? G("", true) : ($(), L(re, {
              key: 0
            }, [
              d.icon ? ($(), L("span", wo, z(d.icon), 1)) : G("", true),
              F("span", vo, z(d.label), 1),
              d.shortcut ? ($(), L("span", _o, z(d.shortcut), 1)) : G("", true)
            ], 64))
          ], 10, xo))), 128))
        ], 36)) : G("", true)
      ]));
    }
  }), So = dt(Eo, [
    [
      "__scopeId",
      "data-v-44903102"
    ]
  ]);
  class Fo {
    constructor() {
      __publicField(this, "app", null);
      __publicField(this, "container", null);
    }
    show(t) {
      this.hide(), this.container = document.createElement("div"), document.body.appendChild(this.container), this.app = Qt(So, {
        items: t.items,
        x: t.x,
        y: t.y,
        visible: true,
        onClose: () => {
          this.hide();
        }
      }), this.app.mount(this.container);
    }
    hide() {
      this.app && (this.app.unmount(), this.app = null), this.container && (document.body.removeChild(this.container), this.container = null);
    }
    isVisible() {
      return this.app !== null;
    }
  }
  const Ro = new Fo();
  function Ao(e) {
    Ro.show(e);
  }
  function Co(e, t) {
    e.preventDefault(), e.stopPropagation(), Ao({
      items: t,
      x: e.clientX,
      y: e.clientY
    });
  }
  let Oo, To, ko, Po, No, Bo, Do, $o, Uo, Io, Lo, jo, Mo, zo, Ho, qo, Jo, Vo, Wo, Ko, Go, Xo, Yo, Zo, Qo, ei, ti, ni, ri, si, oi, ii, ai;
  Oo = {
    class: "main-content"
  };
  To = {
    class: "panel"
  };
  ko = {
    class: "panel-head"
  };
  Po = {
    class: "breadcrumb-container"
  };
  No = {
    class: "panel-body"
  };
  Bo = {
    class: "actions"
  };
  Do = [
    "disabled"
  ];
  $o = {
    key: 0,
    class: "upload-progress-container"
  };
  Uo = {
    class: "upload-progress-info"
  };
  Io = {
    class: "upload-file-name"
  };
  Lo = {
    class: "upload-percentage"
  };
  jo = {
    class: "upload-progress-bar"
  };
  Mo = {
    key: 1,
    class: "clipboard-status"
  };
  zo = {
    class: "clipboard-text"
  };
  Ho = {
    key: 2,
    class: "select-all-container"
  };
  qo = {
    class: "select-all-checkbox"
  };
  Jo = [
    "checked",
    "indeterminate"
  ];
  Vo = {
    class: "select-all-text"
  };
  Wo = {
    key: 0
  };
  Ko = {
    class: "file-list"
  };
  Go = {
    key: 0,
    class: "fileListLoadingContainer"
  };
  Xo = [
    "onDblclick",
    "onContextmenu"
  ];
  Yo = {
    key: 0,
    class: "file-checkbox"
  };
  Zo = {
    class: "checkbox-container"
  };
  Qo = [
    "checked",
    "onChange"
  ];
  ei = {
    class: "file-icon"
  };
  ti = {
    class: "file-info"
  };
  ni = {
    class: "file-name"
  };
  ri = {
    class: "file-meta"
  };
  si = {
    class: "file-type"
  };
  oi = {
    key: 0,
    class: "file-size"
  };
  ii = {
    key: 1,
    class: "file-date"
  };
  ai = pe({
    __name: "FileView",
    setup(e) {
      const t = ze(), n = Bn(), r = Gt(), s = Nn(), o = le(false), i = fe(() => r.params.p || []), l = ft(), a = le(/* @__PURE__ */ new Set()), u = le(false), c = le(false), d = le(0), m = le(""), S = (g) => {
        s.push({
          name: "view",
          params: {
            p: g.split("/").filter(Boolean)
          }
        });
      }, p = (g) => {
        s.push({
          name: "view",
          params: {
            p: i.value.concat(g)
          }
        });
      }, x = async () => {
        const g = prompt("\u8F93\u5165\u6587\u4EF6\u5939\u540D");
        if (!g) return;
        const O = await me.createFolder([
          ...i.value,
          g
        ].join("/"));
        Ae(O), l.success(`\u6210\u529F\u521B\u5EFA\u6587\u4EF6\u5939 "${g}"`), await J();
      }, y = async (g) => {
        const O = prompt("\u8F93\u5165\u65B0\u7684\u6587\u4EF6\u540D");
        if (!O) return;
        const I = await me.renameItem(g.path, O);
        Ae(I), l.success(`\u6210\u529F\u91CD\u547D\u540D\u4E3A "${O}"`), await J();
      }, C = async () => {
        const g = document.createElement("input");
        g.type = "file", g.multiple = true, g.onchange = async () => {
          var _a;
          ((_a = g.files) == null ? void 0 : _a.length) && await E(g.files);
        }, g.click();
      }, U = () => {
        const g = document.createElement("input");
        g.type = "file", g.webkitdirectory = true, g.onchange = async () => {
          console.log(g.files, g.webkitEntries), await ho([
            ...i.value
          ].join("/"), g.files), await J();
        }, g.click();
      }, E = async (g) => {
        if (g == null ? void 0 : g.length) {
          if (t.token === "") {
            l.error("\u8BF7\u5148\u767B\u5F55\u540E\u518D\u4E0A\u4F20\u6587\u4EF6");
            return;
          }
          c.value = true, d.value = 0, m.value = g.length === 1 ? g[0].name : `${g.length} \u4E2A\u6587\u4EF6`;
          try {
            await me.uploadFile([
              ...i.value
            ].join("/"), (O) => {
              d.value = O;
            }, ...g).then(Ae), await J(), l.success(`\u6210\u529F\u4E0A\u4F20 ${g.length} \u4E2A\u6587\u4EF6`);
          } catch (O) {
            console.error("\u4E0A\u4F20\u5931\u8D25:", O), l.error("\u6587\u4EF6\u4E0A\u4F20\u5931\u8D25");
          } finally {
            c.value = false, d.value = 0, m.value = "";
          }
        }
      }, N = async ({ files: g }) => {
        g && await E(g);
      }, B = le([]), h = fe(() => B.value.length > 0 && a.value.size === B.value.length), w = fe(() => a.value.size > 0 && a.value.size < B.value.length), R = fe(() => a.value.size), _ = () => {
        u.value = !u.value, u.value || a.value.clear();
      }, A = () => {
        h.value ? (a.value.clear(), l.info("\u5DF2\u53D6\u6D88\u5168\u9009")) : (a.value = new Set(B.value.filter(() => true)), l.info(`\u5DF2\u5168\u9009 ${B.value.length} \u4E2A\u9879\u76EE`));
      }, v = (g) => {
        a.value.has(g) ? a.value.delete(g) : a.value.add(g);
      }, P = () => {
        const g = B.value.filter((O) => a.value.has(O) && O.type === "File");
        if (g.length === 0) {
          l.warning("\u6CA1\u6709\u9009\u4E2D\u53EF\u4E0B\u8F7D\u7684\u6587\u4EF6");
          return;
        }
        g.forEach((O) => {
          j(O);
        }), l.info(`\u5F00\u59CB\u6279\u91CF\u4E0B\u8F7D ${g.length} \u4E2A\u6587\u4EF6`);
      }, b = async () => {
        if (confirm(`\u786E\u5B9A\u8981\u5220\u9664\u9009\u4E2D\u7684 ${R.value} \u4E2A\u9879\u76EE\u5417\uFF1F`)) {
          const g = [];
          for (const I of a.value) g.push(I);
          const O = await me.deleteItem(...g.map((I) => I.path));
          Ae(O), l.success(`\u6210\u529F\u5220\u9664 ${R.value} \u4E2A\u9879\u76EE`), a.value.clear(), await J();
        }
      }, T = async () => {
        const g = await fetch(`${st.apiUrl}/file/getfilelist`, {
          method: "POST",
          headers: {
            "Content-Type": "application/json"
          },
          body: JSON.stringify({
            directoryPath: i.value.join("/")
          })
        }).then((O) => O.json());
        return g.status === 200 ? g.data : [];
      }, j = async (g) => {
        try {
          window.open(`${st.apiUrl}/file/downloadfile?filePathS=${i.value.join("/")}/${g.name}`, "_blank"), l.success(`\u5F00\u59CB\u4E0B\u8F7D "${g.name}"`);
        } catch {
          l.error(`\u4E0B\u8F7D "${g.name}" \u5931\u8D25`);
        }
      }, M = (g) => {
        g.type === "Directory" && p(g.name);
      }, W = async (g) => {
        if (confirm(`\u786E\u5B9A\u8981\u5220\u9664 ${g.name} \u5417\uFF1F`)) {
          const O = await me.deleteItem(g.path);
          Ae(O), l.success(`\u6210\u529F\u5220\u9664 "${g.name}"`), await J();
        }
      }, V = (g, O) => {
        g.preventDefault(), g.stopPropagation();
        const I = [];
        u.value && a.value.size > 1 ? (Array.from(a.value).filter((ae) => ae.type === "File").length > 0 && I.push({
          label: "\u6279\u91CF\u4E0B\u8F7D(\u4EC5\u6587\u4EF6)",
          action: P
        }), t.isLogin && I.push({
          label: `\u6279\u91CF\u5220\u9664 (${a.value.size}\u9879)`,
          action: b
        }, {
          label: `\u590D\u5236 (${a.value.size}\u9879)`,
          action: ie
        }, {
          label: `\u526A\u5207 (${a.value.size}\u9879)`,
          action: gt
        })) : (O.type === "Directory" ? I.push({
          label: "\u8FDB\u5165",
          action: () => p(O.name)
        }) : I.push({
          label: "\u4E0B\u8F7D",
          action: () => j(O)
        }), t.isLogin && I.push({
          label: "\u5220\u9664",
          action: () => {
            W(O);
          }
        }, {
          label: "\u91CD\u547D\u540D",
          action: () => {
            y(O);
          }
        }, {
          label: "\u590D\u5236",
          action: () => oe([
            O
          ])
        }, {
          label: "\u526A\u5207",
          action: () => ne([
            O
          ])
        })), Co(g, I);
      };
      let D = 0;
      const J = async (g = false) => {
        D++;
        const O = D;
        o.value = true, B.value = [], a.value.clear();
        try {
          const I = await T();
          if (O !== D) return;
          o.value = false, B.value = I, g && O === D && l.info("\u6587\u4EF6\u5217\u8868\u5DF2\u5237\u65B0");
        } catch {
          O === D && (o.value = false, g && l.error("\u5237\u65B0\u6587\u4EF6\u5217\u8868\u5931\u8D25"));
        }
      }, H = fe(() => n.clipboard.items.length > 0), Z = fe(() => n.clipboard.items.length === 0 ? "" : ` (${n.clipboard.items.length}\u9879)`), oe = (g) => {
        n.copy(g), l.success(`\u5DF2\u590D\u5236 ${g.length} \u4E2A\u9879\u76EE`);
      }, ne = (g) => {
        n.cut(g), l.success(`\u5DF2\u526A\u5207 ${g.length} \u4E2A\u9879\u76EE`);
      }, ie = () => {
        const g = Array.from(a.value);
        oe(g);
      }, gt = () => {
        const g = Array.from(a.value);
        ne(g);
      }, Rn = async () => {
        var _a, _b;
        if (!H.value) return;
        const { items: g, mode: O } = n.clipboard, I = i.value.join("/");
        try {
          let K;
          const ae = O === "copy" ? "\u590D\u5236" : "\u79FB\u52A8";
          if (l.info(`\u6B63\u5728${ae} ${g.length} \u4E2A\u9879\u76EE...`), O === "copy" ? K = await me.copyFiles(I, g) : K = await me.moveFiles(I, g), K.data.status === 200) {
            if (K.data.data && Array.isArray(K.data.data) && K.data.data.length > 0) {
              const ve = K.data.data.filter((De) => De.status !== 200), xt = g.length - ve.length;
              if (ve.length > 0) {
                const De = ve.map((Cn) => Cn.data).join("\u3001");
                xt > 0 ? l.warning(`\u90E8\u5206${ae}\u6210\u529F\uFF1A\u6210\u529F${xt}\u4E2A\uFF0C\u5931\u8D25${ve.length}\u4E2A\u3002\u5931\u8D25\u7684\u6587\u4EF6\uFF1A${De}`) : l.error(`${ae}\u5931\u8D25\uFF1A${De}`);
              } else l.success(`\u6210\u529F${ae} ${g.length} \u4E2A\u9879\u76EE`);
            } else l.success(K.data.message || `\u6210\u529F${ae} ${g.length} \u4E2A\u9879\u76EE`);
            O === "cut" && n.clear();
          } else l.error(K.data.message || `${ae}\u5931\u8D25`);
          await J();
        } catch (K) {
          const ae = O === "copy" ? "\u590D\u5236" : "\u79FB\u52A8", ve = ((_b = (_a = K.response) == null ? void 0 : _a.data) == null ? void 0 : _b.message) || K.message || "\u7F51\u7EDC\u9519\u8BEF";
          l.error(`${ae}\u5931\u8D25: ${ve}`);
        }
      }, An = () => {
        n.clear(), l.info("\u5DF2\u6E05\u7A7A\u526A\u8D34\u677F");
      };
      return vt(() => n.clipboard, (g, O) => {
        if (O && (g.items.length !== O.items.length || g.mode !== O.mode)) if (g.items.length > 0) {
          const I = g.mode === "copy" ? "\u590D\u5236" : "\u526A\u5207";
          l.info(`\u526A\u8D34\u677F\u5DF2\u66F4\u65B0\uFF1A${I} ${g.items.length} \u4E2A\u9879\u76EE`);
        } else l.info("\u526A\u8D34\u677F\u5DF2\u6E05\u7A7A");
      }, {
        deep: true
      }), vt(i, (g) => {
        J();
      }, {
        immediate: true
      }), (g, O) => ($(), ct(nr, {
        onDrop: N
      }, {
        default: ge(() => [
          ue(yr),
          F("main", Oo, [
            F("div", To, [
              F("div", ko, [
                F("div", Po, [
                  ue(er, {
                    onNavigate: S
                  })
                ])
              ]),
              F("div", No, [
                F("div", Bo, [
                  Y(t).token !== "" ? ($(), L(re, {
                    key: 0
                  }, [
                    F("button", {
                      onClick: x,
                      class: "btn btn-primary"
                    }, "\u65B0\u5EFA\u6587\u4EF6\u5939"),
                    F("button", {
                      onClick: C,
                      class: "btn"
                    }, "\u4E0A\u4F20\u6587\u4EF6"),
                    F("button", {
                      onClick: U,
                      class: "btn"
                    }, "\u4E0A\u4F20\u6587\u4EF6\u5939")
                  ], 64)) : G("", true),
                  F("button", {
                    onClick: O[0] || (O[0] = () => J(true)),
                    class: "btn"
                  }, "\u5237\u65B0"),
                  F("button", {
                    onClick: _,
                    class: he([
                      "btn",
                      {
                        "btn-primary": u.value
                      }
                    ])
                  }, z(u.value ? "\u53D6\u6D88\u9009\u62E9" : "\u591A\u9009\u6A21\u5F0F"), 3),
                  Y(t).isLogin ? ($(), L("button", {
                    key: 1,
                    onClick: Rn,
                    class: he([
                      "btn",
                      {
                        "btn-success": H.value
                      }
                    ]),
                    disabled: !H.value
                  }, "\u7C98\u8D34" + z(Z.value), 11, Do)) : G("", true),
                  u.value && R.value > 0 ? ($(), L(re, {
                    key: 2
                  }, [
                    F("button", {
                      onClick: P,
                      class: "btn btn-success"
                    }, "\u6279\u91CF\u4E0B\u8F7D (" + z(R.value) + ")", 1),
                    Y(t).isLogin ? ($(), L(re, {
                      key: 0
                    }, [
                      F("button", {
                        onClick: b,
                        class: "btn btn-danger"
                      }, "\u6279\u91CF\u5220\u9664 (" + z(R.value) + ")", 1),
                      F("button", {
                        onClick: ie,
                        class: "btn btn-info"
                      }, "\u590D\u5236 (" + z(R.value) + ")", 1),
                      F("button", {
                        onClick: gt,
                        class: "btn btn-warning"
                      }, "\u526A\u5207 (" + z(R.value) + ")", 1)
                    ], 64)) : G("", true)
                  ], 64)) : G("", true)
                ]),
                c.value ? ($(), L("div", $o, [
                  F("div", Uo, [
                    F("span", Io, "\u6B63\u5728\u4E0A\u4F20: " + z(m.value), 1),
                    F("span", Lo, z(d.value) + "%", 1)
                  ]),
                  F("div", jo, [
                    F("div", {
                      class: "upload-progress-fill",
                      style: ut({
                        width: d.value + "%"
                      })
                    }, null, 4)
                  ])
                ])) : G("", true),
                Y(t).isLogin && Y(n).clipboard.items.length > 0 ? ($(), L("div", Mo, [
                  O[2] || (O[2] = F("span", {
                    class: "clipboard-icon"
                  }, "\u{1F4CB}", -1)),
                  F("span", zo, " \u526A\u8D34\u677F\u4E2D\u6709 " + z(Y(n).clipboard.items.length) + " \u4E2A\u9879\u76EE (" + z(Y(n).clipboard.mode === "copy" ? "\u590D\u5236" : "\u526A\u5207") + ") ", 1),
                  F("button", {
                    onClick: An,
                    class: "btn-clear-clipboard"
                  }, "\u6E05\u7A7A")
                ])) : G("", true),
                u.value && B.value.length > 0 ? ($(), L("div", Ho, [
                  F("label", qo, [
                    F("input", {
                      type: "checkbox",
                      checked: h.value,
                      indeterminate: w.value,
                      onChange: A
                    }, null, 40, Jo),
                    O[3] || (O[3] = F("span", {
                      class: "checkmark"
                    }, null, -1)),
                    F("span", Vo, [
                      Ce(z(h.value ? "\u53D6\u6D88\u5168\u9009" : "\u5168\u9009") + " ", 1),
                      R.value > 0 ? ($(), L("span", Wo, "(\u5DF2\u9009\u62E9 " + z(R.value) + " \u9879)", 1)) : G("", true)
                    ])
                  ])
                ])) : G("", true),
                F("div", Ko, [
                  o.value ? ($(), L("div", Go, [
                    ...O[4] || (O[4] = [
                      F("div", {
                        class: "loading-pulse"
                      }, null, -1)
                    ])
                  ])) : ($(true), L(re, {
                    key: 1
                  }, Me(B.value, (I) => ($(), L("div", {
                    class: he([
                      "file-item",
                      {
                        selected: u.value && a.value.has(I)
                      }
                    ]),
                    onDblclick: (K) => M(I),
                    onContextmenu: (K) => V(K, I),
                    key: I.name
                  }, [
                    u.value ? ($(), L("div", Yo, [
                      F("label", Zo, [
                        F("input", {
                          type: "checkbox",
                          checked: a.value.has(I),
                          onChange: (K) => v(I),
                          onClick: O[1] || (O[1] = ye(() => {
                          }, [
                            "stop"
                          ]))
                        }, null, 40, Qo),
                        O[5] || (O[5] = F("span", {
                          class: "checkmark"
                        }, null, -1))
                      ])
                    ])) : G("", true),
                    F("div", ei, z(I.type === "Directory" ? "\u{1F4C1}" : "\u{1F4C4}"), 1),
                    F("div", ti, [
                      F("div", ni, z(I.name), 1),
                      F("div", ri, [
                        F("span", si, z(I.type === "Directory" ? "\u6587\u4EF6\u5939" : "\u6587\u4EF6"), 1),
                        I.type === "File" ? ($(), L("span", oi, z(Y(Yn)(I.size, {
                          base: 2
                        })), 1)) : G("", true),
                        I.type === "File" ? ($(), L("span", ii, z(new Date(I.lastModified).toLocaleString()), 1)) : G("", true)
                      ])
                    ])
                  ], 42, Xo))), 128))
                ])
              ])
            ])
          ]),
          ue(wr)
        ]),
        _: 1
      }));
    }
  });
  Oi = dt(ai, [
    [
      "__scopeId",
      "data-v-b1feb240"
    ]
  ]);
});
export {
  __tla,
  Oi as default
};
