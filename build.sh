#!/bin/sh
set -e

case "$(uname -s)" in
  *Darwin*)
    platform="macos"
    scene_ext=".dylib"
    rpath_flag="-Wl,-rpath,@executable_path"
    export_flag="-Wl,-export_dynamic"
    ;;
  *MSYS*|*MINGW*|*Cygwin*)
    platform="windows"
    scene_ext=".dll"
    rpath_flag=""
    export_flag=""
    ;;
  *)
    platform="linux"
    scene_ext=".so"
    rpath_flag="-Wl,-rpath,\$ORIGIN:\$ORIGIN/..:\$ORIGIN/../.."
    export_flag="-Wl,--export-dynamic"
    ;;
esac
project="${1:-inferno}"
compiler="g++"
sdl_flags="$(pkg-config --cflags --libs sdl3 sdl3-ttf sdl3-image)"
json_flags="$(pkg-config --cflags --libs jsoncpp)"
if [ "$CROSS" = "windows" ]; then
  platform="windows"
  scene_ext=".dll"
  compiler="x86_64-w64-mingw32-g++"
  rpath_flag=""
  export_flag=""
  json_flags=""
  sdl_flags="-DSDL_MAIN_HANDLED \
    -Ithirdparty/windows/SDL3-headers \
    -Ithirdparty/windows/SDL3_ttf-headers \
    -Ithirdparty/windows/SDL3_image-headers \
    -Lthirdparty/windows/SDL3 \
    -Lthirdparty/windows/SDL3_ttf \
    -Lthirdparty/windows/SDL3_image \
    -static-libgcc -static-libstdc++ \
    -lSDL3 -lSDL3_ttf -lSDL3_image"
fi
if [ ! -d "$project" ]; then
  printf '>> project folder not found: %s .. ERROR\n' "$project"
  exit 1
fi
if ! command -v "$compiler" >/dev/null 2>&1; then
  printf '>> compiler not found: %s .. ERROR\n' "$compiler"
  exit 1
fi
exe_name="Inferno"
if [ "$platform" = "windows" ]; then
  exe_name="Inferno.exe"
fi
out_dir="build/$platform"
mkdir -p "$out_dir/scenes" "$out_dir/fonts" "$out_dir/sounds" "$out_dir/assets"
cp "$project/fonts/debug.ttf" "$out_dir/fonts/"
cp "$project/sounds/"*.wav "$out_dir/sounds/"
cp -r "$project/assets/." "$out_dir/assets/"
cp "$project/game_properties.json" "$out_dir/"
cp "$project/VERSION" "$out_dir/"

if [ "$platform" = "windows" ]; then
  cp thirdparty/windows/SDL3/SDL3.dll "$out_dir/"
  cp thirdparty/windows/SDL3_ttf/SDL3_ttf.dll "$out_dir/"
  cp thirdparty/windows/SDL3_image/SDL3_image.dll "$out_dir/"
fi

scene_libs=""
for scene_dir in "$project"/scenes/*/; do
  [ -d "$scene_dir" ] || continue
  scene_name="$(basename "$scene_dir")"
  scene_out_dir="$out_dir/scenes/$scene_name"
  mkdir -p "$scene_out_dir"
  if [ -f "$scene_dir/properties.json" ]; then
    cp "$scene_dir/properties.json" "$scene_out_dir/properties.json"
  fi
  scene_source="$scene_dir/$scene_name.cpp"
  if [ ! -f "$scene_source" ]; then
    printf '>> scene %s has no %s.cpp .. SKIPPED\n' "$scene_name" "$scene_name"
    continue
  fi
  $compiler -fPIC -shared \
    "$scene_source" \
    src/cpp/sounds.cpp \
    src/cpp/engine.cpp \
    src/cpp/tween.cpp \
    src/cpp/ui.cpp \
    -I src/header \
    -o "$scene_out_dir/$scene_name$scene_ext" \
    $sdl_flags $json_flags $rpath_flag
  scene_libs="$scene_libs $scene_out_dir/$scene_name$scene_ext"
done
for flat_scene in "$project"/scenes/*.cpp; do
  [ -e "$flat_scene" ] || continue
  printf '>> flat scene ignored, move it into a scene folder: %s\n' "$flat_scene"
done

$compiler \
  src/cpp/main.cpp \
  src/cpp/game.cpp \
  src/cpp/sounds.cpp \
  src/cpp/engine.cpp \
  src/cpp/tween.cpp \
  -o "$out_dir/$exe_name" \
  $rpath_flag $export_flag $sdl_flags

sh src/dlls.sh \
  "$out_dir" \
  "$out_dir/$exe_name" \
  $scene_libs
echo ">> build successful .."