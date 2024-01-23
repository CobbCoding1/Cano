{
  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-23.11";
    utils.url = "github:numtide/flake-utils";
  };

  outputs = { nixpkgs, flake-utils, ... }:
    flake-utils.lib.eachDefaultSystem (system: let
      pkgs = nixpkgs.legacyPackages.${system};
    in rec {
      devShell = pkgs.mkShell {
        inputsFrom = [ packages.cano ];
      };

      packages = rec {
        default = cano;
        cano = pkgs.stdenv.mkDerivation rec {
          name = "cano";
          src = ./.;

          buildInputs = with pkgs; [ ncurses ];
          hardeningDisable = [ "format" "fortify" ];

          buildPhase = ''
            runHook preBuild

            ${pkgs.stdenv.cc}/bin/cc -o cano src/main.c \
              -Wall -Wextra -pedantic \
              -lncurses -lm

            runHook postBuild
          '';

          installPhase = ''
            mkdir -p $out/bin
            cp ${name} $out/bin
          '';
        };
      };
    });
}
