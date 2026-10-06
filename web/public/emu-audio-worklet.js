/**
 * EmuAudio is an AudioWorkletProcessor that handles audio data for the emulator. It maintains a circular buffer to store incoming audio samples and processes them for playback. The processor receives audio samples from the main thread via messages and outputs them to the audio context, ensuring smooth audio playback even under varying load conditions.
 */
class EmuAudio extends AudioWorkletProcessor {
  /**
   * Initializes the EmuAudio processor, setting up the circular buffer and message handling for incoming audio samples. The constructor creates a Float32Array buffer, initializes read/write pointers, and sets up a message event listener to receive audio data from the main thread.
   */
  constructor() {
    super();
    this.buf = new Float32Array(1 << 17);
    this.mask = this.buf.length - 1;
    this.r = 0;
    this.w = 0;
    this.count = 0;
    this.tick = 0;

    this.port.onmessage = (e) => {
      const s = e.data;
      for (let i = 0; i < s.length; i++) {
        if (this.count >= this.buf.length) break;
        this.buf[this.w] = s[i];
        this.w = (this.w + 1) & this.mask;
        this.count++;
      }
    };
  }

  /**
   * Processes audio samples for playback. This method is called by the audio context to fill the output buffer with audio data. It reads samples from the circular buffer and writes them to the left and right output channels, handling cases where there are insufficient samples available by outputting silence.
   * @param {*} _inputs
   * @param {*} outputs
   * @returns A boolean indicating whether the processor should continue processing audio.
   */
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
        left[i] = right[i] = 0;
      }
    }
    if ((this.tick++ & 15) === 0) this.port.postMessage(this.count / 2);
    return true;
  }
}
registerProcessor('emu-audio', EmuAudio);
