# flexplugins
A collection of SuperCollider plugins developed by Jeff Martin (www.jeffreymartincomposer.com).

## List of Plugins

### LoopPhasor
LoopPhasor is an adaptation of Phasor, which allows you to set loop points. It is useful for playing audio samples for which you can define loop points.

### PV_CFreeze
A SuperCollider implementation of Jean-François Charles' Max patch for freezing [(the Max patch is available here)](https://newfloremusic.gumroad.com/).
It is a phase vocoder spectral freeze with an innovation that makes the frozen sound less stagnant.

### PV_MagSqueeze
Squeezes spectral magnitudes to fit between (low, high).

### PV_MagXFade
Equal power crossfade between the magnitudes of two FFT buffers.

### RubberBandPS
A formant-preserving phase vocoder pitch shifter using the [RubberBand library](https://breakfastquay.com/rubberband/).

### RubberBandStretcher
A phase vocoder pitch shifter and time stretcher using the [RubberBand library](https://breakfastquay.com/rubberband/).

## To Install
You can download a compiled version of the plugins from Releases in this repository.
Extract the ZIP file and copy its contents to your SuperCollider extensions directory
(you can see where that is by running `Platform.userExtensionDir` in sclang).
Then restart SuperCollider if it is open.

## Building
To build and install these plugins, see `BUILDING.md`.

## License
This project is licensed under the GNU GPLv3.0.