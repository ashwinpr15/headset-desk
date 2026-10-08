# Attribution and source provenance

Protocol framing, semantic commands, protocol session, fake transport, transport interfaces, Windows Winsock RFCOMM connector and selected tests are reused/adapted from [marconvcm/sony-device-center](https://github.com/marconvcm/sony-device-center), under MIT.

The exact upstream notice is preserved at `vendor/sony-device-center/LICENSE`, including all named holders and contributors. The notice must accompany distributions containing substantial portions of that code. The imported file list, original per-file SHA-256 values and input ZIP SHA-256 are in `vendor/sony-device-center/IMPORT-MANIFEST.json`. The uploaded snapshot declares version 0.1.5; it contains no usable commit identity, so a commit SHA is not invented. The uploaded GitHub-page PDF was a research reference; source bytes came from the ZIP.

Changes to imported files are recorded in `upstream-changes.md`; the import manifest records original bytes, not the modified ones. The project deliberately excludes the upstream GUI, daemon/IPC, installer, JSON package, fonts and marketing images from its Phase 2 build. Broader upstream capability records are retained for regression tests but are not used by the Headset Desk core.

Static codec lists are supported by Sony's [XM5 specifications](https://www.sony.com/electronics/support/wireless-headphones-bluetooth-headphones/wh-1000xm5/specifications) and [CH720N specifications](https://www.sony.com/electronics/support/wireless-headphones-bluetooth-headphones/wh-ch720n/specifications). Static support does not establish an active codec or Sony protocol support.

Local verification used [LLVM-MinGW](https://github.com/mstorsjo/llvm-mingw), [CMake](https://cmake.org/download/), [Ninja](https://github.com/ninja-build/ninja) and [Catch2](https://github.com/catchorg/Catch2/tree/v3.8.1). These developer tools are not included in the source deliverable. Any runtime licenses needed for the separate diagnostic binary are included alongside it.

The local desktop preview uses unmodified, dynamically linked Qt 6.8.3 libraries and QML
plugins under LGPLv3, with applicable third-party notices. Its matching compiler is Qt's
MinGW GCC 13.1.0. The portable folder includes Qt, GCC runtime exception, MinGW-w64 and
winpthreads license texts. Complete verified Qt source archives accompany the local
deliverables. FluentWinUI3 assets are supplied by Qt; no Sony artwork is used. The small
headset outline is drawn by new Headset Desk code.
