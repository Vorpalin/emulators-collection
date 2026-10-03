// AudioWorklet : lit un ring buffer de PCM stéréo entrelacé (L,R,L,R...)
// alimenté par le thread principal (postMessage de Float32Array).
// Toutes les ~43 ms il renvoie le nombre de frames en attente : c'est
// l'horloge audio qui pilote la vitesse de l'émulation.
class EmuAudio extends AudioWorkletProcessor {
  constructor() {
    super();
    this.buf = new Float32Array(1 << 17); // puissance de 2 (~1.4 s à 48 kHz)
    this.mask = this.buf.length - 1;
    this.r = 0;
    this.w = 0;
    this.count = 0; // floats en attente (2 par frame stéréo)
    this.tick = 0;

    this.port.onmessage = (e) => {
      const s = e.data;
      for (let i = 0; i < s.length; i++) {
        if (this.count >= this.buf.length) break; // overflow : on jette
        this.buf[this.w] = s[i];
        this.w = (this.w + 1) & this.mask;
        this.count++;
      }
    };
  }

  process(_inputs, outputs) {
    const out = outputs[0];
    const left = out[0];
    const right = out[1] || out[0];
    for (let i = 0; i < left.length; i++) {
      if (this.count >= 2) {
        left[i] = this.buf[this.r];
        right[i] = this.buf[(this.r + 1) & this.mask];
        this.r = (this.r + 2) & this.mask;
        this.count -= 2;
      } else {
        left[i] = right[i] = 0; // underrun : silence
      }
    }
    if ((this.tick++ & 15) === 0) this.port.postMessage(this.count / 2);
    return true;
  }
}
registerProcessor("emu-audio", EmuAudio);
