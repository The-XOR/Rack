# VCV Rack

*Rack* is the host application for the VCV virtual Eurorack modular synthesizer platform.

- [VCV website](https://vcvrack.com/)
- [Manual](https://vcvrack.com/manual/)
- [Support](https://vcvrack.com/support)
- [Module Library](https://library.vcvrack.com/)
- [Rack source code](https://github.com/VCVRack/Rack)
- [Building](https://vcvrack.com/manual/Building)
- [Communities](https://vcvrack.com/manual/Communities)
- [Licenses](LICENSE.md) ([HTML](LICENSE.html))

## Acknowledgments

- [Andrew Belt](https://github.com/AndrewBelt): Lead Rack developer
- [Pyer](https://www.pyer.be/): Module design, component graphics
- [Richie Hindle](http://entrian.com/audio/): Rack developer, bug fixes
- [Grayscale](https://grayscale.info/): Module design, branding
- Christoph Scholtes: [Library reviews](https://github.com/VCVRack/library) and [plugin toolchain](https://github.com/VCVRack/rack-plugin-toolchain)
- Translators
	- German: Stephan Müsch, Norbert Denninger
	- Spanish: Kevin U. Cano Guerra, Coriander V. Pines
	- French: Pyer
	- Italian: Alessandro Paglia
	- Chinese (Simplified): NoiseTone
	- Japanese: [Leo Kuroshita](https://x.com/kurogedelic)
- Rack plugin developers: Authorship shown on each plugin's [VCV Library](https://library.vcvrack.com/) page
- Rack users like you: [Bug reports and feature requests](https://vcvrack.com/support)

## Dependency libraries

- [GLFW](https://www.glfw.org/)
- [GLEW](http://glew.sourceforge.net/)
- [NanoVG](https://github.com/memononen/nanovg)
- [NanoSVG](https://github.com/memononen/nanosvg)
- [oui-blendish](https://hg.sr.ht/~duangle/oui-blendish)
- [osdialog](https://github.com/AndrewBelt/osdialog) (written by Andrew Belt for VCV Rack)
- [ghc::filesystem](https://github.com/gulrak/filesystem)
- [Jansson](https://digip.org/jansson/)
- [libcurl](https://curl.se/libcurl/)
- [OpenSSL](https://www.openssl.org/)
- [Zstandard](https://facebook.github.io/zstd/) (for Rack's `.tar.zstd` patch format)
- [libarchive](https://libarchive.org/) (for Rack's `.tar.zstd` patch format)
- [PFFFT](https://bitbucket.org/jpommier/pffft/)
- [libspeexdsp](https://gitlab.xiph.org/xiph/speexdsp/-/tree/master/libspeexdsp) (for Rack's fixed-ratio resampler)
- [libsamplerate](https://github.com/libsndfile/libsamplerate) (for Rack's variable-ratio resampler)
- [RtMidi](https://www.music.mcgill.ca/~gary/rtmidi/)
- [RtAudio](https://www.music.mcgill.ca/~gary/rtaudio/)
- [Fuzzy Search Database](https://bitbucket.org/j_norberg/fuzzysearchdatabase) (written by Nils Jonas Norberg for VCV Rack's module browser)
- [TinyExpr](https://codeplea.com/tinyexpr) (for math evaluation in parameter context menu)

VCV is unable to accept outside code contributions, but if you wish to contribute to the VCV Rack software, you can:
- Request a feature or report a bug to [VCV Support](https://vcvrack.com/support).
- [Learn about Rack](https://vcvrack.com/manual/) and answer questions in the [VCV communities](https://vcvrack.com/manual/Communities).
- [Develop your own Rack plugin](https://vcvrack.com/manual/PluginDevelopmentTutorial), or help maintain an existing plugin.
- Apply for a [job at VCV](https://vcvrack.com/jobs).

## ATTENSIUN

Su linux, bisogna installare zenity:
sudo pacman -S zenity

Per sincronizzare il sincornizzabile:
1 - git fetch upstream
2 - git checkout v2
3 - git merge upstream/v2


## Supporto OpenGL per macchine virtuose
Per far girare Rack in una macchina virtuosa, e' richiesto OpenGL.

- download an already compiled distribution of the Mesa3D sources at https://github.com/pal1000/mesa-dist-win/releases
- Extract and run systemwidedeploy.cmd in a command prompt.

You should see something like this:

-------------------------------------
Mesa3D system-wide deployment utility
-------------------------------------
Please make a deployment choice:
1. Core desktop OpenGL drivers
2. Core desktop OpenGL drivers + Intel swr
3. Install DirectX IL for redistribution only
4. Microsoft OpenGL over D3D12 driver only (replaces Mesa core desktop OpenGL drivers)
5. Mesa3D off-screen render driver gallium version (osmesa gallium)
7. Mesa3D graw test framework
8. Update system-wide deployment
9. Remove system-wide deployments (uninstall)
10. Exit
Enter choice:

Select the first option 1. Core desktop OpenGL drivers
And thats it.

