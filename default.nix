{ pkgs ? import <nixpkgs> {}}:
pkgs.stdenv.mkDerivation {
    name = "cano"; # Probably put a more meaningful name here
    src = ./.;
    phases = ["unpackPhase" "buildPhase" "installPhase"];
    buildInputs = with pkgs; [ ncurses ];
    buildPhase = ''
      ${pkgs.stdenv.cc}/bin/cc -I${pkgs.ncurses}/include -L${pkgs.ncurses}/lib -lncurses -lm src/main.c -o cano -Wall -Wextra -pedantic
    '';
    installPhase = ''
      mkdir -p $out/bin
      cp cano $out/bin
    '';
}
