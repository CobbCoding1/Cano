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
      packages = with pkgs; [gcc bear valgrind];
      env.HELP_DIR = "docs/help";
    };

    formatter.${system} = pkgs.alejandra;

    packages.${system} = {
      default = self.packages.${system}.cano;
      cano = pkgs.stdenv.mkDerivation {
        inherit hardeningDisable;

        name = "cano";
        src = ./.;

        buildInputs = [pkgs.ncurses];

        makeFlags = "PREFIX=${placeholder "out"}";

        meta = {
          description = "Text Editor Written In C Using ncurses";
          license = pkgs.lib.licenses.asl20;
          maintainers = with pkgs.lib.maintainers; [ sigmanificient ];
          platforms = pkgs.lib.platforms.linux;
          mainProgram = "cano";
        };
      };
    };
  };
}
