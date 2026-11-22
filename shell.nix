{ pkgs ? import <nixpkgs> {} }:
pkgs.mkShell {
  packages = with pkgs; [
    go_1_26
    git
    curl
    python3
    file
    binutils
    shellcheck
  ];
  # Never silently fetch another compiler based on an upstream go.mod.
  GOTOOLCHAIN = "local";
}
