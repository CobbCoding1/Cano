{
  inputs.nixpkgs.url = "github:nixos/nixpkgs/nixpkgs-unstable";

  outputs = {
    self,
    nixpkgs,
  }: let
    system = "x86_64-linux";
    pkgs = nixpkgs.legacyPackages.${system};

    hardeningDisable = ["format" "fortify"];
  in {
    devShells.${system}.default = pkgs.mkShell {
      inherit hardeningDisable;

      inputsFrom = [self.packages.${system}.cano];
      packages = with pkgs; [ gcc bear valgrind ];
    };

    formatter.${system} = pkgs.alejandra;

    packages.${system} = {
      default = self.packages.${system}.cano;
      cano = pkgs.stdenv.mkDerivation {
        inherit hardeningDisable;

        name = "cano";
        src = ./.;

        buildInputs = [pkgs.ncurses];

        installPhase = ''
          runHook preInstall

          install -Dm755 build/cano -t $out/bin

          runHook postInstall
        '';
      };
    };
  };
}
