# FFForge

An FFmpeg-based video pipeline that demonstrates the full lifecycle of programmatic video processing: synthetic frame generation, H.264 encoding, grayscale frame extraction, and H.265 transcoding — all driven from C++ using the FFmpeg libraries directly.

---

## What it does

FFForge runs three sequential stages in a single executable:

| Stage | What happens |
|-------|-------------|
| **Encode** | Generates 2500 frames of a synthetic YUV420P animation and encodes them to an H.264/MP4 file using libx264 (CRF 18, slow preset, 25 fps, 360×360). |
| **Extract** | Opens the H.264 output, decodes it, and saves the first 10 frames as grayscale PGM images in `./frames/`. |
| **Transcode** | Re-encodes the H.264 source to H.265/HEVC using libx265, preserving resolution, frame rate, and GOP structure. |

---

## Project structure

```
ffforge/
├── CMakeLists.txt       # Build definition (CMake + pkg-config)
├── config.sh            # Configure the CMake build
├── run.sh               # Build and run the executable
├── include/
│   ├── utils.h          # Shared macro (LOG_ERROR) and helper declarations
│   ├── encoder.h        # encode_h264() — synthetic video generation
│   ├── decoder.h        # extract_gray_frames() — PGM frame dumping
│   └── transcoder.h     # transcode_to_h265() — H.264 → H.265 pipeline
└── src/
    ├── main.cpp          # Entry point — orchestrates the three stages
    ├── utils.cpp         # encode_frame(), save_gray_frame()
    ├── encoder.cpp       # H.264 encoding implementation
    ├── decoder.cpp       # Decoding and frame extraction implementation
    └── transcoder.cpp    # Full decode → re-encode transcoding implementation
```

---

## Requirements

| Dependency | Minimum version | Notes |
|------------|----------------|-------|
| C++ compiler | C++17 | GCC 9+, Clang 10+, or MSVC 2019+ |
| CMake | 3.16 | |
| Ninja | any | Used as the CMake generator |
| pkg-config | any | Used to locate FFmpeg libraries |
| libavcodec | 58+ (FFmpeg 4.x) | Provides codec API |
| libavformat | 58+ | Provides muxing/demuxing API |
| libavutil | 56+ | Provides frame and utility types |
| libx264 | any | H.264 encoder (runtime dependency of libavcodec) |
| libx265 | any | H.265 encoder (runtime dependency of libavcodec) |

### Installing FFmpeg on common systems

**Ubuntu / Debian**
```bash
sudo apt install libavcodec-dev libavformat-dev libavutil-dev \
                 libx264-dev libx265-dev ninja-build
```

**Arch Linux**
```bash
sudo pacman -S ffmpeg x264 x265 ninja
```

**macOS (Homebrew)**
```bash
brew install ffmpeg ninja
```

---

## Building

### 1. Configure

Run once, or whenever `CMakeLists.txt` changes or you want to pass custom CMake options:

```bash
bash config.sh
```

This creates the `build/` and `frames/` directories and configures the project with CMake in Debug mode. Any extra CMake flags can be passed through:

```bash
bash config.sh -DCMAKE_BUILD_TYPE=Release
```

### 2. Build and run

```bash
bash run.sh
```

This compiles the project incrementally with Ninja and immediately runs the resulting binary.

### Manual equivalent

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build
./build/ffforge
```

---

## Output files

After a successful run you will find:

```
output_H.264.mp4      # Synthetic test video encoded in H.264
output_H.265.mp4      # Same video re-encoded in H.265
frames/
├── frame0.pgm        # Grayscale luma dump of frame 0
├── frame1.pgm
│   ...
└── frame9.pgm        # Last of the 10 extracted frames
```

PGM files can be viewed with any image viewer that supports the Netpbm format (e.g. `feh`, `eog`, Preview on macOS, or `ffplay frame0.pgm`).

---

## How the synthetic video is generated

Each frame `i` fills the YUV420P planes with a simple spatial function that produces an animated diagonal stripe pattern:

```
Y(x, y, i)  =  (x + y + i×3)       mod 256
U(x, y, i)  =  (128 + x + y + i×3) mod 256
V(x, y, i)  =  (64  + x + y + i×3) mod 256
```

The chroma planes are sampled at half resolution in both axes (4:2:0 subsampling). As `i` increments the pattern scrolls diagonally, producing a visually distinct frame for every timestamp — useful for verifying seek accuracy or codec correctness without needing an external source file.

---

## Architecture notes

- **`encode_frame()`** (in `utils.cpp`) is shared by both the encoder and the transcoder. It sends one frame to a codec context and drains all resulting packets into the muxer via `av_interleaved_write_frame`.
- **`LOG_ERROR`** terminates the process on any unrecoverable FFmpeg API failure. It is intentionally not an exception so that resource cleanup is explicit — each stage frees its own allocations unconditionally before returning.
- The transcoder does a **full pixel-level re-encode** rather than a stream copy. This means the output can have different encoder settings (preset, CRF, B-frames) independent of the source.
- Non-video streams (audio, subtitles) are skipped silently in both the decoder and the transcoder. Only the first video stream in a file is processed.

---

## License

This project is provided as-is for educational and demonstration purposes.