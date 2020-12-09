{ sources ? import ./nix/sources.nix
, pkgs ? import sources.nixpkgs {} }:

pkgs.mkShell {
  shellHook = ''
    unset MAKEFLAGS
    unset CONFIG_SITE
  '';
  buildInputs = [
    pkgs.libipt
    pkgs.libmpc
    pkgs.mpfr
    pkgs.isl
    pkgs.python3
    pkgs.ncurses
    pkgs.gmp
    pkgs.automake115x
    pkgs.glibcLocales
  ];
}
