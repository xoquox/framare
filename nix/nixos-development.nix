{ config, lib, pkgs, ... }:
let
  cfg = config.framareDevelopment;
in
{
  options.framareDevelopment = {
    enable = lib.mkEnableOption "the Framare Sailfish development host";
    user = lib.mkOption {
      type = lib.types.str;
      description = "Existing local user allowed to operate the SDK Docker engine.";
    };
  };

  config = lib.mkIf cfg.enable {
    # The Sailfish SDK does not support rootless Docker.
    virtualisation.docker.enable = true;
    users.users.${cfg.user}.extraGroups = [ "docker" ];
    environment.systemPackages = [ pkgs.go_1_26 ];
    # SDK-specific libraries are isolated in sdk-fhs.nix. Do not replace the
    # system loader or install a second, unrelated host Qt toolchain here.
  };
}
