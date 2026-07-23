for (const [note, velocity] of midiInput) {
    arr[note % 12] += 0.5 * velocity
}
