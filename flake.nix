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
        cano = pkgs.stdenv.mkDerivation {
          name = "cano";
          src = ./.;

          buildInputs = with pkgs; [ gnumake ncurses ];
          hardeningDisable = [ "format" "fortify" ];

          installPhase = ''
            mkdir -p $out/bin
            cp build/cano $out/bin
          '';
        };
      };
    });
}
