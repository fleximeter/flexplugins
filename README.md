# flexplugins
A collection of SuperCollider plugins developed by Jeff Martin (www.jeffreymartincomposer.com).

## List of Plugins

### PV_PlayBufStretch
A phase vocoder STFT buffer player for time stretching, for use with PV_RecordBuf in the sc3plugins distribution. It incorporates two different optional phase locking algorithms.

### PV_CFreeze
A SuperCollider implementation of Jean-François Charles' Max patch for freezing [(the Max patch is available here)](https://newfloremusic.gumroad.com/).
It is a phase vocoder spectral freeze with an innovation that makes the frozen sound less stagnant.

### PV_MagMirror
Mirrors spectral magnitudes, so that high magnitudes become low and low magnitudes become high.

### PV_MagSqueeze
Squeezes spectral magnitudes to fit between (low, high).

### PV_MagXFade
Equal power crossfade between the magnitudes of two FFT buffers.

### RubberBandPS
A formant-preserving phase vocoder pitch shifter using the [RubberBand library](https://breakfastquay.com/rubberband/).

### RubberBandStretcher
A phase vocoder pitch shifter and time stretcher using the [RubberBand library](https://breakfastquay.com/rubberband/).

### ImpulseDropout
A modified version of Impulse that randomly drops a percentage of impulses, producing a stuttering effect.

### ImpulseJitter
A modified version of Impulse that allows the impulses to be shifted randomly in time. It allows a steady move between regular and chaotic impulses, which can be useful for event triggers.

### LoopPhasor
An adaptation of Phasor, which allows you to set loop start and stop points. It is useful for playing audio samples for which you can define loop points.

## To Install
You can download a compiled version of the plugins from Releases in this repository.
Extract the ZIP file and copy its contents to your SuperCollider extensions directory
(you can see where that is by running `Platform.userExtensionDir` in sclang).
Then restart SuperCollider if it is open.

## Building
To build and install these plugins, see `BUILDING.md`.

## License
This project is licensed under the GNU GPLv3.0.