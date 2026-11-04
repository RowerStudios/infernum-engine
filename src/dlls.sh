out_dir="$1"
shift
case "$(uname -s)" in
  *Darwin*) host_os="macos" ;;
  *MSYS*|*MINGW*|*Cygwin*) host_os="windows" ;;
  *) host_os="linux" ;;
esac

list_deps() {
  if [ "$host_os" = "macos" ]; then
    otool -L "$1" 2>/dev/null | awk 'NR>2 {print $1}'
  else
    ldd "$1" 2>/dev/null | awk '{print $3}'
  fi
}

scan_lib() {
  target="$1"
  if [ "$(head -c 2 "$target" 2>/dev/null)" = "MZ" ]; then
    for lib in $(objdump -p "$target" 2>/dev/null | awk '/DLL Name:/{print $3}'); do
      check="$(printf '%s' "$lib" | tr '[:upper:]' '[:lower:]')"
      case "$check" in
        kernel32*|ntdll*|user32*|gdi32*|ucrtbase*|msvcrt*|shell32*|advapi32*|ole32*|oleaut32*|ws2_32*|setupapi*|imm32*|winmm*|win32u*|dbghelp*|dinput8*|version*|shcore*|sechost*|bcrypt*|crypt32*|iphlpapi*|userenv*|powrprof*|api-ms-win-*|ext-ms-*|windows.storage*|combase*|propsys*|wldp*|msimg32*) continue ;;
      esac
      if [ -f "$out_dir/$lib" ]; then
        continue
      fi
      if [ -f "/usr/x86_64-w64-mingw32/sys-root/mingw/bin/$lib" ]; then
        cp "/usr/x86_64-w64-mingw32/sys-root/mingw/bin/$lib" "$out_dir/$lib"
        printf '>> bundled %s\n' "$lib"
      else
        printf '>> dll not bundled: %s .. WARNING\n' "$lib"
      fi
    done
    return
  fi
  for lib in $(list_deps "$target"); do
    case "$lib" in
      /*) ;;
      *) continue ;;
    esac
    case "$lib" in
      *libc.so*|*libm.so*|*ld-linux*|*linux-vdso*|*libdl.so*|*libpthread.so*|*librt.so*|*libnsl*|*libcrypt*|*libutil*|*libresolv*|*libanl*) continue ;;
    esac
    if [ "$host_os" = "macos" ]; then
      case "$lib" in
        /usr/lib/*|/System/*) continue ;;
      esac
    fi
    name="$(basename "$lib")"
    if [ -f "$out_dir/$name" ]; then
      continue
    fi
    cp "$lib" "$out_dir/$name"
    printf '>> bundled %s\n' "$name"
    scan_lib "$out_dir/$name"
  done
}

for target in "$@"; do
  scan_lib "$target"
done

if [ "$host_os" = "linux" ]; then
  if command -v patchelf > /dev/null; then
    for lib in "$out_dir"/lib*.so*; do
      [ -f "$lib" ] || continue
      patchelf --set-rpath '$ORIGIN' "$lib"
    done
  fi
fi

if [ "$host_os" = "macos" ]; then
  for dylib in "$out_dir"/*.dylib; do
    [ -f "$dylib" ] || continue
    install_name_tool -id "@rpath/$(basename "$dylib")" "$dylib"
    for dep in $(otool -L "$dylib" | awk 'NR>2 {print $1}'); do
      case "$dep" in
        /usr/lib/*|/System/*) continue ;;
        /*) install_name_tool -change "$dep" "@rpath/$(basename "$dep")" "$dylib" ;;
      esac
    done
  done
fi
