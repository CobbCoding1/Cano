{ pkgs ? import <nixpkgs> {}}:
pkgs.stdenv.mkDerivation {
    name = "cano"; # Probably put a more meaningful name here
    src = ./.;
    phases = ["unpackPhase" "buildPhase" "installPhase"];
    buildInputs = with pkgs; [ ncurses ];
    buildPhase = ''
      $CC -I${pkgs.ncurses}/include -L${pkgs.ncurses}/lib -lncurses src/main.c -o cano -Wall -Wextra -pedantic
    '';
    installPhase = ''
      mkdir -p $out/bin
      cp cano $out/bin
    '';
}
