// Synthwave knob — pure CSS + transform: rotate. No SVG, no deps.
//
// Layout: a circular .knob (the body) contains a .knob-rotor div whose
// rotation drives the indicator tick. The rotor is rotated via inline
// `transform: rotate(Xdeg)` from -135° (min) to +135° (max). Drag
// vertical / scroll wheel / dbl-click are handled here.

export class Knob {
  /**
   * @param container  HTMLElement — receives knob/value/label nodes
   * @param state      JUCE slider state (getNormalisedValue / setNormalisedValue / getScaledValue / events)
   * @param formatter  fn(scaledValue) -> displayable string
   * @param labelText  string above the value
   */
  constructor(container, state, formatter, labelText) {
    this.state = state;
    this.format = formatter;

    container.innerHTML = `
      <div class="knob" tabindex="0">
        <div class="knob-rotor"><div class="knob-tick"></div></div>
        <div class="knob-dot"></div>
      </div>
      <div class="knob-value"></div>
      <div class="knob-label">${labelText}</div>
    `;
    this.knobEl = container.querySelector(".knob");
    this.rotorEl = container.querySelector(".knob-rotor");
    this.valueEl = container.querySelector(".knob-value");

    this._bindInteractions();

    this.state.valueChangedEvent.addListener(() => this._refresh());
    this.state.propertiesChangedEvent.addListener(() => this._refresh());
    this._refresh();
  }

  _bindInteractions() {
    let dragStartY = 0;
    let dragStartValue = 0;

    this.knobEl.addEventListener("pointerdown", (e) => {
      this.knobEl.setPointerCapture(e.pointerId);
      dragStartY = e.clientY;
      dragStartValue = this.state.getNormalisedValue();
      this.knobEl.classList.add("dragging");
      e.preventDefault();
    });

    this.knobEl.addEventListener("pointermove", (e) => {
      if (!this.knobEl.hasPointerCapture(e.pointerId)) return;
      const dy = dragStartY - e.clientY;
      const speed = e.shiftKey ? 0.0006 : 0.0038;
      const v = clamp(dragStartValue + dy * speed, 0, 1);
      this.state.setNormalisedValue(v);
    });

    const release = (e) => {
      if (this.knobEl.hasPointerCapture(e.pointerId))
        this.knobEl.releasePointerCapture(e.pointerId);
      this.knobEl.classList.remove("dragging");
    };
    this.knobEl.addEventListener("pointerup", release);
    this.knobEl.addEventListener("pointercancel", release);

    this.knobEl.addEventListener(
      "wheel",
      (e) => {
        e.preventDefault();
        const delta = -Math.sign(e.deltaY) * (e.shiftKey ? 0.001 : 0.015);
        this.state.setNormalisedValue(
          clamp(this.state.getNormalisedValue() + delta, 0, 1)
        );
      },
      { passive: false }
    );

    this.knobEl.addEventListener("dblclick", (e) => {
      e.preventDefault();
      this.state.setNormalisedValue(0.5);
    });
  }

  _refresh() {
    const norm = this.state.getNormalisedValue();
    const angle = -135 + 270 * norm;
    this.rotorEl.style.transform = `rotate(${angle}deg)`;
    this.valueEl.textContent = this.format(this.state.getScaledValue());
  }
}

function clamp(v, lo, hi) {
  return Math.max(lo, Math.min(hi, v));
}
