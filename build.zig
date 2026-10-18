//! Zig build script for Shockolate; an alternative to the CMake build.
//!
//!   zig build                      # build ./zig-out/bin/systemshock
//!   zig build run                  # build and run
//!   zig build -Dfluidsynth=false   # drop FluidSynth MIDI support
//!
//! SDL2, SDL2_mixer and (optionally) FluidSynth are taken from the system
//! (found through pkg-config, e.g. Homebrew) unless -Dbundled-deps=true, which
//! uses the libraries built by build_deps.sh in build_ext/.
const std = @import("std");

const project_version = "0.7.8";

const include_dirs = [_][]const u8{
    "src/Libraries/2D/Source/GR",
    "src/Libraries/2D/Source/Clip",
    "src/Libraries/2D/Source/Flat8",
    "src/Libraries/2D/Source/Gen",
    "src/Libraries/2D/Source/RSD",
    "src/Libraries/2D/Source",
    "src/Libraries/3D/Source",
    "src/Libraries/AFILE/Source",
    "src/Libraries/DSTRUCT/Source",
    "src/Libraries/EDMS/Source",
    "src/Libraries/EDMS/Source/MODELS",
    "src/Libraries/FIXPP/Source",
    "src/Libraries/INPUT/Source",
    "src/Libraries/PALETTE/Source",
    "src/Libraries/RES/Source",
    "src/Libraries/UI/Source",
    "src/Libraries/RND/Source",
    "src/Libraries/VOX/Source",
    "src/Libraries/FIX/Source",
    "src/Libraries/SND/Source",
    "src/Libraries/H",
    "src/Libraries/LG/Source",
    "src/Libraries/LG/Source/LOG/src",
    "src/Libraries/adlmidi/include",
    "src/GameSrc/Headers",
    "src/MacSrc",
    "src/MusicSrc",
};

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{ .preferred_optimize_mode = .ReleaseSafe });

    const opengl = b.option(bool, "opengl", "Enable OpenGL renderer (default: true)") orelse true;
    const sound = b.option(bool, "sound", "Enable SDL2_mixer sound (default: true)") orelse true;
    const fluidsynth = b.option(bool, "fluidsynth", "Enable FluidSynth MIDI (default: true)") orelse true;
    const debug_blit = b.option(bool, "debug-blit", "Enable debugging blitter") orelse false;
    const alsa = b.option(bool, "alsa", "Enable ALSA MIDI output on Linux (default: true)") orelse true;
    const data_dir = b.option([]const u8, "data-dir", "Path to the game's res folder, passed to `run` as -data-dir (default: res)") orelse "res";
    const bundled = b.option(bool, "bundled-deps", "Use libraries from build_ext/ instead of the system") orelse false;
    const version_suffix = b.option([]const u8, "version-suffix", "Appended to the version string, e.g. -gabc1234") orelse "";

    const os = target.result.os.tag;

    const mod = b.createModule(.{
        .target = target,
        .optimize = optimize,
        .link_libc = true,
        .link_libcpp = true,
    });

    // Generated version header (CMake does this with configure_file).
    const gen = b.addWriteFiles();
    _ = gen.add("shockolate_version.h", b.fmt(
        \\#ifndef SHOCKOLATE_SRC_GAMESRC_HEADERS_SHOCOLATE_VERSION_H_
        \\#define SHOCKOLATE_SRC_GAMESRC_HEADERS_SHOCOLATE_VERSION_H_
        \\#define SHOCKOLATE_VERSION "Shockolate {s}{s}"
        \\#endif
        \\
    , .{ project_version, version_suffix }));
    mod.addIncludePath(gen.getDirectory());

    for (include_dirs) |d| mod.addIncludePath(b.path(d));

    // Defines shared by C and C++.
    mod.addCMacro("SVGA_SUPPORT", "1");
    if (debug_blit) mod.addCMacro("DEBUGGING_BLIT", "1");
    if (opengl) mod.addCMacro("USE_OPENGL", "1");
    if (sound) mod.addCMacro("USE_SDL_MIXER", "1");
    if (fluidsynth) mod.addCMacro("USE_FLUIDSYNTH", "1");

    const common = [_][]const u8{
        "-fsigned-char",
        "-fno-strict-aliasing",
        "-fno-sanitize=undefined", // this codebase relies on plenty of UB
        "-Wno-everything",
    };
    // The game and platform code (but not src/Libraries, as in CMake) is built
    // with the "magic header" that sets struct packing etc. force-included.
    const magic = [_][]const u8{ "-include", "src/GameSrc/Headers/precompiled.h" };
    const lib_c = common ++ [_][]const u8{"-std=gnu99"};
    const lib_cxx = common ++ [_][]const u8{ "-std=gnu++11", "-D__STDC_LIMIT_MACROS" };
    const game_c = lib_c ++ magic;
    const game_cxx = lib_cxx ++ magic;

    const lib_groups = .{
        lib_2d_src,    lib_3d_src,    lib_afile_src,   lib_dstruct_src, lib_fix_src,
        lib_input_src, lib_lg_src,    lib_palette_src, lib_res_src,     lib_rnd_src,
        lib_ui_src,    lib_fixpp_src, lib_edms_src,    lib_adlmidi_src, lib_vox_src,
    };
    inline for (lib_groups) |files| addSources(b, mod, &files, &lib_c, &lib_cxx);
    addSources(b, mod, &mac_src, &game_c, &game_cxx);
    addSources(b, mod, &game_src, &game_c, &game_cxx);

    // Dependencies.
    if (bundled) {
        mod.addIncludePath(b.path("build_ext/built_sdl/include/SDL2"));
        mod.addLibraryPath(b.path("build_ext/built_sdl/lib"));
        if (sound) {
            mod.addIncludePath(b.path("build_ext/built_sdl_mixer/include/SDL2"));
            mod.addLibraryPath(b.path("build_ext/built_sdl_mixer/lib"));
        }
        if (fluidsynth) {
            mod.addIncludePath(b.path("build_ext/fluidsynth-lite/include"));
            mod.addLibraryPath(b.path("build_ext/fluidsynth-lite/src"));
        }
    }
    mod.linkSystemLibrary("SDL2", .{});
    if (sound) mod.linkSystemLibrary("SDL2_mixer", .{});
    if (fluidsynth) mod.linkSystemLibrary("fluidsynth", .{});

    switch (os) {
        .macos => {
            if (opengl) mod.linkFramework("OpenGL", .{});
        },
        .windows => {
            mod.linkSystemLibrary("winmm", .{});
            if (opengl) {
                mod.linkSystemLibrary("opengl32", .{});
                mod.linkSystemLibrary("glew32", .{});
            }
        },
        else => {
            if (opengl) mod.linkSystemLibrary("GL", .{});
            mod.linkSystemLibrary("m", .{});
            if (alsa) {
                mod.addCMacro("USE_ALSA", "1");
                mod.linkSystemLibrary("asound", .{});
            }
        },
    }

    const exe = b.addExecutable(.{ .name = "systemshock", .root_module = mod });
    const exe_install = b.addInstallArtifact(exe, .{});
    b.getInstallStep().dependOn(&exe_install.step);
    // The OpenGL renderer loads shaders relative to the executable.
    b.installDirectory(.{ .source_dir = b.path("shaders"), .install_dir = .bin, .install_subdir = "shaders" });

    // Let Zig pass forward the data dir arg. -Ddata-dir=<path to the res folder> is passed to
    // the game as `-data-dir <path>` (anything after `--` is forwarded too).
    const run = b.addRunArtifact(exe);
    run.step.dependOn(b.getInstallStep());
    run.addArgs(&.{ "-data-dir", data_dir });
    b.step("run", "Build and run systemshock").dependOn(&run.step);
}

/// Adds `files` to `mod`, compiling .c files as C and everything else as C++.
fn addSources(
    b: *std.Build,
    mod: *std.Build.Module,
    files: []const []const u8,
    c_flags: []const []const u8,
    cxx_flags: []const []const u8,
) void {
    var c_files: std.ArrayList([]const u8) = .empty;
    var cxx_files: std.ArrayList([]const u8) = .empty;
    for (files) |f| {
        const list = if (std.mem.eql(u8, std.fs.path.extension(f), ".c")) &c_files else &cxx_files;
        list.append(b.allocator, f) catch @panic("OOM");
    }
    mod.addCSourceFiles(.{ .files = c_files.items, .flags = c_flags, .language = .c });
    mod.addCSourceFiles(.{ .files = cxx_files.items, .flags = cxx_flags, .language = .cpp });
}

// Source lists, mirroring CMakeLists.txt and src/Libraries/CMakeLists.txt.

const lib_2d_src = [_][]const u8{
    "src/Libraries/2D/Source/bit.c",
    "src/Libraries/2D/Source/bitmap.c",
    "src/Libraries/2D/Source/blend.c",
    "src/Libraries/2D/Source/canvas.c",
    "src/Libraries/2D/Source/close.c",
    "src/Libraries/2D/Source/cnvtab.c",
    "src/Libraries/2D/Source/context.c",
    "src/Libraries/2D/Source/detect.c",
    "src/Libraries/2D/Source/devtab.c",
    "src/Libraries/2D/Source/fcntab.c",
    "src/Libraries/2D/Source/init.c",
    "src/Libraries/2D/Source/lintab.c",
    "src/Libraries/2D/Source/mode.c",
    "src/Libraries/2D/Source/pal.c",
    "src/Libraries/2D/Source/permap.c",
    "src/Libraries/2D/Source/persetup.c",
    "src/Libraries/2D/Source/pertol.c",
    "src/Libraries/2D/Source/rgb.c",
    "src/Libraries/2D/Source/screen.c",
    "src/Libraries/2D/Source/sscrn.c",
    "src/Libraries/2D/Source/StateStk.c",
    "src/Libraries/2D/Source/svgainit.c",
    "src/Libraries/2D/Source/tempbm.c",
    "src/Libraries/2D/Source/temptm.c",
    "src/Libraries/2D/Source/tlucdat.c",
    "src/Libraries/2D/Source/tluctab.c",
    "src/Libraries/2D/Source/valloc.c",
    "src/Libraries/2D/Source/vtab.c",
    "src/Libraries/2D/Source/MacDev.c",
    "src/Libraries/2D/Source/GR/grilin.c",
    "src/Libraries/2D/Source/GR/grmalloc.c",
    "src/Libraries/2D/Source/GR/grnull.c",
    "src/Libraries/2D/Source/GR/gruilin.c",
    "src/Libraries/2D/Source/GR/grd.c",
    "src/Libraries/2D/Source/Gen/gendisk.c",
    "src/Libraries/2D/Source/Gen/gentm.c",
    "src/Libraries/2D/Source/Gen/genwclin.c",
    "src/Libraries/2D/Source/Gen/genuclin.c",
    "src/Libraries/2D/Source/Gen/genrsd8.c",
    "src/Libraries/2D/Source/Gen/genuvlin.c",
    "src/Libraries/2D/Source/Gen/genhlin.c",
    "src/Libraries/2D/Source/Gen/genrsdbm.c",
    "src/Libraries/2D/Source/Gen/genvlin.c",
    "src/Libraries/2D/Source/Gen/genvcply.c",
    "src/Libraries/2D/Source/Gen/genclin.c",
    "src/Libraries/2D/Source/Gen/genfl8c.c",
    "src/Libraries/2D/Source/Gen/genrsdtm.c",
    "src/Libraries/2D/Source/Gen/genel.c",
    "src/Libraries/2D/Source/Gen/genwlin.c",
    "src/Libraries/2D/Source/Gen/genpix.c",
    "src/Libraries/2D/Source/Gen/genuhlin.c",
    "src/Libraries/2D/Source/Gen/genhfl8.c",
    "src/Libraries/2D/Source/Gen/genov.c",
    "src/Libraries/2D/Source/Gen/gente.c",
    "src/Libraries/2D/Source/Gen/genlin.c",
    "src/Libraries/2D/Source/Gen/gencwlin.c",
    "src/Libraries/2D/Source/Gen/genchfl8.c",
    "src/Libraries/2D/Source/Gen/gengfl8.c",
    "src/Libraries/2D/Source/Gen/genbox.c",
    "src/Libraries/2D/Source/Gen/genmono.c",
    "src/Libraries/2D/Source/Gen/genulin.c",
    "src/Libraries/2D/Source/Gen/gencnv.c",
    "src/Libraries/2D/Source/Gen/genuslin.c",
    "src/Libraries/2D/Source/Gen/genvpoly.c",
    "src/Libraries/2D/Source/Gen/general.c",
    "src/Libraries/2D/Source/Gen/gencirc.c",
    "src/Libraries/2D/Source/Gen/genfl8.c",
    "src/Libraries/2D/Source/Gen/genf24.c",
    "src/Libraries/2D/Source/Gen/gentl8.c",
    "src/Libraries/2D/Source/Gen/genrect.c",
    "src/Libraries/2D/Source/Gen/genvrect.c",
    "src/Libraries/2D/Source/Gen/genslin.c",
    "src/Libraries/2D/Source/Flat8/fl8vlin.c",
    "src/Libraries/2D/Source/Flat8/FL8OPL.c",
    "src/Libraries/2D/Source/Flat8/fl8cply.c",
    "src/Libraries/2D/Source/Flat8/fl8clin.c",
    "src/Libraries/2D/Source/Flat8/fl8lp.c",
    "src/Libraries/2D/Source/Flat8/fl8lf.c",
    "src/Libraries/2D/Source/Flat8/fl8ll.c",
    "src/Libraries/2D/Source/Flat8/fl8s.c",
    "src/Libraries/2D/Source/Flat8/fl8hfl8.c",
    "src/Libraries/2D/Source/Flat8/fl8pnt.c",
    "src/Libraries/2D/Source/Flat8/fl8fl8m.c",
    "src/Libraries/2D/Source/Flat8/fl8ltp.c",
    "src/Libraries/2D/Source/Flat8/fl8fl8c.c",
    "src/Libraries/2D/Source/Flat8/fl8w.c",
    "src/Libraries/2D/Source/Flat8/fl8cop.c",
    "src/Libraries/2D/Source/Flat8/fl8wlin.c",
    "src/Libraries/2D/Source/Flat8/fl8lw.c",
    "src/Libraries/2D/Source/Flat8/fl8pix.c",
    "src/Libraries/2D/Source/Flat8/fl8row.c",
    "src/Libraries/2D/Source/Flat8/fl8ft.c",
    "src/Libraries/2D/Source/Flat8/fl8rsd8.c",
    "src/Libraries/2D/Source/Flat8/Fl8F.c",
    "src/Libraries/2D/Source/Flat8/fl8bldbl.c",
    "src/Libraries/2D/Source/Flat8/fl8clear.c",
    "src/Libraries/2D/Source/Flat8/fl8p.c",
    "src/Libraries/2D/Source/Flat8/fl8wclin.c",
    "src/Libraries/2D/Source/Flat8/fl8hlin.c",
    "src/Libraries/2D/Source/Flat8/fl8bl.c",
    "src/Libraries/2D/Source/Flat8/fl8gpix.c",
    "src/Libraries/2D/Source/Flat8/fl8ns.c",
    "src/Libraries/2D/Source/Flat8/fl8lnop.c",
    "src/Libraries/2D/Source/Flat8/fl8dbl.c",
    "src/Libraries/2D/Source/Flat8/fl8cnv.c",
    "src/Libraries/2D/Source/Flat8/fl8slin.c",
    "src/Libraries/2D/Source/Flat8/fl8lin.c",
    "src/Libraries/2D/Source/Flat8/fl8tsmap.c",
    "src/Libraries/2D/Source/Flat8/fl8ply.c",
    "src/Libraries/2D/Source/Flat8/fl8sply.c",
    "src/Libraries/2D/Source/Flat8/fl8rect.c",
    "src/Libraries/2D/Source/Flat8/fl8gfl8.c",
    "src/Libraries/2D/Source/Flat8/fl8tl8.c",
    "src/Libraries/2D/Source/Flat8/fl8p24.c",
    "src/Libraries/2D/Source/Flat8/fl8g24.c",
    "src/Libraries/2D/Source/Flat8/fl8ctp.c",
    "src/Libraries/2D/Source/Flat8/fl8mscl.c",
    "src/Libraries/2D/Source/Flat8/fl8lop.c",
    "src/Libraries/2D/Source/Flat8/fl8ntrp2.c",
    "src/Libraries/2D/Source/Flat8/fl8sub.c",
    "src/Libraries/2D/Source/Flat8/fl8tpl.c",
    "src/Libraries/2D/Source/Flat8/fl8nl.c",
    "src/Libraries/2D/Source/Flat8/fl8fltr2.c",
    "src/Libraries/2D/Source/Flat8/fl8mono.c",
    "src/Libraries/2D/Source/Flat8/fl8fl8.c",
    "src/Libraries/2D/Source/Flat8/fl8chfl8.c",
    "src/Libraries/2D/Source/Clip/clpply.c",
    "src/Libraries/2D/Source/Clip/clplin.c",
    "src/Libraries/2D/Source/Clip/clpclin.c",
    "src/Libraries/2D/Source/Clip/clpltab.c",
    "src/Libraries/2D/Source/Clip/clpf24.c",
    "src/Libraries/2D/Source/Clip/clplin2.c",
    "src/Libraries/2D/Source/Clip/clppoly.c",
    "src/Libraries/2D/Source/Clip/clpslin.c",
    "src/Libraries/2D/Source/Clip/clprect.c",
    "src/Libraries/2D/Source/Clip/clpmono.c",
    "src/Libraries/2D/Source/RSD/rsdcvt.c",
    "src/Libraries/2D/Source/RSD/RSDUnpack.c",
    "src/Libraries/2D/Source/string/chrsiz.c",
    "src/Libraries/2D/Source/string/genchr.c",
    "src/Libraries/2D/Source/string/genstr.c",
    "src/Libraries/2D/Source/string/genuchr.c",
    "src/Libraries/2D/Source/string/genustr.c",
    "src/Libraries/2D/Source/string/strscl.c",
    "src/Libraries/2D/Source/string/strsiz.c",
    "src/Libraries/2D/Source/string/struscl.c",
    "src/Libraries/2D/Source/string/strwrap.c",
};

const lib_3d_src = [_][]const u8{
    "src/Libraries/3D/Source/alloc.c",
    "src/Libraries/3D/Source/Bitmap.c",
    "src/Libraries/3D/Source/clip.c",
    "src/Libraries/3D/Source/detail.c",
    "src/Libraries/3D/Source/fov.c",
    "src/Libraries/3D/Source/GlobalV.c",
    "src/Libraries/3D/Source/instance.c",
    "src/Libraries/3D/Source/interp.c",
    "src/Libraries/3D/Source/light.c",
    "src/Libraries/3D/Source/matrix.c",
    "src/Libraries/3D/Source/points.c",
    "src/Libraries/3D/Source/polygon.c",
    "src/Libraries/3D/Source/slew.c",
    "src/Libraries/3D/Source/tmap.c",
    "src/Libraries/3D/Source/vector.c",
};

const lib_afile_src = [_][]const u8{
    "src/Libraries/AFILE/Source/afile.c",
    "src/Libraries/AFILE/Source/amov.c",
    "src/Libraries/AFILE/Source/compose.c",
    "src/Libraries/AFILE/Source/draw4x4.cpp",
    "src/Libraries/AFILE/Source/huffde.cpp",
    "src/Libraries/AFILE/Source/movie.c",
};

const lib_dstruct_src = [_][]const u8{
    "src/Libraries/DSTRUCT/Source/array.c",
    "src/Libraries/DSTRUCT/Source/hash.c",
    "src/Libraries/DSTRUCT/Source/pqueue.c",
    "src/Libraries/DSTRUCT/Source/rect.c",
};

// fix_pow.c is omitted: fix.c defines fix_pow too, and with CMake's static
// archives the linker never pulled fix_pow.o in.
const lib_fix_src = [_][]const u8{
    "src/Libraries/FIX/Source/f_exp.c",
    "src/Libraries/FIX/Source/fix.c",
    "src/Libraries/FIX/Source/fix_sqrt.c",
    "src/Libraries/FIX/Source/MakeTables.c",
};

const lib_input_src = [_][]const u8{
    "src/Libraries/INPUT/Source/kbcook.c",
    "src/Libraries/INPUT/Source/mouse.c",
    "src/Libraries/INPUT/Source/sdl_events.c",
};

const lib_lg_src = [_][]const u8{
    "src/Libraries/LG/Source/LOG/src/log.c",
    "src/Libraries/LG/Source/memall.c",
    "src/Libraries/LG/Source/stack.c",
    "src/Libraries/LG/Source/tmpalloc.c",
};

const lib_palette_src = [_][]const u8{
    "src/Libraries/PALETTE/Source/palette.c",
};

const lib_res_src = [_][]const u8{
    "src/Libraries/RES/Source/caseless.c",
    "src/Libraries/RES/Source/lzw.c",
    "src/Libraries/RES/Source/refacc.c",
    "src/Libraries/RES/Source/resacc.c",
    "src/Libraries/RES/Source/resbuild.c",
    "src/Libraries/RES/Source/res.c",
    "src/Libraries/RES/Source/resfile.c",
    "src/Libraries/RES/Source/resformat.c",
    "src/Libraries/RES/Source/resload.c",
    "src/Libraries/RES/Source/resmake.c",
    "src/Libraries/RES/Source/restypes.c",
};

const lib_rnd_src = [_][]const u8{
    "src/Libraries/RND/Source/rnd.c",
};

const lib_ui_src = [_][]const u8{
    "src/Libraries/UI/Source/curdrw.c",
    "src/Libraries/UI/Source/cursors.c",
    "src/Libraries/UI/Source/event.c",
    "src/Libraries/UI/Source/hotkey.c",
    "src/Libraries/UI/Source/region.c",
    "src/Libraries/UI/Source/slab.c",
    "src/Libraries/UI/Source/vmouse.c",
};

const lib_fixpp_src = [_][]const u8{
    "src/Libraries/FIXPP/Source/fixpp.cpp",
};

const lib_edms_src = [_][]const u8{
    "src/Libraries/EDMS/Source/interfac.cc",
    "src/Libraries/EDMS/Source/collide.cc",
    "src/Libraries/EDMS/Source/intrsect.cc",
    "src/Libraries/EDMS/Source/globals.cc",
    "src/Libraries/EDMS/Source/phy_tool.cc",
    "src/Libraries/EDMS/Source/soliton.cc",
    "src/Libraries/EDMS/Source/MODELS/robot.cc",
    "src/Libraries/EDMS/Source/MODELS/pelface.cc",
    "src/Libraries/EDMS/Source/MODELS/ftl.cc",
    "src/Libraries/EDMS/Source/MODELS/pelvis.cc",
    "src/Libraries/EDMS/Source/MODELS/d_frame.cc",
    "src/Libraries/EDMS/Source/MODELS/d_f_face.cc",
    "src/Libraries/EDMS/Source/MODELS/ftlface.cc",
    "src/Libraries/EDMS/Source/MODELS/d_f_2.cc",
};

const lib_adlmidi_src = [_][]const u8{
    "src/Libraries/adlmidi/adlmidi_sequencer.cpp",
    "src/Libraries/adlmidi/chips/dosbox/dbopl.cpp",
    "src/Libraries/adlmidi/chips/dosbox_opl3.cpp",
    "src/Libraries/adlmidi/chips/nuked_opl3_v174.cpp",
    "src/Libraries/adlmidi/chips/nuked/nukedopl3_174.c",
    "src/Libraries/adlmidi/chips/nuked/nukedopl3.c",
    "src/Libraries/adlmidi/chips/nuked_opl3.cpp",
    "src/Libraries/adlmidi/wopl/wopl_file.c",
    "src/Libraries/adlmidi/adlmidi_opl3.cpp",
    "src/Libraries/adlmidi/adlmidi_private.cpp",
    "src/Libraries/adlmidi/adlmidi_midiplay.cpp",
    "src/Libraries/adlmidi/adlmidi.cpp",
    "src/Libraries/adlmidi/adlmidi_load.cpp",
    "src/Libraries/adlmidi/adldata.cpp",
};

const lib_vox_src = [_][]const u8{
    "src/Libraries/VOX/Source/vox2d.c",
    "src/Libraries/VOX/Source/vox3d.c",
    "src/Libraries/VOX/Source/voxinit.c",
};

const mac_src = [_][]const u8{
    "src/MacSrc/ShockBitmap.c",
    "src/MacSrc/InitMac.c",
    "src/MacSrc/Shock.c",
    "src/MacSrc/Prefs.c",
    "src/MacSrc/MacTune.c",
    "src/MacSrc/SDLSound.c",
    "src/MacSrc/Modding.c",
    "src/MacSrc/OpenGL.cc",
    "src/MacSrc/Xmi.c",
    "src/MusicSrc/MusicDevice.c",
};

const game_src = [_][]const u8{
    "src/GameSrc/ai.c",
    "src/GameSrc/airupt.c",
    "src/GameSrc/amap.c",
    "src/GameSrc/amaploop.c",
    "src/GameSrc/ammomfd.c",
    "src/GameSrc/anim.c",
    "src/GameSrc/archiveformat.c",
    "src/GameSrc/audiolog.c",
    "src/GameSrc/automap.c",
    "src/GameSrc/bark.c",
    "src/GameSrc/biohelp.c",
    "src/GameSrc/cardmfd.c",
    "src/GameSrc/citres.c",
    "src/GameSrc/combat.c",
    "src/GameSrc/cone.c",
    "src/GameSrc/criterr.c",
    "src/GameSrc/cyber.c",
    "src/GameSrc/cybermfd.c",
    "src/GameSrc/cybmem.c",
    "src/GameSrc/cybrnd.c",
    "src/GameSrc/cutsloop.c",
    "src/GameSrc/damage.c",
    "src/GameSrc/digifx.c",
    "src/GameSrc/drugs.c",
    "src/GameSrc/effect.c",
    "src/GameSrc/email.c",
    "src/GameSrc/faceobj.c",
    "src/GameSrc/fixtrmfd.c",
    "src/GameSrc/frcamera.c",
    "src/GameSrc/frclip.c",
    "src/GameSrc/frcompil.c",
    "src/GameSrc/frmain.c",
    "src/GameSrc/frobj.c",
    "src/GameSrc/froslew.c",
    "src/GameSrc/frpipe.c",
    "src/GameSrc/frpts.c",
    "src/GameSrc/frsetup.c",
    "src/GameSrc/frtables.c",
    "src/GameSrc/frterr.c",
    "src/GameSrc/frutil.c",
    "src/GameSrc/FrUtils.c",
    "src/GameSrc/fullamap.c",
    "src/GameSrc/fullscrn.c",
    "src/GameSrc/gameloop.c",
    "src/GameSrc/gameobj.c",
    "src/GameSrc/gamesort.c",
    "src/GameSrc/gamestrn.c",
    "src/GameSrc/gamesys.c",
    "src/GameSrc/gametime.c",
    "src/GameSrc/gamewrap.c",
    "src/GameSrc/gearmfd.c",
    "src/GameSrc/gr2ss.c",
    "src/GameSrc/grenades.c",
    "src/GameSrc/hand.c",
    "src/GameSrc/hflip.c",
    "src/GameSrc/hkeyfunc.c",
    "src/GameSrc/hud.c",
    "src/GameSrc/hudobj.c",
    "src/GameSrc/init.c",
    "src/GameSrc/input.c",
    "src/GameSrc/invent.c",
    "src/GameSrc/leanmetr.c",
    "src/GameSrc/mainloop.c",
    "src/GameSrc/map.c",
    "src/GameSrc/mfdfunc.c",
    "src/GameSrc/mfdgadg.c",
    "src/GameSrc/mfdgames.c",
    "src/GameSrc/mfdgump.c",
    "src/GameSrc/mfdpanel.c",
    "src/GameSrc/minimax.c",
    "src/GameSrc/mlimbs.c",
    "src/GameSrc/movekeys.c",
    "src/GameSrc/musicai.c",
    "src/GameSrc/newai.c",
    "src/GameSrc/newmfd.c",
    "src/GameSrc/objapp.c",
    "src/GameSrc/objects.c",
    "src/GameSrc/objload.c",
    "src/GameSrc/objprop.c",
    "src/GameSrc/objsim.c",
    "src/GameSrc/objuse.c",
    "src/GameSrc/olh.c",
    "src/GameSrc/olhscan.c",
    "src/GameSrc/palfx.c",
    "src/GameSrc/pathfind.c",
    "src/GameSrc/physics.c",
    "src/GameSrc/player.c",
    "src/GameSrc/plotware.c",
    "src/GameSrc/popups.c",
    "src/GameSrc/render.c",
    "src/GameSrc/rendtool.c",
    "src/GameSrc/saveload.c",
    "src/GameSrc/schedule.c",
    "src/GameSrc/screen.c",
    "src/GameSrc/setup.c",
    "src/GameSrc/shodan.c",
    "src/GameSrc/sideicon.c",
    "src/GameSrc/sndcall.c",
    "src/GameSrc/star.c",
    "src/GameSrc/statics.c",
    "src/GameSrc/status.c",
    "src/GameSrc/target.c",
    "src/GameSrc/textmaps.c",
    "src/GameSrc/tickcount.c",
    "src/GameSrc/tfdirect.c",
    "src/GameSrc/tfutil.c",
    "src/GameSrc/tools.c",
    "src/GameSrc/trigger.c",
    "src/GameSrc/view360.c",
    "src/GameSrc/viewhelp.c",
    "src/GameSrc/vitals.c",
    "src/GameSrc/vmail.c",
    "src/GameSrc/wares.c",
    "src/GameSrc/weapons.c",
    "src/GameSrc/wrapper.c",
    "src/GameSrc/gamerend.c",
    "src/GameSrc/mouselook.c",
};
