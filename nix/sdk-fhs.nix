{ pkgs ? import <nixpkgs> {} }:
pkgs.buildFHSEnv {
  name = "framare-sdk-env";
  targetPkgs = p: with p; [
    bashInteractive coreutils findutils gnugrep gnused gawk
    git curl cacert openssh rsync which gnutar gzip bzip2 xz unzip
    docker
    glibc libxcrypt-legacy zlib stdenv.cc.cc openssl
    fontconfig freetype harfbuzz glib libGL libxkbcommon wayland dbus
    xorg.libX11 xorg.libXext xorg.libXrender xorg.libXrandr
    xorg.libXi xorg.libXcursor xorg.libXfixes xorg.libICE xorg.libSM
    xorg.libxcb xorg.xcbutil xorg.xcbutilimage xorg.xcbutilkeysyms
    xorg.xcbutilrenderutil xorg.xcbutilwm
    xorg.libXinerama xorg.xcbutilcursor xorg.libXcomposite
  ];
  runScript = "bash";
  profile = ''
    export SSL_CERT_FILE=/etc/ssl/certs/ca-bundle.crt
  '';
}
