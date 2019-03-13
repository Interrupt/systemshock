{
  nixpkgs ? <nixpkgs>,
  pkgs ? (import nixpkgs {}).pkgsi686Linux
}:

with pkgs;
with pkgs.stdenv.lib;

let
  makeSDLExtraCFlags = l: builtins.concatStringsSep " " (
    concatMap (p: [ "-I${getDev p}/include/SDL2" ]) l
  );
  SDLlibs = [ SDL2 SDL2_mixer ];
in pkgs.stdenv.mkDerivation {
  name = "systemshock";
  src = ./.;

  nativeBuildInputs = [ cmake pkgconfig ];
  buildInputs = SDLlibs;

  NIX_CFLAGS_COMPILE = (makeSDLExtraCFlags SDLlibs) + " -Wimplicit-fallthrough=0";

  # These two are just for debugging to be able to see what's going wrong
  makeFlags = "VERBOSE=1";
  enableParallelBuilding = false;

  installPhase = ''
      install -Dsm 644 systemshock $out/bin/systemshock
  '';
}
