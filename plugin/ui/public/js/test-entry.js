// Browser-only test entry point. Mounts the same UI without depending
// on the real JUCE WebView module — fakes a JUCE slider state and
// drives the same Knob class.
import { Knob } from "./knob.js";

function makeFakeSliderState(initial, scale) {
  const subs = [];
  const propSubs = [];
  const s = {
    _v: initial,
    valueChangedEvent: { addListener: (cb) => subs.push(cb) },
    propertiesChangedEvent: { addListener: (cb) => propSubs.push(cb) },
    getNormalisedValue() { return s._v; },
    getScaledValue() { return scale(s._v); },
    setNormalisedValue(x) {
      s._v = Math.max(0, Math.min(1, x));
      subs.forEach((cb) => cb());
    },
  };
  return s;
}

function makeFakeToggleState(initial) {
  const subs = [];
  const s = {
    _v: !!initial,
    valueChangedEvent: { addListener: (cb) => subs.push(cb) },
    propertiesChangedEvent: { addListener: () => {} },
    getValue() { return s._v; },
    setValue(x) { s._v = !!x; subs.forEach((cb) => cb()); },
  };
  return s;
}

const FORMATTERS = {
  ms:  (v) => `${v.toFixed(1)} ms`,
  hz:  (v) => `${v.toFixed(1)} Hz`,
  st:  (v) => `${v.toFixed(2)} st`,
  deg: (v) => `${v.toFixed(0)}°`,
  lin: (v) => v.toFixed(2),
  db:  (v) => (v <= 0.0001 ? "-inf dB" : `${(20 * Math.log10(v)).toFixed(1)} dB`),
};

const PARAMS = {
  GAIN:          { initial: 0.5,  scale: (v) => v * 2 },
  DRY_WET:       { initial: 0.7,  scale: (v) => v },
  GRAIN_SIZE:    { initial: 0.3,  scale: (v) => 5 + v * 495 },
  DENSITY:       { initial: 0.2,  scale: (v) => 1 + v * 199 },
  PITCH:         { initial: 0.5,  scale: (v) => -24 + v * 48 },
  PITCH_RAND:    { initial: 0.0,  scale: (v) => v * 12 },
  POSITION:      { initial: 0.05, scale: (v) => v * 2000 },
  POSITION_RAND: { initial: 0.05, scale: (v) => v * 1000 },
  SPREAD:        { initial: 0.5,  scale: (v) => v * 180 },
};

const TOGGLES = ["FREEZE", "BYPASS"];

function boot() {
  document.getElementById("vendor").innerText = "Borato Company (test)";
  document.getElementById("pluginName").innerText = "Picotado";
  document.getElementById("pluginVersion").innerText = "0.1.0";

  document.querySelectorAll(".knob-wrap").forEach((node) => {
    const id = node.dataset.param;
    const cfg = PARAMS[id];
    if (!cfg) return;
    const state = makeFakeSliderState(cfg.initial, cfg.scale);
    const fmt = FORMATTERS[node.dataset.format] || FORMATTERS.lin;
    new Knob(node, state, fmt, node.dataset.label || id);
  });

  TOGGLES.forEach((id) => {
    const el = document.getElementById(id.toLowerCase());
    if (!el) return;
    const state = makeFakeToggleState(false);
    el.addEventListener("input", () => state.setValue(el.checked));
    state.valueChangedEvent.addListener(() => { el.checked = state.getValue(); });
  });

  document.getElementById("loadSofaButton").addEventListener("click", () => {
    document.getElementById("hrtfStatus").innerText = "(test) Load SOFA clicked";
  });
  document.getElementById("unloadSofaButton").addEventListener("click", () => {
    document.getElementById("hrtfStatus").innerText = "(test) Cleared";
  });
}

if (document.readyState === "loading") {
  document.addEventListener("DOMContentLoaded", boot);
} else {
  boot();
}
