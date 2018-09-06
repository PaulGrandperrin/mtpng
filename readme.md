# mtpng

A parallelized PNG encoder in Rust

by Brion Vibber <brion@pobox.com>

# Background

Compressing PNG files is a relatively slow operation at large image sizes, and can take from half a second to over a second for 4K resolution and beyond.

The biggest CPU costs in traditional libpng seem to be the filtering, which is easy to parallelize, and the deflate compression, which can be parallelized in chunks at a slight loss of compression between block boundaries.

[pigz](https://zlib.net/pigz/) is a well-known C implementation of parallelized deflate/gzip compression, and was a strong inspiration for the chunking scheme used here.

I was also inspired by an experimental C++/OpenMP project called `png-parallel` by Pascal Beyeler, which didn't implement filtering but confirmed the basic theory.

# State

Currently very unfinished, but more or less works. Not yet optimized or made usable.

# Performance

Note that unoptimized debug builds are about 25x slower than optimized release builds. Always run with `--release`!

As of September 5, 2018 with Rust 1.28.0, single-threaded performance is ~20% faster than gdk-pixbuf + libpng on the test images in the samples folder, but ~20% slower than php + gd + libpng saving the same images.

Compression is a bit better than libpng with the dual-4K screenshot, and a bit worse on the arch photo.

Scaling is pretty good up to 8 physical cores.

Times for re-encoding the dual-4K screenshot at default options:

```
MacBook Pro 13" 2015
5th-gen Core i7 3.1 GHz
2 cores + Hyper-Threading

macOS x86_64:
- libgd     + libpng --  974 ms (target to beat)

- mtpng @  1 thread  --  907 ms (beating target!)
- mtpng @  2 threads --  480 ms -- 1.9x
- mtpng @  4 threads --  438 ms -- 2.2x (HT)
```

```
Refurbed old Dell workstation
Xeon E5520 2.26 GHz
2x 4 cores + Hyper-Threading
configured for SMP (NUMA disabled)

Linux x86_64:
- gdkpixbuf + libpng -- 2308 ms (slowdown fixed upstream)
- libgd     + libpng -- 1695 ms (target to beat)

- mtpng @  1 thread  -- 1802 ms (getting close!)
- mtpng @  2 threads --  948 ms -- 1.9x
- mtpng @  4 threads --  488 ms -- 3.7x
- mtpng @  8 threads --  264 ms -- 6.8x
- mtpng @ 16 threads --  229 ms -- 7.8x (HT)

Windows 10 x86_64:
- mtpng @  1 thread  -- 2240 ms
- mtpng @  2 threads -- 1148 ms -- 2.0x
- mtpng @  4 threads --  595 ms -- 3.8x
- mtpng @  8 threads --  321 ms -- 7.0x
- mtpng @ 16 threads --  275 ms -- 8.2x (HT)

Windows 10 i686:
- mtpng @  1 thread  -- 2631 ms
- mtpng @  2 threads -- 1315 ms -- 2.0x
- mtpng @  4 threads --  684 ms -- 3.8x
- mtpng @  8 threads --  359 ms -- 7.3x
- mtpng @ 16 threads --  295 ms -- 8.9x
```

Windows seems a little slower than Linux on the same machine; this may be system overhead or CPU throttling configuration or something rather than the generated code. (Trying the Linux binary under WSL runs about the same speed as the Windows native binary.)

32-bit builds are a bit slower still, but I don't have a native libpng comparison handy.

# Todos

Immediate todos:
* benchmark and optimize
* compare compression tradeoffs for different chunk sizes

In a bit todos:
* clean up error handling and builder patterns
* start figuring out a public-facing api
* publish crate

Someday todos:
* reading support with two-thread pipeline (see data flow diagram below)

See the [projects list on GitHub](https://github.com/brion/mtpng/projects) for active details.


# Data flow

Encoding can be broken into many parallel blocks:

![Encoder data flow diagram](https://raw.githubusercontent.com/brion/mtpng/master/docs/data-flow-write.png)

Decoding cannot; it must be run as a stream, but can pipeline.

![Decoder data flow diagram](https://raw.githubusercontent.com/brion/mtpng/master/docs/data-flow-read.png)

# Dependencies

[Rayon](https://crates.io/crates/rayon) is used for its ThreadPool implementation. You can create an encoder using either the default Rayon global pool or a custom ThreadPool instance.

[crc](https://crates.io/crates/crc) is used for calculating PNG chunk checksums.

[libz-sys](https://crates.io/crates/libz-sys) is used to wrap libz for the deflate compression. I briefly looked at pure-Rust implementations but couldn't find any supporting raw stream output, dictionary setting, and flushing to byte boundaries without closing the stream.

[png](https://crates.io/crates/png) is used by the CLI tool to load input files to recompress for testing.

[clap](https://crates.io/crates/clap) is used by the CLI tool to handle option parsing and help display.

[time](https://crates.io/crates/time) is used by the CLI tool to time compression.

# License

You may use this software under the following MIT-style license:

Copyright (c) 2018 Brion Vibber

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
