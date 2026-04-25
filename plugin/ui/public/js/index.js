import * as Juce from "./juce/index.js";
import { Knob } from "./knob.js";

// === Live debug overlay — surfaces JS errors right inside the plugin UI ===
function showError(msg) {
  let log = document.getElementById("debug-log");
  if (!log) {
    log = document.createElement("div");
    log.id = "debug-log";
    document.body.appendChild(log);
  }
  log.textContent += (log.textContent ? "\n" : "") + msg;
  log.style.display = "block";
}
window.addEventListener("error", (e) => {
  showError(`[ERR] ${e.message} @ ${e.filename}:${e.lineno}:${e.colno}`);
});
window.addEventListener("unhandledrejection", (e) => {
  showError(`[REJECT] ${e.reason}`);
});

// === Initialisation data ===
const initData = window.__JUCE__.initialisationData;
document.getElementById("vendor").innerText = initData.vendor;
document.getElementById("pluginName").innerText = initData.pluginName;
document.getElementById("pluginVersion").innerText = initData.pluginVersion;

const loadSofaFn = Juce.getNativeFunction("loadSofa");
const unloadSofaFn = Juce.getNativeFunction("unloadSofa");

// === Value formatters ===
const FORMATTERS = {
  ms:  (v) => `${v.toFixed(1)} ms`,
  hz:  (v) => `${v.toFixed(1)} Hz`,
  st:  (v) => `${v.toFixed(2)} st`,
  deg: (v) => `${v.toFixed(0)}°`,
  lin: (v) => v.toFixed(2),
  db:  (v) => (v <= 0.0001 ? "-inf dB" : `${(20 * Math.log10(v)).toFixed(1)} dB`),
};

// === Toggle binding ===
function bindToggle(domId, paramId) {
  const checkbox = document.getElementById(domId);
  const state = Juce.getToggleState(paramId);
  const refresh = () => { checkbox.checked = state.getValue(); };
  checkbox.addEventListener("input", () => state.setValue(checkbox.checked));
  state.valueChangedEvent.addListener(refresh);
  refresh();
}

// === Boot ===
function boot() {
  try {
    document.querySelectorAll(".knob-wrap").forEach((node) => {
      const param = node.dataset.param;
      const fmtKey = node.dataset.format || "lin";
      const label = node.dataset.label || param;
      const fmt = FORMATTERS[fmtKey] || FORMATTERS.lin;
      const state = Juce.getSliderState(param);
      new Knob(node, state, fmt, label);
    });

    bindToggle("freeze", "FREEZE");
    bindToggle("bypass", "BYPASS");

    document.getElementById("loadSofaButton").addEventListener("click", () => {
      loadSofaFn().catch((e) => showError(`[loadSofa] ${e}`));
    });
    document.getElementById("unloadSofaButton").addEventListener("click", () => {
      unloadSofaFn().catch((e) => showError(`[unloadSofa] ${e}`));
    });

    const hrtfDot = document.getElementById("hrtfDot");
    const hrtfStatus = document.getElementById("hrtfStatus");
    const activeGrainsEl = document.getElementById("activeGrains");

    window.__JUCE__.backend.addEventListener("grainStats", () => {
      fetch(Juce.getBackendResourceAddress("grainStats.json"))
        .then((r) => r.json())
        .then((s) => {
          activeGrainsEl.innerText = s.activeGrains;
          if (s.hrtfLoaded) {
            hrtfDot.classList.add("on");
            const name = s.sofaName ? s.sofaName.split(/[\\/]/).pop() : "sofa";
            hrtfStatus.innerText = `HRTF // ${name} · ${s.filterLength} taps`;
          } else {
            hrtfDot.classList.remove("on");
            hrtfStatus.innerText = "No SOFA — ITD/ILD fallback";
          }
        })
        .catch(() => {});
    });
  } catch (e) {
    showError(`[boot] ${e.message}\n${e.stack}`);
  }
}

if (document.readyState === "loading") {
  document.addEventListener("DOMContentLoaded", boot);
} else {
  boot();
}
