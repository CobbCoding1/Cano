{
  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-23.11";
    utils.url = "github:numtide/flake-utils";
  };

  outputs = { nixpkgs, flake-utils, ... }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};

        hardeningDisable = [ "format" "fortify" ];
      in
      rec {
        devShell = pkgs.mkShell {
          inherit hardeningDisable;

          inputsFrom = [ packages.cano ];
        };

        formatter = pkgs.nixpkgs-fmt;

        packages = rec {
          default = cano;
          cano = pkgs.stdenv.mkDerivation rec {
            inherit hardeningDisable;

            name = "cano";
            src = ./.;

            buildInputs = with pkgs; [ gnumake ncurses ];
            installPhase = ''
              mkdir -p $out/bin
              cp build/${name} $out/bin
            '';
          };
        };
      });
}
