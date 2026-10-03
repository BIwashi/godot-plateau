#!/usr/bin/env python
"""
SConstruct for godot-plateau GDExtension
Build with: scons platform=<windows|linux|macos|android|ios> target=<template_debug|template_release|editor>

Android build requires:
  - ANDROID_HOME or ANDROID_SDK_ROOT environment variable set
  - NDK installed (default version: 23.1.7779620)
  - Example: scons platform=android arch=arm64

iOS build requires:
  - macOS with Xcode installed
  - Example: scons platform=ios arch=arm64

Linux build requires:
  - GCC 13 (recommended, Ubuntu 24.04 default) or Clang
  - OpenGL development libraries: sudo apt-get install libgl1-mesa-dev libglu1-mesa-dev
  - Example: scons platform=linux arch=x86_64
  - For GCC 15+ environments, use Clang: scons platform=linux arch=x86_64 use_clang=yes

macOS build requires:
  - Xcode with Command Line Tools
  - Homebrew packages: brew install mesa-glu xz libdeflate
  - Example: scons platform=macos arch=arm64
  - Custom Homebrew prefix: scons platform=macos arch=arm64 homebrew_prefix=/usr/local

Options:
  - skip_libplateau_build=yes: Skip libplateau build (use pre-built library)
  - skip_tests=yes: Skip building libplateau tests and examples (faster CI builds)
  - use_clang=yes: Use Clang instead of GCC on Linux
  - homebrew_prefix=/path: Custom Homebrew prefix on macOS
"""

import os
import sys
import platform as python_platform
import shutil
import subprocess
from pathlib import Path

from SCons.Script import Dir

from methods import print_error, print_warning

# Configuration
LIB_NAME = "godot-plateau"
GODOT_PROJECT_DIR = "project"
DEFAULT_HOMEBREW_PREFIX = "/opt/homebrew"

REPO_ROOT = Path(Dir('#').abspath)
LIBPLATEAU_ROOT = REPO_ROOT / "libplateau"
# GLU's polygon tessellator, compiled into the iOS library (iOS has no GLU).
GLU_ROOT = REPO_ROOT / "thirdparty" / "glu"
GLU_SOURCES = [
    "src/libtess/dict.c",
    "src/libtess/geom.c",
    "src/libtess/memalloc.c",
    "src/libtess/mesh.c",
    "src/libtess/normal.c",
    "src/libtess/priorityq.c",  # includes priorityq-heap.c
    "src/libtess/render.c",
    "src/libtess/sweep.c",
    "src/libtess/tess.c",
    "src/libtess/tessmono.c",
    "src/glu_error.c",
]
# Patches applied to the libplateau / libcitygml submodules before cmake configures them:
# (submodule path, patch file). They let iOS build the CityGML parser (see patches/).
SUBMODULE_PATCHES = [
    (LIBPLATEAU_ROOT, REPO_ROOT / "patches" / "libplateau-ios-citygml.patch"),
    (LIBPLATEAU_ROOT / "3rdparty" / "libcitygml", REPO_ROOT / "patches" / "libcitygml-ios-citygml.patch"),
    (LIBPLATEAU_ROOT / "3rdparty" / "xerces-c", REPO_ROOT / "patches" / "xerces-c-ios-utf8.patch"),
]
BUILD_ROOT = REPO_ROOT / "build"

# Platform-specific libplateau build settings
def get_libplateau_build_dir(platform, arch=""):
    """Get the libplateau build directory for the platform."""
    if platform in ("android", "ios") and arch:
        return BUILD_ROOT / platform / arch / "libplateau"
    return BUILD_ROOT / platform / "libplateau"


def get_libplateau_lib_path(platform, build_dir, build_type):
    """Get the libplateau library path for the platform."""
    if platform == "windows":
        # Windows uses build type as subdirectory (Release/RelWithDebInfo/Debug)
        return build_dir / "src" / build_type / "plateau_combined.lib"
    elif platform == "macos":
        # Use combined library that includes all dependencies
        return build_dir / "src" / "libplateau_combined.a"
    elif platform == "android":
        return build_dir / "src" / "libplateau.a"
    elif platform == "ios":
        # iOS builds as a framework
        return build_dir / "src" / "plateau.framework" / "plateau"
    else:  # linux
        return build_dir / "src" / "libplateau_combined.a"


def get_cmake_build_type(target_name):
    """Map SCons target to CMake build type."""
    target_to_cmake = {
        "editor": "RelWithDebInfo",
        "template_debug": "RelWithDebInfo",
        "template_release": "Release",
    }
    return target_to_cmake.get(target_name, "Release")


# Compiler flag that rewrites the checkout's absolute path to "." in __FILE__, debug info and
# assertion messages, so the built libraries do not carry the build machine's directory names.
PATH_MAP_FLAG = f"-ffile-prefix-map={REPO_ROOT}=."


def get_cmake_configure_args(platform, build_dir, build_type, env=None):
    """Get platform-specific cmake configure arguments."""
    args = _get_cmake_configure_args(platform, build_dir, build_type, env)
    if platform == "windows":
        return args
    # Add PATH_MAP_FLAG to the C and C++ flags of every libplateau sub-build.
    flags = [a for a in args if a.startswith("-DCMAKE_CXX_FLAGS")]
    if flags:
        args = [f"{a} {PATH_MAP_FLAG}" if a in flags else a for a in args]
    else:
        args = args + [f"-DCMAKE_CXX_FLAGS={PATH_MAP_FLAG}"]
    return args + [f"-DCMAKE_C_FLAGS={PATH_MAP_FLAG}"]


def _get_cmake_configure_args(platform, build_dir, build_type, env=None):
    common_args = [
        "-DPLATEAU_USE_FBX=OFF",
        "-DPLATEAU_USE_HTTP=OFF",  # Disable HTTP/OpenSSL - use Godot's HTTPRequest instead
        "-DBUILD_LIB_TYPE=static",
        f"-DCMAKE_BUILD_TYPE:STRING={build_type}",
        # Disable documentation to avoid duplicate "doc" target conflicts
        "-Dtiff-docs=OFF",           # libtiff docs
        "-DRAPIDJSON_BUILD_DOC=OFF", # RapidJSON docs (via glTF-SDK)
    ]

    if platform == "windows":
        return common_args + [
            "-DRUNTIME_LIB_TYPE=MT",  # Static runtime to match godot-cpp
            "-G", "Visual Studio 17 2022",
        ]
    elif platform == "macos":
        # Get Homebrew prefix for liblzma and libdeflate
        homebrew_prefix = env.get("homebrew_prefix", DEFAULT_HOMEBREW_PREFIX) if env else DEFAULT_HOMEBREW_PREFIX
        arch = env.get("arch", "arm64") if env else "arm64"
        # For universal builds, need both arm64 and x86_64
        if arch == "universal":
            macos_arch = "arm64;x86_64"
        else:
            macos_arch = arch
        return common_args + [
            "-DRUNTIME_LIB_TYPE=MD",
            '-DCMAKE_CXX_FLAGS=-w',
            f"-DCMAKE_OSX_ARCHITECTURES:STRING={macos_arch}",
            # Enable lzma and libdeflate for libtiff
            f"-DCMAKE_PREFIX_PATH={homebrew_prefix}",
            "-G", "Ninja",
        ]
    elif platform == "android":
        # Get NDK path from environment
        ndk_version = env.get("ndk_version", "23.1.7779620") if env else "23.1.7779620"
        android_home = env.get("ANDROID_HOME", os.environ.get("ANDROID_HOME", os.environ.get("ANDROID_SDK_ROOT", ""))) if env else os.environ.get("ANDROID_HOME", os.environ.get("ANDROID_SDK_ROOT", ""))
        ndk_root = os.path.join(android_home, "ndk", ndk_version) if android_home else os.environ.get("ANDROID_NDK_ROOT", "")
        toolchain_file = os.path.join(ndk_root, "build", "cmake", "android.toolchain.cmake")

        # Map godot-cpp arch to Android ABI
        arch = env.get("arch", "arm64") if env else "arm64"
        android_abi_map = {
            "arm64": "arm64-v8a",
            "arm32": "armeabi-v7a",
            "x86_64": "x86_64",
            "x86_32": "x86",
        }
        android_abi = android_abi_map.get(arch, "arm64-v8a")

        return common_args + [
            "-G", "Ninja",
            "-DANDROID_PLATFORM=android-24",
            f"-DANDROID_ABI={android_abi}",
            f"-DCMAKE_TOOLCHAIN_FILE={toolchain_file}",
        ]
    elif platform == "ios":
        arch = env.get("arch", "arm64") if env else "arm64"
        ios_arch = "arm64" if arch in ("arm64", "universal") else arch
        ios_min_version = env.get("ios_min_version", "13.0") if env else "13.0"
        return common_args + [
            "-G", "Ninja",
            "-DCMAKE_SYSTEM_NAME=iOS",
            # As on macOS: glTF-SDK pins its deployment target to 10.11 and builds with -Werror,
            # and current SDKs' libc++ warns about such old targets.
            "-DCMAKE_CXX_FLAGS=-w",
            f"-DCMAKE_OSX_ARCHITECTURES={ios_arch}",
            f"-DCMAKE_OSX_DEPLOYMENT_TARGET={ios_min_version}",
            # Xerces-C's sample programs would be app bundles without an install destination.
            "-DCMAKE_MACOSX_BUNDLE=OFF",
            # libcitygml's tessellator includes <OpenGL/glu.h>; thirdparty/glu provides it.
            f"-DGLU_INCLUDE_PATH={GLU_ROOT / 'include'}",
        ]
    else:  # linux
        use_clang = env.get("use_clang", False) if env else False
        if use_clang:
            # Use Clang to avoid GCC 15+ strict C++23 header requirements
            # Skip tests (-DANDROID=ON) as libplateau tests have libjpeg path issues on Linux
            return common_args + [
                "-DCMAKE_C_COMPILER=clang",
                "-DCMAKE_CXX_COMPILER=clang++",
                "-DCMAKE_CXX_FLAGS=-w -Wno-c++11-narrowing -include cstdint -include climits -include cstddef -include algorithm",
                "-DANDROID=ON",  # Skip test build (tests are excluded for Android/iOS)
            ]
        else:
            # Default: GCC 13 (Ubuntu 24.04 default) - matches libplateau CI environment
            # -include climits: Required for INT_MAX/INT_MIN in vector_tile_downloader.cpp
            # -include cstddef: Required for size_t in height_map_with_alpha.h
            return common_args + [
                "-DCMAKE_CXX_FLAGS=-w -include climits -include cstddef",
            ]


def _patch_submodule_sources(libplateau_root):
    """Patch vendored submodule sources for cmake 4.x compat and macOS/Linux builds.

    1. glTF-SDK: Replace PowerShell SchemaJson.h generator with Python
    2. RapidJSON download template: Bump cmake_minimum_required from 2.8.2 to 3.5
    """
    # --- 1. glTF-SDK: PowerShell → Python ---
    cmake_file = libplateau_root / "3rdparty" / "glTF-SDK" / "glTF-SDK" / "GLTFSDK" / "CMakeLists.txt"
    marker = "# patched-by-godot-plateau"
    if cmake_file.exists():
        content = cmake_file.read_text()
        if marker not in content:
            # Copy our Python generator next to the original PowerShell script
            # SConstruct is exec()'d by scons, so __file__ is not defined.
            py_gen_src = Path.cwd() / "GenerateSchemaJsonHeader.py"
            py_gen_dst = cmake_file.parent / "GenerateSchemaJsonHeader.py"
            if py_gen_src.exists() and not py_gen_dst.exists():
                shutil.copy2(py_gen_src, py_gen_dst)

            old = '''find_program(POWERSHELL_PATH NAMES pwsh powershell NO_PACKAGE_ROOT_PATH NO_CMAKE_PATH NO_CMAKE_ENVIRONMENT_PATH NO_CMAKE_SYSTEM_PATH NO_CMAKE_FIND_ROOT_PATH)

add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/GeneratedFiles/SchemaJson.h
    COMMAND ${POWERSHELL_PATH} -ExecutionPolicy Bypass "${CMAKE_CURRENT_LIST_DIR}/GenerateSchemaJsonHeader.ps1" -outPath "${CMAKE_BINARY_DIR}/GeneratedFiles"
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    DEPENDS "${schema_deps}"
)'''

            new = f'''# {marker}
find_package(Python3 COMPONENTS Interpreter QUIET)

if(Python3_FOUND)
    add_custom_command(
        OUTPUT ${{CMAKE_BINARY_DIR}}/GeneratedFiles/SchemaJson.h
        COMMAND ${{Python3_EXECUTABLE}} "${{CMAKE_CURRENT_LIST_DIR}}/GenerateSchemaJsonHeader.py" "${{CMAKE_BINARY_DIR}}/GeneratedFiles"
        WORKING_DIRECTORY ${{CMAKE_CURRENT_LIST_DIR}}
        DEPENDS "${{schema_deps}}"
    )
else()
    find_program(POWERSHELL_PATH NAMES pwsh powershell NO_PACKAGE_ROOT_PATH NO_CMAKE_PATH NO_CMAKE_ENVIRONMENT_PATH NO_CMAKE_SYSTEM_PATH NO_CMAKE_FIND_ROOT_PATH)
    add_custom_command(
        OUTPUT ${{CMAKE_BINARY_DIR}}/GeneratedFiles/SchemaJson.h
        COMMAND ${{POWERSHELL_PATH}} -ExecutionPolicy Bypass "${{CMAKE_CURRENT_LIST_DIR}}/GenerateSchemaJsonHeader.ps1" -outPath "${{CMAKE_BINARY_DIR}}/GeneratedFiles"
        WORKING_DIRECTORY ${{CMAKE_CURRENT_LIST_DIR}}
        DEPENDS "${{schema_deps}}"
    )
endif()'''

            if old in content:
                content = content.replace(old, new)
                cmake_file.write_text(content)
                print("[godot-plateau] Patched glTF-SDK CMakeLists.txt to use Python instead of PowerShell")
            else:
                print("[godot-plateau] WARNING: Could not patch glTF-SDK CMakeLists.txt (source text not found)")

    # --- 2. RapidJSON download template: cmake_minimum_required 2.8.2 → 3.5 ---
    rapidjson_template = libplateau_root / "3rdparty" / "glTF-SDK" / "glTF-SDK" / "External" / "RapidJSON" / "CMakeRapidJSONDownload.txt.in"
    if rapidjson_template.exists():
        content = rapidjson_template.read_text()
        old_ver = "cmake_minimum_required(VERSION 2.8.2)"
        new_ver = "cmake_minimum_required(VERSION 3.5)"
        if old_ver in content:
            content = content.replace(old_ver, new_ver)
            rapidjson_template.write_text(content)
            print("[godot-plateau] Patched RapidJSON download template: cmake_minimum_required 2.8.2 → 3.5")


def _apply_submodule_patches():
    """Apply patches/*.patch to the submodules once (a patch that reverses cleanly is already in)."""
    for repo, patch in SUBMODULE_PATCHES:
        def git_apply(*args):
            return subprocess.call(["git", "-C", str(repo), "apply", *args, str(patch)],
                                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        if git_apply("--reverse", "--check") == 0:
            continue
        if git_apply() != 0:
            raise RuntimeError(f"Could not apply {patch.name} to {repo}")
        print(f"[godot-plateau] Applied {patch.name}")


def configure_libplateau(target, source, env):
    """Configure libplateau with cmake."""
    cmake_executable = shutil.which("cmake")
    if not cmake_executable:
        raise RuntimeError("cmake executable not found in PATH.")

    # Patch submodule sources before cmake configure
    _patch_submodule_sources(LIBPLATEAU_ROOT)
    _apply_submodule_patches()

    # Set CMAKE_POLICY_VERSION_MINIMUM as env var so it propagates to all
    # cmake subprocesses (ExternalProject_Add, execute_process, etc.).
    # The -D flag only applies to the top-level configure and doesn't reach
    # nested cmake invocations like libjpeg-turbo or RapidJSON downloads.
    os.environ.setdefault("CMAKE_POLICY_VERSION_MINIMUM", "3.5")

    platform = env["platform"]
    arch = env.get("arch", "")
    build_type = env.get("LIBPLATEAU_BUILD_TYPE", "Release")
    build_dir = get_libplateau_build_dir(platform, arch)

    cmake_args = [
        cmake_executable,
        "-S", str(LIBPLATEAU_ROOT),
        "-B", str(build_dir),
    ] + get_cmake_configure_args(platform, build_dir, build_type, env)

    print(f"Configuring libplateau: {' '.join(cmake_args)}")
    subprocess.check_call(cmake_args)

    target_path = Path(str(target[0]))
    target_path.parent.mkdir(parents=True, exist_ok=True)
    target_path.touch()
    return 0


def build_libplateau(target, source, env):
    """Build libplateau with cmake."""
    cmake_executable = shutil.which("cmake")
    if not cmake_executable:
        raise RuntimeError("cmake executable not found in PATH.")

    platform = env["platform"]
    arch = env.get("arch", "")
    build_type = env.get("LIBPLATEAU_BUILD_TYPE", "Release")
    build_dir = get_libplateau_build_dir(platform, arch)
    skip_tests = env.get("skip_tests", False)

    cmake_args = [
        cmake_executable,
        "--build", str(build_dir),
        "--config", build_type,
    ]

    # Skip tests/examples by building only the main library target
    if skip_tests:
        if platform in ("android", "ios"):
            cmake_args.extend(["--target", "plateau"])
        else:
            cmake_args.extend(["--target", "plateau_combined"])

    print(f"Building libplateau: {' '.join(cmake_args)}")
    subprocess.check_call(cmake_args)

    target_path = Path(str(target[0]))
    target_path.parent.mkdir(parents=True, exist_ok=True)
    target_path.touch()
    return 0


# Initialize environment
localEnv = Environment(tools=["default"], PLATFORM="")

customs = ["custom.py"]
customs = [os.path.abspath(path) for path in customs]

opts = Variables(customs, ARGUMENTS)
opts.Add("homebrew_prefix", "Homebrew installation prefix (macOS only)", DEFAULT_HOMEBREW_PREFIX)
opts.Update(localEnv)

Help(opts.GenerateHelpText(localEnv))

env = localEnv.Clone()

# Ensure godot-cpp exists
if not (os.path.isdir("godot-cpp") and os.listdir("godot-cpp")):
    print_error("""godot-cpp is not available within this folder, as Git submodules haven't been initialized.
Run the following command to download godot-cpp:

    git submodule update --init --recursive""")
    sys.exit(1)

# Map the checkout's path to "." in everything compiled here, godot-cpp included (PATH_MAP_FLAG).
if ARGUMENTS.get("platform", "") != "windows":
    env.Append(CCFLAGS=[PATH_MAP_FLAG])

# Load godot-cpp environment
env = SConscript("godot-cpp/SConstruct", {"env": env, "customs": customs})

# Get platform info
platform = env["platform"]
target = env["target"]
arch = env.get("arch", "")

# macOS: Set deployment target to 15.0 for ARM64 PAC compatibility with macOS 26+
# This fixes pointer authentication crashes on macOS Tahoe (26.x) beta
if platform == "macos" and env.get("macos_deployment_target", "default") == "default":
    env["macos_deployment_target"] = "15.0"
    env.Append(CCFLAGS=["-mmacosx-version-min=15.0"])
    env.Append(LINKFLAGS=["-mmacosx-version-min=15.0"])

# iOS: Default the minimum version to 13.0 (libplateau's CMAKE_OSX_DEPLOYMENT_TARGET follows
# ios_min_version, so an explicit ios_min_version=... applies to both)
if platform == "ios" and env.get("ios_min_version", "12.0") == "12.0":
    env["ios_min_version"] = "13.0"
    # Note: godot-cpp/tools/ios.py already added -miphoneos-version-min with old value,
    # so we need to replace it
    env["CCFLAGS"] = [f for f in env["CCFLAGS"] if "-miphoneos-version-min" not in str(f) and "-mios-simulator-version-min" not in str(f)]
    env["LINKFLAGS"] = [f for f in env["LINKFLAGS"] if "-miphoneos-version-min" not in str(f) and "-mios-simulator-version-min" not in str(f)]
    if env.get("ios_simulator", False):
        env.Append(CCFLAGS=["-mios-simulator-version-min=13.0"])
        env.Append(LINKFLAGS=["-mios-simulator-version-min=13.0"])
    else:
        env.Append(CCFLAGS=["-miphoneos-version-min=13.0"])
        env.Append(LINKFLAGS=["-miphoneos-version-min=13.0"])

# Apple release builds: leave out the debug map (object file paths) and local symbols.
if platform in ("macos", "ios") and target == "template_release":
    env.Append(LINKFLAGS=["-Wl,-S", "-Wl,-x"])

# Ensure libplateau exists
if not LIBPLATEAU_ROOT.exists():
    print_error("""libplateau directory is missing. Please initialize the submodule:

    git submodule update --init --recursive""")
    sys.exit(1)

# Setup libplateau build
libplateau_build_type = get_cmake_build_type(target)
env["LIBPLATEAU_BUILD_TYPE"] = libplateau_build_type
env["use_clang"] = ARGUMENTS.get("use_clang", "no") == "yes"
env["homebrew_prefix"] = ARGUMENTS.get("homebrew_prefix", DEFAULT_HOMEBREW_PREFIX)
env["skip_tests"] = ARGUMENTS.get("skip_tests", "no") == "yes"

libplateau_build_dir = get_libplateau_build_dir(platform, arch)
libplateau_lib_path = get_libplateau_lib_path(platform, libplateau_build_dir, libplateau_build_type)

# Check if we need to build libplateau
skip_libplateau_build = ARGUMENTS.get("skip_libplateau_build", "no") == "yes"

# Create File node for libplateau library
libplateau_lib_file = File(str(libplateau_lib_path))

if not skip_libplateau_build:
    # Configure step
    # Use NoCache() to prevent SCons from caching the marker file
    # This ensures CMake configure runs when cache is restored without actual libraries
    libplateau_configure = env.Command(
        str(libplateau_build_dir / ".cmake_configured"),
        [],
        configure_libplateau,
    )
    libplateau_configure_node = libplateau_configure[0]
    env.NoCache(libplateau_configure_node)
    # Configure again when the cmake arguments change (e.g. compiler flags).
    env.Depends(libplateau_configure_node, env.Value(" ".join(
        get_cmake_configure_args(platform, libplateau_build_dir, libplateau_build_type, env))))

    # Build step - target is the actual library file
    # Also use NoCache() since CMake manages its own build artifacts
    libplateau_build = env.Command(
        libplateau_lib_file,
        [libplateau_configure_node],
        build_libplateau,
    )
    libplateau_build_node = libplateau_build[0]
    env.NoCache(libplateau_build_node)
    # Let CMake/Ninja decide what is out of date (a changed submodule source or patch); the
    # extension relinks only when a library's content changed.
    env.AlwaysBuild(libplateau_build_node)

    # Declare 3rdparty libraries as side effects for Android/iOS
    # This tells SCons that CMake build also produces these files
    if platform in ("android", "ios"):
        libplateau_3rdparty = libplateau_build_dir / "3rdparty"
        thirdparty_libs = [
            str(libplateau_3rdparty / "libcitygml" / "lib" / "libcitygml.a"),
            str(libplateau_3rdparty / "openmesh" / "src" / "OpenMesh" / "Core" / "libOpenMeshCore.a"),
            str(libplateau_3rdparty / "openmesh" / "src" / "OpenMesh" / "Tools" / "libOpenMeshTools.a"),
            str(libplateau_3rdparty / "hmm" / "src" / "libhmm.a"),
            str(libplateau_3rdparty / "glTF-SDK" / "glTF-SDK" / "GLTFSDK" / "libGLTFSDK.a"),
        ]
        if platform == "ios":
            thirdparty_libs.append(str(libplateau_3rdparty / "xerces-c" / "src" / "libxerces-c.a"))
        for lib in thirdparty_libs:
            env.SideEffect(lib, libplateau_build_node)
else:
    libplateau_build_node = None
    if not libplateau_lib_path.exists():
        print_error(f"""libplateau library not found at: {libplateau_lib_path}
Either build libplateau manually or run without skip_libplateau_build=yes""")
        sys.exit(1)

# Add include paths for libplateau
env.Append(CPPPATH=[
    str(LIBPLATEAU_ROOT / "include"),
    str(LIBPLATEAU_ROOT / "3rdparty" / "libcitygml" / "sources" / "include"),
    str(LIBPLATEAU_ROOT / "3rdparty" / "glm"),
])

# Link libplateau and its dependencies
env.Append(LIBS=[libplateau_lib_file])

# Platform-specific libraries and frameworks
if platform == "windows":
    env.Append(LIBS=["glu32", "opengl32", "advapi32", "user32"])
elif platform == "linux":
    # Linux uses combined lib (same as macOS/Windows)
    env.Append(LIBS=["GL", "GLU", "pthread", "dl"])
elif platform == "macos":
    # macOS uses combined lib; add required system frameworks via LINKFLAGS
    env.Append(LINKFLAGS=[
        "-framework", "OpenGL",
        "-framework", "CoreFoundation",
        "-framework", "CoreServices",
    ])
    # Add Homebrew libraries
    homebrew_prefix = env["homebrew_prefix"]
    env.Append(LIBPATH=[homebrew_prefix + "/lib"])
    # Add mesa-glu for GLU functions (required by libcitygml)
    mesa_glu_prefix = homebrew_prefix + "/opt/mesa-glu"
    if os.path.exists(mesa_glu_prefix):
        env.Append(CPPPATH=[mesa_glu_prefix + "/include"])
        env.Append(LIBPATH=[mesa_glu_prefix + "/lib"])
        env.Append(LIBS=["GLU"])
    # Add liblzma and libdeflate for libtiff compression support
    env.Append(LIBS=["lzma", "deflate"])
elif platform == "android":
    env.Append(LIBS=["log", "android"])
    # Android needs 3rdparty libs separately (libplateau.a doesn't include them)
    libplateau_3rdparty = libplateau_build_dir / "3rdparty"
    env.Append(LIBS=[
        File(str(libplateau_3rdparty / "libcitygml" / "lib" / "libcitygml.a")),
        File(str(libplateau_3rdparty / "openmesh" / "src" / "OpenMesh" / "Core" / "libOpenMeshCore.a")),
        File(str(libplateau_3rdparty / "openmesh" / "src" / "OpenMesh" / "Tools" / "libOpenMeshTools.a")),
        File(str(libplateau_3rdparty / "hmm" / "src" / "libhmm.a")),
        File(str(libplateau_3rdparty / "glTF-SDK" / "glTF-SDK" / "GLTFSDK" / "libGLTFSDK.a")),
    ])
elif platform == "ios":
    env.Append(FRAMEWORKS=["Foundation", "CoreGraphics"])
    # iOS needs 3rdparty libs separately (framework doesn't include them)
    libplateau_3rdparty = libplateau_build_dir / "3rdparty"
    env.Append(LIBS=[
        File(str(libplateau_3rdparty / "libcitygml" / "lib" / "libcitygml.a")),
        File(str(libplateau_3rdparty / "openmesh" / "src" / "OpenMesh" / "Core" / "libOpenMeshCore.a")),
        File(str(libplateau_3rdparty / "openmesh" / "src" / "OpenMesh" / "Tools" / "libOpenMeshTools.a")),
        File(str(libplateau_3rdparty / "hmm" / "src" / "libhmm.a")),
        File(str(libplateau_3rdparty / "glTF-SDK" / "glTF-SDK" / "GLTFSDK" / "libGLTFSDK.a")),
        # The CityGML parser: Xerces-C (gnuiconv transcoder, hence libiconv)
        File(str(libplateau_3rdparty / "xerces-c" / "src" / "libxerces-c.a")),
        "iconv",
    ])
    env.Append(CPPPATH=[str(GLU_ROOT / "include")])

# Suppress warnings from libplateau headers and enable C++ exceptions
if platform == "windows":
    env.Append(CXXFLAGS=["/wd4251", "/wd4275", "/EHsc"])
else:
    env.Append(CXXFLAGS=["-Wno-deprecated-declarations", "-Wno-switch"])
    if platform in ("macos", "linux", "android", "ios"):
        env.Append(CXXFLAGS=["-fexceptions"])

# Source files
env.Append(CPPPATH=["src/"])
sources = Glob("src/*.cpp") + Glob("src/plateau/*.cpp")
if platform == "ios":
    sources += [File(str(GLU_ROOT / path)) for path in GLU_SOURCES]

# Build suffix (remove .dev and .universal for compatibility)
suffix = env['suffix'].replace(".dev", "").replace(".universal", "")

# Android needs explicit lib prefix (godot-cpp sets SHLIBPREFIX to empty string)
if platform == "android":
    lib_prefix = "lib"
else:
    lib_prefix = env.subst('$SHLIBPREFIX')

lib_filename = "{}{}{}{}".format(lib_prefix, LIB_NAME, suffix, env.subst('$SHLIBSUFFIX'))

# Determine output directory (include arch for mobile platforms)
if platform in ("android", "ios"):
    bin_dir = "bin/{}/{}".format(platform, arch)
    addons_dir = "{}/addons/plateau/{}/{}/".format(GODOT_PROJECT_DIR, platform, arch)
else:
    bin_dir = "bin/{}".format(platform)
    addons_dir = "{}/addons/plateau/{}/".format(GODOT_PROJECT_DIR, platform)

# Build the library
library = env.SharedLibrary(
    "{}/{}".format(bin_dir, lib_filename),
    source=sources,
)

# Depend on libplateau build if we're building it
if libplateau_build_node:
    env.Depends(library, libplateau_build_node)
    # Also make sources depend on configure step to ensure generated headers exist
    # (citygml_api.h is generated during cmake configure)
    env.Depends(sources, libplateau_configure_node)

# Copy to project addons folder
copy_addons = env.Install(addons_dir, library)

default_args = [library] + copy_addons
Default(*default_args)
